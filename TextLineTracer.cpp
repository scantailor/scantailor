/*
    Scan Tailor - Interactive post-processing tool for scanned pages.
	Copyright (C)  Joseph Artsimovich <joseph.artsimovich@gmail.com>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "TextLineTracer.h"
#include "TaskStatus.h"
#include "DebugImages.h"
#include "NumericTraits.h"
#include "FastQueue.h"
#include "VecNT.h"
#include "MatrixCalc.h"
#include "ToLineProjector.h"
#include "imageproc/BinaryImage.h"
#include "imageproc/Connectivity.h"
#include "imageproc/ConnectivityMap.h"
#include "imageproc/InfluenceMap.h"
#include "imageproc/ColorForId.h"
#include <QPoint>
#include <QPointF>
#include <QSize>
#include <QRect>
#include <QRectF>
#include <QPolygonF>
#include <QVector>
#include <QImage>
#include <QPainter>
#include <QColor>
#include <QPen>
#include <QDebug>
#include <boost/foreach.hpp>
#include <vector>
#include <set>
#include <map>
#include <functional>
#include <algorithm>
#include <iterator>
#include <stdexcept>
#include <math.h>
#include <stddef.h>
#include <stdint.h>
#include <assert.h>

using namespace imageproc;

struct TextLineTracer::BoundingBox
{
	int top;
	int left;
	int bottom;
	int right;

	BoundingBox() {
		top = left = NumericTraits<int>::max();
		bottom = right = NumericTraits<int>::min();
	}

	int width() const { return right - left + 1; }

	int height() const { return bottom - top + 1; }

	int square() const { return width() * height(); }

	void extend(int x, int y) {
		top = std::min(top, y);
		left = std::min(left, x);
		bottom = std::max(bottom, y);
		right = std::max(right, x);
	}

	QPoint center() const { return QPoint(left + width()/2, top + height()/2); }

	QPointF centerF() const { return QPointF(0.5*(left + right + 1), 0.5*(top + bottom + 1)); }
};

struct TextLineTracer::Component
{
	enum { LEFTMOST_BIT_POS = 0, RIGHTMOST_BIT_POS = 1 };
	enum Flag {
		LEFTMOST    = 1 << LEFTMOST_BIT_POS,
		RIGHTMOST   = 1 << RIGHTMOST_BIT_POS,
		ENCOUNTERED = 1 << 2,
		THROWN_AWAY = 1 << 3,
		IN_QUEUE    = 1 << 4,
		FINALIZED   = 1 << 5,
	};

	std::vector<uint32_t> connectedLabels;
	BoundingBox bbox;
	uint32_t label;
	union {
		uint32_t startLabel;
		int closestEdgePointX;
		int minY;
	};
	union {
		uint32_t prevInPath;
		int closestEdgePointY;
		int maxY;
	};
	union {
		uint32_t priorityIndex;
		int edgeX;
	};
	union {
		float pathCost;
		float distToEdge;
	};
	int flags;

	Component() : startLabel(0), prevInPath(0), priorityIndex(~uint32_t(0)),
		pathCost(NumericTraits<float>::max()), flags(0) {}

	bool higherPriority(Component& other) const {
		return pathCost < other.pathCost;
	}
};

struct TextLineTracer::Connection
{
	uint32_t lesserLabel;
	uint32_t greaterLabel;
	mutable uint32_t distSq;

	Connection(uint32_t label0, uint32_t label1, uint32_t sqdist) {
		if (label0 < label1) {
			lesserLabel = label0;
			greaterLabel = label1;
		} else {
			lesserLabel = label1;
			greaterLabel = label0;
		}
		distSq = sqdist;
	}

	bool operator<(Connection const& rhs) const {
		if (lesserLabel != rhs.lesserLabel) {
			return lesserLabel < rhs.lesserLabel;
		} else {
			return greaterLabel < rhs.greaterLabel;
		}
	}

	bool goodConnection(std::vector<Component> const& components) const {
		BoundingBox const& bbox1 = components[lesserLabel].bbox;
		BoundingBox const& bbox2 = components[greaterLabel].bbox;

		//int const min_square = std::min(bbox1.square(), bbox2.square());
		//int const y_delta = bbox1.center().y() - bbox2.center().y();
		
		//return y_delta * y_delta <= min_square;
#if 1
		if (bbox1.top > bbox2.bottom) {
			return false;
		}

		if (bbox2.top > bbox1.bottom) {
			return false;
		}

		return true;
#endif
	}
};

class TextLineTracer::CompPriorityQueue
{
public:
	void reserve(size_t capacity) { m_index.reserve(capacity); }

	bool empty() const { return m_index.empty(); }

	Component* front() { return m_index.front(); }

	void push(Component* obj);

	void pop();

	void erase(Component* obj);

	void reposition(Component* obj);
private:
	static size_t parent(size_t idx) { return (idx - 1) / 2; }

	static size_t left(size_t idx) { return idx * 2 + 1; }

	static size_t right(size_t idx) { return idx * 2 + 2; }

	void bubbleUp(size_t idx);

	void bubbleDown(size_t idx);

	std::vector<Component*> m_index;
};

void
TextLineTracer::trace(
	imageproc::BinaryImage const& input,
	TaskStatus const& status, DebugImages* dbg)
{
	ConnectivityMap cmap(input, CONN8);
	if (dbg) {
		dbg->add(cmap.visualized(), "cmap");
	}

	std::vector<Component> components;
	calcBoundingBoxes(cmap, components);

	// We subdivide long connected components into multiple
	// vertical stripes for the following reasons:
	// 1. Two words with letters glued together in the curved
	//    area might have a significant vertical shift in relation
	//    to each other, which might prevent a connection between
	//    them to be formed.
	// 2. We want to detect not just text lines, but also various
	//    solid lines.
	subdivideLongComponents(cmap, components);
	if (dbg) {
		dbg->add(cmap.visualized(), "more_segments");
	}

	for (uint32_t label = 0; label < components.size(); ++label) {
		components[label].label = label;
	}

	int const median_comp_height = calcMedianCompHeight(components);

	InfluenceMap imap(cmap);
	cmap = ConnectivityMap(); // Save memory.

	if (imap.maxLabel() == 0) {
		// No regions in the map.
		return;
	}
	if (dbg) {
		dbg->add(overlay(imap.visualized(), input), "voronoi");
	}

	std::set<Connection> connections;
	findVoronoiConnections(imap, connections);

	// Remove "bad" connections, and while at it, populate connectedLabels lists.
	std::set<Connection>::iterator it(connections.begin());
	while (it != connections.end()) {
		if (true) { //it->goodConnection(components)) {
			components[it->lesserLabel].connectedLabels.push_back(it->greaterLabel);
			components[it->greaterLabel].connectedLabels.push_back(it->lesserLabel);
			++it;
		} else {
			connections.erase(it++);
		}
	}

	// Label some components as leftmost, rightmost or both.
	labelEdgeComponents(components, imap, 0, Component::LEFTMOST);
	labelEdgeComponents(components, imap, imap.size().width() - 1, Component::RIGHTMOST);
	if (dbg) {
		dbg->add(visualizeLeftRightComponents(components, imap), "endpoints");
	}

	// Estimate vertical boundaries.
	QLineF const left_cutter(
		estimateVerticalCutter(
			imap, components, Component::LEFTMOST, median_comp_height, std::less<int>()
		)
	);
	QLineF const right_cutter(
		estimateVerticalCutter(
			imap, components, Component::RIGHTMOST, median_comp_height, std::greater<int>()
		)
	);
	if (dbg) {
		dbg->add(visualizeCutters(input.toQImage(), left_cutter, right_cutter), "vert_cutters");
	}

	std::list<std::vector<uint32_t> > lines;
	connectEndpoints(components, lines);
	if (dbg) {
		dbg->add(
			visualizeTracedLinesAndCutters(
				input.toQImage(), lines, components, left_cutter, right_cutter
			),
			"traced_lines"
		);
	}
}

void
TextLineTracer::calcBoundingBoxes(
	imageproc::ConnectivityMap const& cmap, std::vector<Component>& components)
{
	components.resize(cmap.maxLabel() + 1);

	int const w = cmap.size().width();
	int const h = cmap.size().height();
	uint32_t const* cmap_line = cmap.data();
	int const cmap_stride = cmap.stride();

	for (int y = 0; y < h; ++y, cmap_line += cmap_stride) {
		for (int x = 0; x < w; ++x) {
			uint32_t const label = cmap_line[x];
			if (label) {
				components[label].bbox.extend(x, y);
			}
		}
	}
}

void
TextLineTracer::subdivideLongComponents(
	imageproc::ConnectivityMap& cmap, std::vector<Component>& components)
{
	uint32_t* cmap_data = cmap.data();
	int const cmap_stride = cmap.stride();
	uint32_t const initial_max_label = cmap.maxLabel();
	uint32_t new_label = initial_max_label;

	for (uint32_t label = 1; label <= initial_max_label; ++label) {
		Component& comp = components[label];
		int const width = comp.bbox.width();
		int const height = comp.bbox.height();
		
		int const target_segment_length = std::max<int>(30, height); // XXX: make 30 dpi-dependent.
		if (target_segment_length * 2 > width) {
			// No point in subdividing.
			continue;
		}

		int num_segments = width / target_segment_length;
		assert(num_segments >= 2);

		int const segment_length = width / num_segments;
		assert(segment_length > 0);

		// Give new labels to pixels within each segment, except the first one.
		int left = comp.bbox.left + width - (num_segments - 1) * segment_length;
		if (left == comp.bbox.left) {
			// We don't want any components to remain without any pixels.
			--num_segments;
			left += segment_length;
		}
		
		uint32_t* const cmap_top_line = cmap_data + comp.bbox.top * cmap_stride;
		for (; left <= comp.bbox.right; left += segment_length) {
			++new_label;
			
			uint32_t* cmap_line = cmap_top_line + left;
			bool new_label_used = false;
			for (int y = comp.bbox.top; y <= comp.bbox.bottom; ++y, cmap_line += cmap_stride) {
				for (int off = 0; off < segment_length; ++off) {
					if (cmap_line[off] == label) {
						cmap_line[off] = new_label;
						new_label_used = true;
					}
				}
			}

			if (!new_label_used) {
				// We don't want any components to remain without any pixels.
				--new_label;
			}
		}
	}

	cmap.setMaxLabel(new_label);
	
	components.clear();
	calcBoundingBoxes(cmap, components);
}

int
TextLineTracer::calcMedianCompHeight(std::vector<Component> const& components)
{
	std::vector<int> heights;
	heights.reserve(components.size());
	
	uint32_t const num_components = components.size();
	for (uint32_t label = 1; label < num_components; ++label) {
		heights.push_back(components[label].bbox.height());
	}

	if (heights.empty()) {
		return 0;
	}

	std::vector<int>::iterator mid(heights.begin() + heights.size()/2);
	std::nth_element(heights.begin(), mid, heights.end());
	return *mid;
}

void
TextLineTracer::findVoronoiConnections(InfluenceMap const& imap, std::set<Connection>& connections)
{
	InfluenceMap::Cell const* imap_line = imap.data();
	int const imap_stride = imap.stride();
	InfluenceMap::Cell const* imap_prev_line = imap_line - imap_stride;

	int const w = imap.size().width();
	int const h = imap.size().height();

	for (int y = 0; y < h; ++y) {
		
		for (int x = 0; x < w; ++x) {
			InfluenceMap::Cell const& tl = imap_prev_line[x - 1];
			InfluenceMap::Cell const& tr = imap_prev_line[x];
			InfluenceMap::Cell const& bl = imap_line[x - 1];
			InfluenceMap::Cell const& br = imap_line[x];

			processPossibleConnection(connections, tl, tr);
			processPossibleConnection(connections, tr, br);
			processPossibleConnection(connections, br, bl);
			processPossibleConnection(connections, bl, tl);
		}

		imap_prev_line = imap_line;
		imap_line += imap_stride;
	}
}

void
TextLineTracer::processPossibleConnection(
	std::set<Connection>& connections,
	imageproc::InfluenceMap::Cell const& cell1,
	imageproc::InfluenceMap::Cell const& cell2)
{
	if (cell1.label == cell2.label || cell1.label == 0 || cell2.label == 0) {
		return;
	}

	uint32_t const sqdist = cell1.distSq + cell2.distSq + 1;
	Connection const conn(cell1.label, cell2.label, sqdist);
	std::set<Connection>::iterator it(connections.lower_bound(conn));
	if (it == connections.end() || conn < *it) {
		connections.insert(it, conn);
	} else if (sqdist < it->distSq) {
		it->distSq = sqdist;
	}
}

void
TextLineTracer::labelEdgeComponents(
	std::vector<Component>& components, imageproc::InfluenceMap const& imap,
	int const x, int const flag)
{
	int const h = imap.size().height();
	int const imap_stride = imap.stride();
	InfluenceMap::Cell const* const imap_start_cell = imap.data() + x;
	InfluenceMap::Cell const* imap_cell;

	// Initialization.
	imap_cell = imap_start_cell;
	for (int y = 0; y < h; ++y, imap_cell += imap_stride) {
		uint32_t const label = imap_cell->label;
		Component& comp = components[label];
		comp.flags &= ~(flag|Component::ENCOUNTERED);
		comp.distToEdge = NumericTraits<float>::max();
	}

	// Initial tagging plus finding the closest point on the edge.
	imap_cell = imap_start_cell;
	for (int y = 0; y < h; ++y, imap_cell += imap_stride) {
		uint32_t const label = imap_cell->label;
		Component& comp = components[label];
		comp.flags |= flag;
		QPoint const dv(comp.bbox.center() - QPoint(x, y));
		float const dist = sqrt(double(dv.x()*dv.x() + dv.y()*dv.y()));
		if (dist < comp.distToEdge) {
			comp.distToEdge = dist;
			comp.closestEdgePointX = x;
			comp.closestEdgePointY = y;
		}
	}

	// If a line of text has lots of whitespace before or after it,
	// not only the first/last letter, but one or more adjacent ones
	// may be tagged on the previous step.  Here we detect and untag them.
	imap_cell = imap_start_cell;
	for (int y = 0; y < h; ++y, imap_cell += imap_stride) {
		uint32_t const label = imap_cell->label;
		Component& comp = components[label];
		if (comp.flags & Component::ENCOUNTERED) {
			continue;
		}
		comp.flags |= Component::ENCOUNTERED;

		QPoint const comp_center(comp.bbox.center());
		BOOST_FOREACH(uint32_t label2, comp.connectedLabels) {
			Component& comp2 = components[label2];
			if (!(comp2.flags & flag)) {
				continue; // Already untagged.
			}
			QPoint const dv(comp2.bbox.center() - comp_center);
			float const comp_comp2_dist = sqrt(double(dv.x()*dv.x() + dv.y()*dv.y()));
			if (comp.distToEdge + comp_comp2_dist < comp2.distToEdge) {
				// Distance to edge through another component is shorter then a direct distance!
				comp2.flags &= ~flag;
			}
		}
	}
}

template<typename BetterX>
QLineF
TextLineTracer::estimateVerticalCutter(
	imageproc::InfluenceMap const& imap, std::vector<Component>& components,
	int const flag, int median_comp_height, BetterX better_x)
{
	// Initialization.
	int const initial_x = (flag == Component::LEFTMOST)
		? NumericTraits<int>::max() : NumericTraits<int>::min();
	BOOST_FOREACH(Component& comp, components) {
		comp.edgeX = initial_x;
		comp.minY = NumericTraits<int>::max();
		comp.maxY = NumericTraits<int>::min();
		comp.flags &= ~(Component::ENCOUNTERED|Component::THROWN_AWAY);
	}

	int const w = imap.size().width();
	int const h = imap.size().height();
	int const imap_stride = imap.stride();
	InfluenceMap::Cell const* imap_line = imap.data();

	std::vector<size_t> edge_comp_labels;

	for (int y = 0; y < h; ++y, imap_line += imap_stride) {
		for (int x = 0; x < w; ++x) {
			InfluenceMap::Cell const& cell = imap_line[x];
			if (cell.distSq != 0) {
				continue;
			}

			uint32_t const label = imap_line[x].label;
			Component& comp = components[label];
			if (!(comp.flags & flag)) {
				continue;
			}

			if (!(comp.flags & Component::ENCOUNTERED)) {
				edge_comp_labels.push_back(label);
				comp.flags |= Component::ENCOUNTERED;
			}

			if (x == comp.edgeX) {
				comp.minY = std::min<int>(y, comp.minY);
				comp.maxY = std::max<int>(y, comp.maxY);
			} else if (better_x(x, comp.edgeX)) {
				comp.edgeX = x; 
				comp.minY = y;
				comp.maxY = y;
			}
		}
	}
	
	std::vector<double> At;
	std::vector<double> b;
	std::vector<double> x(2, 0.0);

	uint32_t const num_components = components.size();
	QLineF line;

	int const max_iterations = 4;
	for (int iteration = 0; iteration < max_iterations; ++iteration) {

		At.clear();
		BOOST_FOREACH(uint32_t label, edge_comp_labels) {
			Component const& comp = components[label];
			if (comp.flags & Component::THROWN_AWAY) {
				continue;
			}
			At.push_back(comp.edgeX);
			At.push_back(comp.minY);
			At.push_back(comp.edgeX);
			At.push_back(comp.maxY);
		}

		if (At.empty()) {
			return QLineF();
		}

		b.resize(At.size()/2, 1.0);

		// At*A*x = At*b
		// x = (At*A)-1*At*b

		try {
			DynamicMatrixCalc<double> mc;
			(
				(mc(&At[0], 2, b.size())*mc(&At[0], 2, b.size()).trans()).inv() *
				mc(&At[0], 2, b.size()) * mc(&b[0], b.size(), 1)
			).write(&x[0]);
		} catch (std::runtime_error const&) {
			break;
		}

		// Ax + By = 1  |  A = x[0], B = x[1]
		if (fabs(x[0]) < fabs(x[1])) { // |A| < |B|
			double const x1 = 0;
			double const x2 = 1;
			double const y1 = (1 - x[0]*x1) / x[1];
			double const y2 = (1 - x[0]*x2) / x[1];
			line.setLine(x1, y1, x2, y2);
		} else {
			double const y1 = 0;
			double const y2 = 1;
			double const x1 = (1 - x[1]*y1) / x[0];
			double const x2 = (1 - x[1]*y1) / x[0];
			line.setLine(x1, y1, x2, y2);
		}

		Vec2d outward_normal(line.normalVector().p2() - line.p1());
		if (better_x(-outward_normal[0], outward_normal[0])) {
			// It points inward -> flip it.
			outward_normal = -outward_normal;
		}
		outward_normal /= sqrt(outward_normal.squaredNorm()); // XXX: prevent possible division by zero

		ToLineProjector const projector(line);

		double max_dist_from_line = 0;
		size_t const At_size = At.size();
		for (size_t i = 0; i < At_size; i += 2) {
			QPointF const pt(At[i], At[i+1]);
			QPointF const proj(projector.projectionPoint(pt));
			double const dist = outward_normal.dot(pt - proj);
			max_dist_from_line = std::max(max_dist_from_line, dist);
			if (dist < -median_comp_height*2) {
				// Throw away this sample.  Actually two samples,
				// as that's home much each component contributes.
				uint32_t const label = edge_comp_labels[i/4];
				components[label].flags |= Component::THROWN_AWAY;
			}
		}

		line.translate(outward_normal * max_dist_from_line);

		if (edge_comp_labels.size()*4 == At_size) {
			// No samples were thrown away - no need to do more iterations.
			break;
		}
	}


	// Extend the line to cover the whole image, and possibly more.

	ToLineProjector const projector(line);
	QPolygonF poly;
	poly << QPointF(0, 0) << QPointF(0, w) << QPointF(w, h) << QPointF(0, h);
	
	double p_min = NumericTraits<double>::max();
	double p_max = NumericTraits<double>::min();
	BOOST_FOREACH(QPointF const& pt, poly) {
		double const p = projector.projectionScalar(pt);
		p_min = std::min(p_min, p);
		p_max = std::max(p_max, p);
	}

	return QLineF(line.pointAt(p_min), line.pointAt(p_max));
}

void
TextLineTracer::connectEndpoints(
	std::vector<Component>& components, std::list<std::vector<uint32_t> >& lines)
{
	CompPriorityQueue queue;
	queue.reserve(components.size());

	// Initialization.
	uint32_t const num_components = components.size();
	for (uint32_t label = 1; label < num_components; ++label) {
		Component& comp = components[label];
		comp.prevInPath = 0;
		comp.pathCost = NumericTraits<float>::max();
		comp.priorityIndex = ~uint32_t(0);
		comp.flags &= ~(Component::IN_QUEUE|Component::FINALIZED);
		if (comp.flags & Component::LEFTMOST) {
			comp.pathCost = 0;
			comp.flags |= Component::FINALIZED;
		}
	}

	// Start from LEFTMOST components.
	// Note that we can't merge this loop with the previous one,
	// as otherwise propagateLengthFrom() might act on unititialized components.
	for (uint32_t label = 1; label < num_components; ++label) {
		Component& comp = components[label];
		if (comp.flags & Component::LEFTMOST) {
			propagateLengthFrom(comp, components, queue);
		}
	}

	// Path length propagation.
	while (!queue.empty()) {
		Component& comp = *queue.front();
		queue.pop();
		comp.flags &= ~Component::IN_QUEUE;
		comp.flags |= Component::FINALIZED;
		
		propagateLengthFrom(comp, components, queue);
	}

	// Clear connectedLabels for all components and rebuild them
	// only for paths that reach all the way from a leftmost
	// to a rightmost point.
	BOOST_FOREACH(Component& comp, components) {
		comp.connectedLabels.clear();
	}
	for (uint32_t label = 1; label < num_components; ++label) {
		Component* comp = &components[label];
		if (!(comp->flags & Component::RIGHTMOST)) {
			continue;
		}

		for (Component* prev_comp; comp->prevInPath; comp = prev_comp) {
			prev_comp = &components[comp->prevInPath];
			prev_comp->connectedLabels.push_back(comp->label);
			// We only really need unidirectional connections.
		}
	}

	// Initialize another pass of shortest-path-in-a-graph.
	// This time the cost function will be the maximum cosine
	// of an angle in a path.
	BOOST_FOREACH(Component& comp, components) {
		comp.startLabel = 0;
		comp.prevInPath = 0;
		comp.pathCost = NumericTraits<float>::max();
		comp.priorityIndex = ~uint32_t(0);
		comp.flags &= ~(Component::IN_QUEUE|Component::FINALIZED);
		if (comp.flags & Component::LEFTMOST) {
			comp.startLabel = comp.label;
			comp.pathCost = 0;
			comp.flags |= Component::FINALIZED;
		}
	}

	assert(queue.empty());

	// Start from LEFTMOST components.
	// Note that we can't merge this loop with the previous one,
	// as otherwise propagateMaxCurvatureFrom() might act on unititialized components.
	for (uint32_t label = 1; label < num_components; ++label) {
		Component& comp = components[label];
		if (comp.flags & Component::LEFTMOST) {
			propagateMaxCurvatureFrom(comp, components, queue);
		}
	}

	// Max curvature propagation.
	while (!queue.empty()) {
		Component& comp = *queue.front();
		queue.pop();
		comp.flags &= ~Component::IN_QUEUE;
		comp.flags |= Component::FINALIZED;
		
		propagateMaxCurvatureFrom(comp, components, queue);
	}

	std::map<uint32_t, uint32_t> best_paths; // start label -> end label

	for (uint32_t endpoint_label = 1; endpoint_label < num_components; ++endpoint_label) {
		Component& endpoint_comp = components[endpoint_label];
		if (!(endpoint_comp.flags & Component::RIGHTMOST)) {
			continue;
		}

		uint32_t& best_endpoint = best_paths[endpoint_comp.startLabel];
		if (best_endpoint == 0 || endpoint_comp.pathCost < components[best_endpoint].pathCost) {
			best_endpoint = endpoint_label;
		}
	}

	// Trace entries of best_path from endpoint to startpoint.
	lines.clear();
	typedef std::map<uint32_t, uint32_t>::value_type KeyValue;
	BOOST_FOREACH(KeyValue const& kv, best_paths) {
		Component* comp = &components[kv.second];
		lines.push_back(std::vector<uint32_t>());
		std::vector<uint32_t>& line = lines.back();
		for (Component* prev_comp; comp->prevInPath; comp = prev_comp) {
			prev_comp = &components[comp->prevInPath];
			line.push_back(comp->label);
		}
		line.push_back(comp->label);
	}
}

void
TextLineTracer::propagateLengthFrom(
	Component& comp, std::vector<Component>& components, CompPriorityQueue& queue)
{
	QPoint const comp_center(comp.bbox.center());
	QPoint vec_from_prev(0, 0);
	if (comp.prevInPath) {
		vec_from_prev = comp_center - components[comp.prevInPath].bbox.center();
	}
	int const vec_from_prev_sqlen = vec_from_prev.x()*vec_from_prev.x() + vec_from_prev.y()*vec_from_prev.y();
	double const vec_from_prev_len = sqrt(double(vec_from_prev_sqlen));

	BOOST_FOREACH(uint32_t level1_nbh_label, comp.connectedLabels) {
		Component& level1_nbh_comp = components[level1_nbh_label];
		propagateLengthFromTo(vec_from_prev, vec_from_prev_len, comp, comp_center, level1_nbh_comp, queue);

		// We allow connections not only with direct neighbors, but also with neighbors' neighbors.
		// This helps to jump over punctuation marks for instance.
		BOOST_FOREACH(uint32_t level2_nbh_label, level1_nbh_comp.connectedLabels) {
			Component& level2_nbh_comp = components[level2_nbh_label];
			propagateLengthFromTo(vec_from_prev, vec_from_prev_len, comp, comp_center, level2_nbh_comp, queue);
		}
	}
}

void
TextLineTracer::propagateLengthFromTo(
	QPoint vec_from_prev, double const vec_from_prev_len, Component const& from_comp,
	QPoint const comp_center, Component& to_comp, CompPriorityQueue& queue)
{
	if (to_comp.flags & Component::FINALIZED) {
		return;
	}

	uint32_t const from_label = from_comp.label;
	uint32_t const to_label = to_comp.label;

	if (to_label == from_label) {
		return;
	}

	QPoint const vec_to_next(to_comp.bbox.center() - comp_center);
	int const vec_to_next_sqlen = vec_to_next.x()*vec_to_next.x() + vec_to_next.y()*vec_to_next.y();
	double const vec_to_next_len = sqrt(double(vec_to_next_sqlen));
	
	if (vec_to_next_len > 300) { // XXX: adjust for DPI
		return;
	}
	
	// Cosine of the angle between vec_from_prev and vec_to_next, but not yet.
	double cos = vec_from_prev.x()*vec_to_next.x() + vec_from_prev.y()*vec_to_next.y();
	if (from_comp.prevInPath != 0) { // Skip this branch if vec_from_prev is fake.
		cos /= vec_from_prev_len*vec_to_next_len;
		// Now it's finally the real cosine.
		assert(cos > -1.001 && cos < 1.001); 

		if (cos < 0.9) {
			// The angle is too sharp.  We don't allow that.
			if (to_comp.prevInPath == from_label) {
				// Even though the continuation is too sharp,
				// it's already connected to us.  Must be a residual
				// of another path.  Fortunately, the Dijkstra algorithm
				// we are using guarantees it won't propagate more than
				// one hop.  We are going to just invalidate this hop.
				to_comp.prevInPath = 0;
				to_comp.pathCost = NumericTraits<float>::max();
				if (to_comp.flags & Component::IN_QUEUE) {
					to_comp.flags &= ~Component::IN_QUEUE;
					queue.erase(&to_comp);
				}
			}
			return;
		}
	}

	float const new_cost = from_comp.pathCost + vec_to_next_len;
	if (new_cost < to_comp.pathCost) {
		to_comp.pathCost = new_cost;
		to_comp.prevInPath = from_label;
		if (to_comp.flags & Component::IN_QUEUE) {
			queue.reposition(&to_comp);
		} else {
			queue.push(&to_comp);
			to_comp.flags |= Component::IN_QUEUE;
		}
	}
}

void
TextLineTracer::propagateMaxCurvatureFrom(
	Component& comp, std::vector<Component>& components, CompPriorityQueue& queue)
{
	QPoint const comp_center(comp.bbox.center());
	QPointF normal_from_prev(0, 0);
	if (comp.prevInPath) {
		normal_from_prev = comp_center - components[comp.prevInPath].bbox.center();
		normal_from_prev = QPointF(-normal_from_prev.y(), normal_from_prev.x());
	}
	double const normal_from_prev_len = sqrt(double(normal_from_prev.x()*normal_from_prev.x() + normal_from_prev.y()*normal_from_prev.y()));
	if (normal_from_prev_len) {
		normal_from_prev /= normal_from_prev_len;
	}

	BOOST_FOREACH(uint32_t nbh_label, comp.connectedLabels) {
		Component& nbh_comp = components[nbh_label];
		propagateMaxCurvatureFromTo(normal_from_prev, comp, comp_center, nbh_comp, queue);
	}
}

void
TextLineTracer::propagateMaxCurvatureFromTo(
	QPointF normal_from_prev, Component const& from_comp,
	QPoint comp_center, Component& to_comp, CompPriorityQueue& queue)
{
	if (to_comp.flags & Component::FINALIZED) {
		return;
	}

	uint32_t const from_label = from_comp.label;
	uint32_t const to_label = to_comp.label;

	QPointF const vec_to_next(to_comp.bbox.center() - comp_center);
	double const vec_to_next_len = sqrt(double(vec_to_next.x()*vec_to_next.x() + vec_to_next.y()*vec_to_next.y()));
	
	double cos = normal_from_prev.x()*vec_to_next.x() + normal_from_prev.y()*vec_to_next.y();
	if (cos != 0) {
		cos /= vec_to_next_len;
	}
	
	assert(cos >= -1.001 && cos <= 1.001);

	float const new_cost = std::max<float>(from_comp.pathCost, fabs(cos));

	if (new_cost < to_comp.pathCost) {
		to_comp.pathCost = new_cost;
		to_comp.prevInPath = from_label;
		to_comp.startLabel = from_comp.startLabel;
		if (to_comp.flags & Component::IN_QUEUE) {
			queue.reposition(&to_comp);
		} else {
			queue.push(&to_comp);
			to_comp.flags |= Component::IN_QUEUE;
		}
	}
}



QImage
TextLineTracer::overlay(QImage const& background, imageproc::BinaryImage const& overlay)
{
	QImage layer1(background.convertToFormat(QImage::Format_ARGB32_Premultiplied));
	QImage layer2(background.size(), QImage::Format_ARGB32_Premultiplied);
	layer2.fill(0xff000000); // black.
	layer2.setAlphaChannel(overlay.inverted().toQImage());
	
	QPainter painter(&layer1);
	painter.drawImage(layer1.rect(), layer2);
	return layer1;
}

QImage
TextLineTracer::visualizeLeftRightComponents(
	std::vector<Component> const& components, imageproc::InfluenceMap const& imap)
{
	int const w = imap.size().width();
	int const h = imap.size().height();

	QImage image(w, h, QImage::Format_RGB32);
	image.fill(0xffffffff); // white
	uint32_t* image_line = (uint32_t*)image.bits();
	int const image_stride = image.bytesPerLine() / 4;

	InfluenceMap::Cell const* imap_line = imap.data();
	int const imap_stride = imap.stride();

	static uint32_t const colors[2][2] = {
		{ 0xff000000, 0xff00ff00 },
		{ 0xff0000ff, 0xffff0000 }
	};

	for (int y = 0; y < h; ++y, imap_line += imap_stride, image_line += image_stride) {
		for (int x = 0; x < w; ++x) {
			InfluenceMap::Cell const& cell = imap_line[x];
			if (cell.distSq) {
				continue;
			}

			Component const& comp = components[cell.label];
			int const leftmost = (comp.flags >> Component::LEFTMOST_BIT_POS) & 1;
			int const rightmost = (comp.flags >> Component::RIGHTMOST_BIT_POS) & 1;
			image_line[x] = colors[leftmost][rightmost];
		}
	}

	return image;
}

QImage
TextLineTracer::visualizeCutters(
	QImage const& background, QLineF const& cutter1, QLineF const& cutter2)
{
	QImage canvas(background.convertToFormat(QImage::Format_ARGB32_Premultiplied));
	QPainter painter(&canvas);
	painter.setRenderHint(QPainter::Antialiasing);
	
	QPen pen(QColor(0, 0, 255, 180));
	pen.setWidthF(5.0);
	painter.setPen(pen);

	painter.drawLine(cutter1);
	painter.drawLine(cutter2);

	return canvas;
}

QImage
TextLineTracer::visualizeTracedLinesAndCutters(
	QImage const& background, std::list<std::vector<uint32_t> > const& lines,
	std::vector<Component> const& components, QLineF const& cutter1, QLineF const& cutter2)
{
	QImage canvas(background.convertToFormat(QImage::Format_ARGB32_Premultiplied));
	QPainter painter(&canvas);
	painter.setRenderHint(QPainter::Antialiasing);

	QPen pen(QColor(255, 0, 0, 180));
	pen.setWidthF(10.0);
	painter.setPen(pen);
	
	QVector<QPointF> polyline;
	BOOST_FOREACH(std::vector<uint32_t> const& line, lines) {
		polyline.clear();
		BOOST_FOREACH(uint32_t label, line) {
			polyline.push_back(components[label].bbox.centerF());
		}
		painter.drawPolyline(polyline);
	}

	pen.setColor(QColor(0, 0, 255, 180));
	painter.setPen(pen);

	painter.drawLine(cutter1);
	painter.drawLine(cutter2);

	return canvas;
}


/*======================== CompPriorityQueue ========================*/

void
TextLineTracer::CompPriorityQueue::push(Component* obj)
{
	size_t const idx = m_index.size();
	m_index.push_back(obj);
	obj->priorityIndex = idx;
	bubbleUp(idx);
}

void
TextLineTracer::CompPriorityQueue::pop()
{
	assert(!empty());

	m_index.front() = m_index.back();
	m_index.pop_back();
	if (!empty()) {
		bubbleDown(0);
	}
}

void
TextLineTracer::CompPriorityQueue::erase(Component* obj)
{
	assert(m_index[obj->priorityIndex] == obj);
	m_index[obj->priorityIndex] = m_index.back();
	m_index[obj->priorityIndex]->priorityIndex = obj->priorityIndex;
	m_index.pop_back();
	reposition(m_index[obj->priorityIndex]);
}

void
TextLineTracer::CompPriorityQueue::reposition(Component* obj)
{
	assert(m_index[obj->priorityIndex] == obj);
	bubbleDown(obj->priorityIndex);
	assert(m_index[obj->priorityIndex] == obj);
	bubbleUp(obj->priorityIndex);
	assert(m_index[obj->priorityIndex] == obj);
}

void
TextLineTracer::CompPriorityQueue::bubbleUp(size_t idx)
{
	// Iteratively swap the element with its parent,
	// if it's greater than the parent.

	assert(idx < m_index.size());

	Component* const obj = m_index[idx];

	while (idx > 0) {
		size_t const parent_idx = parent(idx);
		if (!obj->higherPriority(*m_index[parent_idx])) {
			break;
		}
		m_index[idx] = m_index[parent_idx];
		m_index[idx]->priorityIndex = idx;
		idx = parent_idx;
	}

	m_index[idx] = obj;
	obj->priorityIndex = idx;
}

void
TextLineTracer::CompPriorityQueue::bubbleDown(size_t idx)
{
	size_t const len = m_index.size();
	assert(idx < len);

	Component* const obj = m_index[idx];

	// While any child is greater than the element itself,
	// swap it with the greatest child.

	for (;;) {
		size_t const lft = left(idx);
		size_t const rgt = right(idx);
		size_t best_child;

		if(rgt < len) {
			best_child = m_index[lft]->higherPriority(*m_index[rgt]) ? lft : rgt;
		} else if (lft < len) {
			best_child = lft;
		} else {
			break;
		}
        
		if (m_index[best_child]->higherPriority(*obj)) {
			m_index[idx] = m_index[best_child];
			m_index[idx]->priorityIndex = idx;
            idx = best_child;
		} else {
			break;
		}
    }

	m_index[idx] = obj;
	obj->priorityIndex = idx;
}
