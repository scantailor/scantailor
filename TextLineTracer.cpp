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
#include "TextLineRefiner.h"
#include "DetectVertContentBounds.h"
#include "TowardsLineTracer.h"
#include "Dpi.h"
#include "TaskStatus.h"
#include "DebugImages.h"
#include "NumericTraits.h"
#include "MatrixCalc.h"
#include "VecNT.h"
#include "Grid.h"
#include "PriorityQueue.h"
#include "SidesOfLine.h"
#include "ToLineProjector.h"
#include "PolylineIntersector.h"
#include "CylindricalSurfaceDewarper.h"
#include "PerformanceTimer.h"
#include "filters/output/Curve.h"  // TODO: move these to global namespace
#include "filters/output/DistortionModel.h" 
#include "imageproc/BinaryImage.h"
#include "imageproc/BinaryThreshold.h"
#include "imageproc/Binarize.h"
#include "imageproc/Grayscale.h"
#include "imageproc/GrayImage.h"
#include "imageproc/Scale.h"
#include "imageproc/Constants.h"
#include "imageproc/GaussBlur.h"
#include "imageproc/Morphology.h"
#include "imageproc/MorphGradientDetect.h"
#include "imageproc/RasterOp.h"
#include "imageproc/GrayRasterOp.h"
#include "imageproc/RasterOpGeneric.h"
#include "imageproc/SeedFill.h"
#include "imageproc/FindPeaksGeneric.h"
#include "imageproc/ConnectivityMap.h"
#include "imageproc/ColorForId.h"
#include <QTransform>
#include <QImage>
#include <QRect>
#include <QPainter>
#include <QBrush>
#include <QPen>
#include <QColor>
#include <QtGlobal>
#include <boost/scoped_array.hpp>
#include <boost/foreach.hpp>
#include <boost/lambda/lambda.hpp>
#include <boost/lambda/bind.hpp>
#include <boost/lambda/if.hpp>
#include <algorithm>
#include <set>
#include <map>
#include <deque>
#include <utility>
#include <stdexcept>
#include <stdlib.h>
#include <math.h>

using namespace imageproc;

namespace
{

	uint8_t darkest(uint8_t lhs, uint8_t rhs) {
		return lhs < rhs ? lhs : rhs;
	}

	uint8_t lightest(uint8_t lhs, uint8_t rhs) {
		return lhs > rhs ? lhs : rhs;
	}

	uint8_t darker(uint8_t color) {
		return color == 0 ? 0 : color - 1;
	}

}

class TextLineTracer::CentroidCalculator
{
public:
	CentroidCalculator() : m_sumX(0), m_sumY(0), m_numSamples(0) {}

	void processSample(int x, int y) {
		m_sumX += x;
		m_sumY += y;
		++m_numSamples;
	}

	Vec2f centroid() const {
		if (m_numSamples == 0) {
			return Vec2f(0, 0);
		} else {
			float const r_num_samples = 1.0f / m_numSamples;
			return Vec2f(float(m_sumX) * r_num_samples, float(m_sumY) * r_num_samples);
		}
	}
private:
	int m_sumX;
	int m_sumY;
	int m_numSamples;
};

struct TextLineTracer::Region
{
	Vec2f centroid;
	std::vector<RegionIdx> connectedRegions;
	bool leftmost;
	bool rightmost;

	Region() : leftmost(false), rightmost(false) {}
};

struct TextLineTracer::GridNode
{
private:

public:
	static uint32_t const INVALID_REGION_IDX = 0x7FFFFF;

	GridNode() : m_data() {}

	GridNode(uint8_t gray_level, RegionIdx region_idx, uint32_t finalized)
		: m_data((finalized << 31) | (region_idx << 8) | uint32_t(gray_level)) {}

	uint8_t grayLevel() const { return static_cast<uint8_t>(m_data & 0xff); }

	void setGrayLevel(uint8_t gray_level) {
		m_data = (m_data & ~GRAY_LEVEL_MASK) | uint32_t(gray_level);
	}

	uint32_t regionIdx() const { return (m_data & REGION_IDX_MASK) >> 8; }

	void setRegionIdx(uint32_t region_idx) {
		assert((region_idx & ~INVALID_REGION_IDX) == 0);
		m_data = (m_data & ~REGION_IDX_MASK) | (region_idx << 8);
	}

	uint32_t finalized() const { return (m_data & FINALIZED_MASK) >> 31; }

	void setFinalized(uint32_t finalized) {
		assert(finalized <= 1);
		m_data = (m_data & ~FINALIZED_MASK) | (finalized << 31);
	}
private:
	// Layout (MSB to LSB): [finalized: 1 bit][region idx: 23 bits][gray level: 8 bits]
	static uint32_t const GRAY_LEVEL_MASK = 0x000000FF;
	static uint32_t const REGION_IDX_MASK = 0x7FFFFF00;
	static uint32_t const FINALIZED_MASK  = 0x80000000;

	uint32_t m_data;
};

struct TextLineTracer::RegionGrowingPosition
{
	int gridOffset;
	uint32_t order;

	RegionGrowingPosition(int grid_offset, uint32_t ord)
		: gridOffset(grid_offset), order(ord) {}
};

class TextLineTracer::RegionGrowingQueue :
	public PriorityQueue<RegionGrowingPosition, RegionGrowingQueue>
{
public:
	RegionGrowingQueue(GridNode const* grid_data) : m_pGridData(grid_data) {}

	bool higherThan(RegionGrowingPosition const& lhs, RegionGrowingPosition const& rhs) const {
		GridNode const* lhs_node = m_pGridData + lhs.gridOffset;
		GridNode const* rhs_node = m_pGridData + rhs.gridOffset;
		if (lhs_node->grayLevel() < rhs_node->grayLevel()) {
			return true;
		} else if (lhs_node->grayLevel() == rhs_node->grayLevel()) {
			return lhs.order < rhs.order;
		} else {
			return false;
		}
	}

	void setIndex(RegionGrowingPosition, size_t) {}
private:
	GridNode const* m_pGridData;
};

/**
 * Edge is an bidirectional connection between two regions.
 * Geometrically it can be viewed as a connection between their centroids.
 * Note that centroids are calculated based on region seeds, not on full
 * region areas.
 */
struct TextLineTracer::Edge
{
	RegionIdx lesserRegionIdx;
	RegionIdx greaterRegionIdx;

	Edge(RegionIdx region_idx1, RegionIdx region_idx2) {
		if (region_idx1 < region_idx2) {
			lesserRegionIdx = region_idx1;
			greaterRegionIdx = region_idx2;
		} else {
			lesserRegionIdx = region_idx2;
			greaterRegionIdx = region_idx1;
		}
	}

	bool operator<(Edge const& rhs) const {
		if (lesserRegionIdx < rhs.lesserRegionIdx) {
			return true;
		} else if (lesserRegionIdx > rhs.lesserRegionIdx) {
			return false;
		} else {
			return greaterRegionIdx < rhs.greaterRegionIdx;
		}
	}
};

/**
 * A connection between two edges.
 */
struct TextLineTracer::EdgeConnection
{
	EdgeNodeIdx edgeNodeIdx;
	float cost;

	EdgeConnection(EdgeNodeIdx idx, float cost) : edgeNodeIdx(idx), cost(cost) {}
};

/**
 * A node in a graph that represents a connection between two regions.
 */
struct TextLineTracer::EdgeNode
{
	Edge edge;
	std::vector<EdgeConnection> connectedEdges;
	float pathCost;
	EdgeNodeIdx prevEdgeNodeIdx;
	RegionIdx leftmostRegionIdx;
	uint32_t heapIdx;

	EdgeNode(Edge const& edg) : edge(edg), pathCost(NumericTraits<float>::max()),
		prevEdgeNodeIdx(~EdgeNodeIdx(0)), leftmostRegionIdx(~RegionIdx(0)), heapIdx(~uint32_t(0)) {}
};

class TextLineTracer::ShortestPathQueue : public PriorityQueue<EdgeNodeIdx, ShortestPathQueue>
{
public:
	ShortestPathQueue(std::vector<EdgeNode>& edge_nodes) : m_rEdgeNodes(edge_nodes) {}

	bool higherThan(EdgeNodeIdx lhs, EdgeNodeIdx rhs) const {
		EdgeNode const& lhs_node = m_rEdgeNodes[lhs];
		EdgeNode const& rhs_node = m_rEdgeNodes[rhs];
		return lhs_node.pathCost < rhs_node.pathCost;
	}

	void setIndex(EdgeNodeIdx edge_node_idx, uint32_t heap_idx) {
		m_rEdgeNodes[edge_node_idx].heapIdx = heap_idx;
	}

	void reposition(EdgeNodeIdx edge_node_idx) {
		PriorityQueue<EdgeNodeIdx, ShortestPathQueue>::reposition(
			m_rEdgeNodes[edge_node_idx].heapIdx
		);
	}
private:
	std::vector<EdgeNode>& m_rEdgeNodes;
};


std::list<std::vector<QPointF> >
TextLineTracer::trace(
	GrayImage const& input, Dpi const& dpi, QRect const& content_rect,
	TaskStatus const& status, DebugImages* dbg)
{
	using namespace boost::lambda;

	GrayImage downscaled(downscale(input, dpi));
	if (dbg) {
		dbg->add(downscaled, "downscaled");
	}

	int const downscaled_width = downscaled.width();
	int const downscaled_height = downscaled.height();

	double const downscale_x_factor = double(downscaled_width) / input.width();
	double const downscale_y_factor = double(downscaled_height) / input.height();
	QTransform to_orig;
	to_orig.scale(1.0 / downscale_x_factor, 1.0 / downscale_y_factor);

	QRect const downscaled_content_rect(to_orig.inverted().mapRect(content_rect));
	Dpi const downscaled_dpi(
		qRound(dpi.horizontal() * downscale_x_factor),
		qRound(dpi.vertical() * downscale_y_factor)
	);

	BinaryImage binarized(binarizeWolf(downscaled, QSize(31, 31)));
	if (dbg) {
		dbg->add(binarized, "binarized");
	}

	// detectVertContentBounds() is sensitive to clutter and speckles, so let's try to remove it.
	sanitizeBinaryImage(binarized, downscaled_content_rect);
	if (dbg) {
		dbg->add(binarized, "sanitized");
	}

	std::pair<QLineF, QLineF> const vert_bounds(detectVertContentBounds(binarized, dbg));
	if (dbg) {
		dbg->add(visualizeVerticalBounds(binarized.toQImage(), vert_bounds), "vert_bounds");
	}

	GrayImage blurred(gaussBlur(stretchGrayRange(downscaled), 17, 5));
	if (dbg) {
		dbg->add(blurred.toQImage(), "blurred");
	}

	GrayImage eroded(erodeGray(blurred, QSize(31, 31)));
	rasterOpGeneric(
		eroded.data(), eroded.stride(), eroded.size(),
		blurred.data(), blurred.stride(),
		if_then_else(_1 > _2 + 8, _1 = uint8_t(0), _1 = uint8_t(255))
	);
	BinaryImage thick_mask(eroded);
	eroded = GrayImage();
	if (dbg) {
		dbg->add(thick_mask, "thick_mask");
	}

	std::list<std::vector<QPointF> > polylines;
	segmentBlurredTextLines(blurred, thick_mask, polylines, dbg);

	if (polylines.size() < 2 && !dbg) {
		polylines.clear();
		return polylines;
	}

	filterOutOfBoundsCurves(polylines, vert_bounds.first, vert_bounds.second);

	BOOST_FOREACH(std::vector<QPointF>& polyline, polylines) {
		extendOrTrimPolyline(
			polyline, vert_bounds.first, vert_bounds.second,
			binarized, blurred, thick_mask
		);
	}
	if (dbg) {
		dbg->add(
			visualizeExtendedPolylines(
				blurred.toQImage(), thick_mask, polylines,
				vert_bounds.first, vert_bounds.second
			),
			"extended_polylines"
		);
	}

	TextLineRefiner refiner(downscaled, Dpi(200, 200), dbg);
	refiner.refine(polylines, /*iterations=*/100, dbg, &downscaled.toQImage());

	filterEdgyCurves(polylines);
	if (dbg) {
		dbg->add(visualizePolylines(downscaled, polylines), "edgy_curves_removed");
	}

	BOOST_FOREACH(std::vector<QPointF>& polyline, polylines) {
		makeLeftToRight(polyline);
	}

	pickRepresentativeLines(polylines, blurred.rect(), vert_bounds.first, vert_bounds.second);
	if (dbg) {
		dbg->add(visualizePolylines(downscaled.toQImage(), polylines, &vert_bounds), "representative_lines");
	}

	// Transform back to original coordinates.
	BOOST_FOREACH(std::vector<QPointF>& polyline, polylines) {
		BOOST_FOREACH(QPointF& pt, polyline) {
			pt = to_orig.map(pt);
		}
	}

	return polylines;
}

GrayImage
TextLineTracer::downscale(GrayImage const& input, Dpi const& dpi)
{
	// Downscale to 200 DPI.
	QSize downscaled_size(input.size());
	if (dpi.horizontal() < 180 || dpi.horizontal() > 220 || dpi.vertical() < 180 || dpi.vertical() > 220) {
		downscaled_size.setWidth(std::max<int>(1, input.width() * 200 / dpi.horizontal()));
		downscaled_size.setHeight(std::max<int>(1, input.height() * 200 / dpi.vertical()));
	}

	return scaleToGray(input, downscaled_size);
}

void
TextLineTracer::segmentBlurredTextLines(
	GrayImage const& blurred, BinaryImage const& thick_mask,
	std::list<std::vector<QPointF> >& out, DebugImages* dbg)
{
	int const width = blurred.width();
	int const height = blurred.height();

	BinaryImage region_seeds(
		findPeaksGeneric<uint8_t>(
			&darkest, &lightest, &darker, QSize(31, 15), 255,
			blurred.data(), blurred.stride(), blurred.size()
		)
	);
	
	// We don't want peaks outside of the thick mask.
	// This mostly happens on pictures.
	region_seeds = seedFill(thick_mask, region_seeds, CONN8);

	// We really don't want two region seeds close to each other.
	// Even though the peak_neighbourhood parameter we pass to findPeaksGeneric()
	// will suppress nearby weaker peaks, but it won't suppress a nearby peak
	// if it has has exactly the same value.  Therefore, we dilate those peaks.
	// Note that closeBrick() wouldn't handle cases like:
	// x
	//    x
	region_seeds = dilateBrick(region_seeds, QSize(9, 9));
	if (dbg) {
		dbg->add(region_seeds, "region_seeds");
	}

	std::vector<Region> regions;
	std::set<Edge> edges;

	labelAndGrowRegions(blurred, region_seeds.release(), thick_mask, regions, edges, dbg);

	std::vector<EdgeNode> edge_nodes;
	std::map<Edge, uint32_t> edge_to_index;
	edge_nodes.reserve(edges.size());

	// Populate ConnComp::connectedRagions and edge_nodes.
	BOOST_FOREACH(Edge const& edge, edges) {
		edge_to_index[edge] = edge_nodes.size();
		edge_nodes.push_back(EdgeNode(edge));

		regions[edge.lesserRegionIdx].connectedRegions.push_back(edge.greaterRegionIdx);
		regions[edge.greaterRegionIdx].connectedRegions.push_back(edge.lesserRegionIdx);
	}

	float const cos_threshold = cos(15 * constants::DEG2RAD);
	float const cos_sq_threshold = cos_threshold * cos_threshold;

	uint32_t const num_regions = regions.size();

	// Populate EdgeNode::connectedEdges
	for (RegionIdx region_idx = 1; region_idx < num_regions; ++region_idx) {
		Region const& region = regions[region_idx];
		size_t const num_connected_regions = region.connectedRegions.size();
		for (size_t i = 0; i < num_connected_regions; ++i) {
			RegionIdx const region1_idx = region.connectedRegions[i];
			assert(region1_idx != region_idx);
			Edge const edge1(region_idx, region1_idx);
			uint32_t const edge1_node_idx = edge_to_index[edge1];
			EdgeNode& edge1_node = edge_nodes[edge1_node_idx];
			Vec2f const vec1(regions[region1_idx].centroid - region.centroid);

			for (size_t j = i + 1; j < num_connected_regions; ++j) {
				RegionIdx const region2_idx = region.connectedRegions[j];
				assert(region2_idx != region_idx && region2_idx != region1_idx);
				Edge const edge2(region_idx, region2_idx);
				uint32_t const edge2_node_idx = edge_to_index[edge2];
				EdgeNode& edge2_node = edge_nodes[edge2_node_idx];
				Vec2f const vec2(regions[region2_idx].centroid - region.centroid);
				
				float const dot = vec1.dot(vec2);
				float const cos_sq = (fabs(dot) * -dot) / (vec1.squaredNorm() * vec2.squaredNorm());
				float const cost = std::max<float>(1.0f - cos_sq, 0);

				if (cos_sq > cos_sq_threshold) {
					edge1_node.connectedEdges.push_back(EdgeConnection(edge2_node_idx, cost));
					edge2_node.connectedEdges.push_back(EdgeConnection(edge1_node_idx, cost));
				}
			}
		}
	}
	
	ShortestPathQueue queue(edge_nodes);
	uint32_t const num_edge_nodes = edge_nodes.size();

	// Put leftmost nodes in the queue with the path cost of zero.
	for (uint32_t edge_node_idx = 0; edge_node_idx < num_edge_nodes; ++edge_node_idx) {
		EdgeNode& edge_node = edge_nodes[edge_node_idx];
		RegionIdx const region1_idx = edge_node.edge.lesserRegionIdx;
		RegionIdx const region2_idx = edge_node.edge.greaterRegionIdx;

		if (regions[region1_idx].leftmost) {
			edge_node.pathCost = 0;
			edge_node.leftmostRegionIdx = region1_idx;
			queue.push(edge_node_idx);
		} else if (regions[region2_idx].leftmost) {
			edge_node.pathCost = 0;
			edge_node.leftmostRegionIdx = region2_idx;
			queue.push(edge_node_idx);
		}
	}

	while (!queue.empty()) {
		uint32_t const edge_node_idx = queue.front();
		queue.pop();

		EdgeNode& edge_node = edge_nodes[edge_node_idx];
		edge_node.heapIdx = ~uint32_t(0);

		BOOST_FOREACH(EdgeConnection const& connection, edge_node.connectedEdges) {
			EdgeNode& edge_node2 = edge_nodes[connection.edgeNodeIdx];
			float const new_path_cost = std::max<float>(
				edge_node.pathCost, connection.cost
			) + 0.001 * connection.cost;
			if (new_path_cost < edge_node2.pathCost) {
				edge_node2.pathCost = new_path_cost;
				edge_node2.prevEdgeNodeIdx = edge_node_idx;
				edge_node2.leftmostRegionIdx = edge_node.leftmostRegionIdx;
				if (edge_node2.heapIdx == ~uint32_t(0)) {
					queue.push(connection.edgeNodeIdx);
				} else {
					queue.reposition(connection.edgeNodeIdx);
				}
			}
		}
	}

	std::vector<std::vector<EdgeNodeIdx> > edge_node_paths;
	extractEdegeNodePaths(edge_node_paths, edge_nodes, regions);

	// Visualize refined graph.
	if (dbg) {
		QImage canvas(blurred.toQImage().convertToFormat(QImage::Format_ARGB32_Premultiplied));
		{
			QPainter painter(&canvas);
			
			// Visualize connections.
			painter.setRenderHint(QPainter::Antialiasing);
			QPen pen(Qt::blue);
			pen.setWidthF(2.0);
			painter.setPen(pen);
			BOOST_FOREACH(std::vector<uint32_t> const& path, edge_node_paths) {
				BOOST_FOREACH(uint32_t const edge_node_idx, path) {
					Edge const& edge = edge_nodes[edge_node_idx].edge;
					painter.drawLine(
						regions[edge.lesserRegionIdx].centroid,
						regions[edge.greaterRegionIdx].centroid
					);
				}
			}

			// Visualize peaks.
			painter.setPen(Qt::NoPen);
			QColor color;
			BOOST_FOREACH(Region const& region, regions) {
				if (region.leftmost && region.rightmost) {
					color = Qt::green;
				} else if (region.leftmost) {
					color = Qt::magenta;
				} else if (region.rightmost) {
					color = Qt::cyan;
				} else {
					color = Qt::yellow;
				}
				painter.setBrush(color);
				QRectF rect(0, 0, 15, 15);
				rect.moveCenter(region.centroid);
				painter.drawEllipse(rect);
			}
		}
		dbg->add(canvas, "refined_graph");
	}

	edgeSequencesToPolylines(edge_node_paths, edge_nodes, regions, out);
}

void
TextLineTracer::labelAndGrowRegions(
	GrayImage const& blurred, BinaryImage region_seeds,
	BinaryImage const& thick_mask, std::vector<Region>& regions,
	std::set<Edge>& edges, DebugImages* dbg)
{
	int const width = blurred.width();
	int const height = blurred.height();

	BinaryImage eroded(erodeBrick(region_seeds, QSize(3, 3)));
	rasterOp<RopXor<RopSrc, RopDst> >(region_seeds, eroded.release());
	if (dbg) {
		dbg->add(region_seeds, "region_seed_edges");
	}

	Grid<GridNode> grid(width, height, /*padding=*/1);
	grid.initPadding(GridNode(0, 0, 1));
	
	GridNode* const grid_data = grid.data();
	int const grid_stride = grid.stride();
	
	RegionGrowingQueue queue(grid.data());

	ConnectivityMap cmap(region_seeds, CONN8);

	std::vector<CentroidCalculator> centroid_calculators(cmap.maxLabel());

	int const cmap_stride = cmap.stride();
	uint32_t* cmap_line = cmap.paddedData();

	uint8_t const* blurred_line = blurred.data();
	int const blurred_stride = blurred.stride();

	// Populate the grid from connectivity map, also find region centroids.
	int grid_offset = 0;
	for (int y = 0; y < height; ++y) {
		for (int x = 0; x < width; ++x, ++grid_offset) {
			GridNode* node = grid_data + grid_offset;
			node->setGrayLevel(blurred_line[x]);
			
			uint32_t const label = cmap_line[x];
			
			if (label == 0) {
				node->setRegionIdx(GridNode::INVALID_REGION_IDX);
				node->setFinalized(0);
			} else {
				// Label 0 stands for background in ConnectivityMap.
				node->setRegionIdx(label - 1);
				node->setFinalized(1);
				queue.push(RegionGrowingPosition(grid_offset, 0));
				
				centroid_calculators[label - 1].processSample(x, y);
			}
		}

		blurred_line += blurred_stride;
		cmap_line += cmap_stride;
		grid_offset += 2;
	}

	cmap = ConnectivityMap(); // Save memory.

	int const nbh_offsets[] = { -grid_stride, -1, 1, grid_stride };

	uint32_t iteration = 0;
	while (!queue.empty()) {
		++iteration;

		int const offset = queue.front().gridOffset;
		queue.pop();

		GridNode const* node = grid_data + offset;
		RegionIdx const region_idx = node->regionIdx();

		// Spread this value to 4-connected neighbours.
		for (int i = 0; i < 4; ++i) {
			int const nbh_offset = offset + nbh_offsets[i];
			GridNode* nbh = grid_data + nbh_offset;
			if (!nbh->finalized()) {
				nbh->setFinalized(1);
				nbh->setRegionIdx(region_idx);
				queue.push(RegionGrowingPosition(nbh_offset, iteration));
			}
		}
	}

	uint32_t const num_regions = centroid_calculators.size();
	regions.resize(num_regions);
	
	for (uint32_t i = 0; i < num_regions; ++i) {
		Region& region = regions[i];
		region.centroid = centroid_calculators[i].centroid();
	}

	// Mark regions as leftmost / rightmost.
	GridNode const* grid_line = grid.data();
	for (int y = 0; y < height; ++y, grid_line += grid_stride) {
		regions[grid_line[0].regionIdx()].leftmost = true;
		regions[grid_line[width - 1].regionIdx()].rightmost = true;
	}
	
	// Process horizontal connections between regions.
	grid_line = grid.data();
	uint32_t const* thick_mask_line = thick_mask.data();
	int const thick_mask_stride = thick_mask.wordsPerLine();
	for (int y = 0; y < height; ++y) {
		for (int x = 1; x < width; ++x) {
			uint32_t const mask1 = thick_mask_line[x >> 5] >> (31 - (x & 31));
			uint32_t const mask2 = thick_mask_line[(x - 1) >> 5] >> (31 - ((x - 1) & 31));
			if (mask1 & mask2 & 1) {
				GridNode const* node1 = grid_line + x;
				GridNode const* node2 = node1 - 1;
				if (node1->regionIdx() != node2->regionIdx()) {
					edges.insert(Edge(node1->regionIdx(), node2->regionIdx()));
				}
			}
		}

		grid_line += grid_stride;
		thick_mask_line += thick_mask_stride;
	}

	uint32_t const msb = uint32_t(1) << 31;

	// Process vertical connections between regions.
	grid_line = grid.data();
	thick_mask_line = thick_mask.data();
	for (int x = 0; x < width; ++x) {
		grid_line = grid.data() + x;
		uint32_t const* mask_word = thick_mask_line + (x >> 5);
		uint32_t const mask = msb >> (x & 31);
		
		for (int y = 1; y < height; ++y) {
			grid_line += grid_stride;
			mask_word += thick_mask_stride;

			if (mask_word[0] & mask_word[-thick_mask_stride] & mask) {
				GridNode const* node1 = grid_line;
				GridNode const* node2 = grid_line - grid_stride;
				if (node1->regionIdx() != node2->regionIdx()) {
					edges.insert(Edge(node1->regionIdx(), node2->regionIdx()));
				}
			}
		}
	}

	if (dbg) {
		// Visualize regions and seeds.
		QImage visualized_regions(
			visualizeRegions(grid).convertToFormat(QImage::Format_ARGB32_Premultiplied)
		);

		QImage canvas(visualized_regions);
		{
			QPainter painter(&canvas);

			painter.setOpacity(0.7);
			painter.drawImage(0, 0, blurred);

			painter.setOpacity(1.0);
			painter.drawImage(0, 0, region_seeds.toAlphaMask(Qt::blue));
		}
		dbg->add(canvas, "regions");

		// Visualize region connectivity.
		canvas = visualized_regions;
		visualized_regions = QImage();
		{
			QPainter painter(&canvas);
			painter.setOpacity(0.3);
			painter.drawImage(0, 0, thick_mask.toQImage());
			
			// Visualize connections.
			painter.setOpacity(1.0);
			painter.setRenderHint(QPainter::Antialiasing);
			QPen pen(Qt::blue);
			pen.setWidthF(2.0);
			painter.setPen(pen);
			BOOST_FOREACH(Edge const& edge, edges) {
				painter.drawLine(
					regions[edge.lesserRegionIdx].centroid,
					regions[edge.greaterRegionIdx].centroid
				);
			}

			// Visualize nodes.
			painter.setPen(Qt::NoPen);
			QColor color;
			BOOST_FOREACH(Region const& region, regions) {
				if (region.leftmost && region.rightmost) {
					color = Qt::green;
				} else if (region.leftmost) {
					color = Qt::magenta;
				} else if (region.rightmost) {
					color = Qt::cyan;
				} else {
					color = Qt::yellow;
				}
				painter.setBrush(color);
				QRectF rect(0, 0, 15, 15);
				rect.moveCenter(region.centroid);
				painter.drawEllipse(rect);
			}
		}
		dbg->add(canvas, "connectivity");
	}
}

void
TextLineTracer::extractEdegeNodePaths(
	std::vector<std::vector<uint32_t> >& edge_node_paths,
	std::vector<EdgeNode> const& edge_nodes,
	std::vector<Region> const& regions)
{
	uint32_t const num_edge_nodes = edge_nodes.size();

	std::map<RegionIdx, EdgeNodeIdx> best_incoming_paths; // rightmost region -> rightmost EdgeNode index
	
	for (uint32_t rightmost_edge_node_idx = 0; rightmost_edge_node_idx < num_edge_nodes; ++rightmost_edge_node_idx) {
		EdgeNode const& edge_node = edge_nodes[rightmost_edge_node_idx];
		
		uint32_t rightmost_region_idx;

		if (regions[edge_node.edge.lesserRegionIdx].rightmost) {
			rightmost_region_idx = edge_node.edge.lesserRegionIdx;
		} else if (regions[edge_node.edge.greaterRegionIdx].rightmost) {
			rightmost_region_idx = edge_node.edge.greaterRegionIdx;
		} else {
			continue;
		}

		uint32_t const leftmost_region_idx = edge_node.leftmostRegionIdx;
		if (leftmost_region_idx == ~RegionIdx(0)) {
			// No path reached this node.
			continue; 
		}

		std::map<RegionIdx, EdgeNodeIdx>::iterator it(best_incoming_paths.find(rightmost_region_idx));
		if (it == best_incoming_paths.end()) {
			best_incoming_paths[rightmost_region_idx] = rightmost_edge_node_idx;
		} else {
			float const old_cost = edge_nodes[it->second].pathCost;
			float const new_cost = edge_nodes[rightmost_edge_node_idx].pathCost;
			if (new_cost < old_cost) {
				it->second = rightmost_edge_node_idx;
			}
		}
	}

	std::map<RegionIdx, EdgeNodeIdx> best_outgoing_paths; // leftmost region -> rightmost EdgeNode index
	
	typedef std::map<RegionIdx, EdgeNodeIdx>::value_type KV;
	BOOST_FOREACH(KV const& kv, best_incoming_paths) {
		uint32_t const leftmost_region_idx = edge_nodes[kv.second].leftmostRegionIdx;
		uint32_t const rightmost_edge_node_idx = kv.second;

		std::map<RegionIdx, EdgeNodeIdx>::iterator it(
			best_outgoing_paths.find(leftmost_region_idx)
		);
		if (it == best_outgoing_paths.end()) {
			best_outgoing_paths[leftmost_region_idx] = rightmost_edge_node_idx;
		} else {
			float const existing_cost = edge_nodes[it->second].pathCost;
			float const new_cost = edge_nodes[rightmost_edge_node_idx].pathCost;
			if (new_cost < existing_cost) {
				it->second = rightmost_edge_node_idx;
			}
		}
	}
	
	// Follow by EdgeNode::prevEdgeNode from rightmost edges to leftmost ones.
	typedef std::map<RegionIdx, EdgeNodeIdx>::value_type LR;
	BOOST_FOREACH (LR const& lr, best_outgoing_paths) {
	
		edge_node_paths.push_back(std::vector<uint32_t>());
		std::vector<uint32_t>& path = edge_node_paths.back();
		
		uint32_t edge_node_idx = lr.second;
		for (;;) {
			path.push_back(edge_node_idx);

			EdgeNode const& edge_node = edge_nodes[edge_node_idx];
			if (edge_node.edge.lesserRegionIdx == lr.first || edge_node.edge.greaterRegionIdx == lr.first) {
				break; // We are done!
			}

			edge_node_idx = edge_node.prevEdgeNodeIdx;
		}
	}
}

void
TextLineTracer::edgeSequencesToPolylines(
	std::vector<std::vector<EdgeNodeIdx> > const& edge_node_paths,
	std::vector<EdgeNode> const& edge_nodes, std::vector<Region> const& regions,
	std::list<std::vector<QPointF> >& polylines)
{
	BOOST_FOREACH(std::vector<EdgeNodeIdx> const& edge_node_path, edge_node_paths) {
		if (edge_node_path.empty()) {
			continue;
		}
		
		polylines.push_back(std::vector<QPointF>());
		std::vector<QPointF>& polyline = polylines.back();

		if (edge_node_path.size() == 1) {
			Edge const& edge = edge_nodes[edge_node_path.front()].edge;
			polyline.push_back(regions[edge.lesserRegionIdx].centroid);
			polyline.push_back(regions[edge.greaterRegionIdx].centroid);
			continue;
		}

		std::vector<RegionIdx> region_indexes;

		// This one will be written later.
		region_indexes.push_back(0);

		std::vector<EdgeNodeIdx>::const_iterator it(edge_node_path.begin());
		std::vector<EdgeNodeIdx>::const_iterator const end(edge_node_path.end());
		for (;;) {
			EdgeNodeIdx const edge_node1_idx = *it;
			++it;
			if (it == end) {
				break;
			}
			EdgeNodeIdx const edge_node2_idx = *it;
			
			RegionIdx const connecting_region_idx = findConnectingRegion(
				edge_nodes[edge_node1_idx].edge, edge_nodes[edge_node2_idx].edge
			);
			assert(connecting_region_idx != ~RegionIdx(0));

			region_indexes.push_back(connecting_region_idx);
		}

		Edge const& first_edge = edge_nodes[edge_node_path.front()].edge;
		if (first_edge.lesserRegionIdx == region_indexes[1]) {
			region_indexes[0] = first_edge.greaterRegionIdx;
		} else {
			region_indexes[0] = first_edge.lesserRegionIdx;
		}

		Edge const& last_edge = edge_nodes[edge_node_path.back()].edge;
		if (last_edge.lesserRegionIdx == region_indexes.back()) {
			region_indexes.push_back(last_edge.greaterRegionIdx);
		} else {
			region_indexes.push_back(last_edge.lesserRegionIdx);
		}

		BOOST_FOREACH(RegionIdx region_idx, region_indexes) {
			polyline.push_back(regions[region_idx].centroid);
		}
	}
}

TextLineTracer::RegionIdx
TextLineTracer::findConnectingRegion(Edge const& edge1, Edge const& edge2)
{
	RegionIdx const edge1_regions[] = { edge1.lesserRegionIdx, edge1.greaterRegionIdx };
	RegionIdx const edge2_regions[] = { edge2.lesserRegionIdx, edge2.greaterRegionIdx };

	BOOST_FOREACH(RegionIdx idx1, edge1_regions) {
		BOOST_FOREACH(RegionIdx idx2, edge2_regions) {
			if (idx1 == idx2) {
				return idx1;
			}
		}
	}

	return ~RegionIdx(0);
}

void
TextLineTracer::sanitizeBinaryImage(BinaryImage& image, QRect const& content_rect)
{
	// Kill connected components touching the borders.
	BinaryImage seed(image.size(), WHITE);
	seed.fillExcept(seed.rect().adjusted(1, 1, -1, -1), BLACK);

	BinaryImage touching_border(seedFill(seed.release(), image, CONN8));
	rasterOp<RopSubtract<RopDst, RopSrc> >(image, touching_border.release());

	// Poor man's despeckle.
	BinaryImage content_seeds(openBrick(image, QSize(2, 3), WHITE));
	rasterOp<RopOr<RopSrc, RopDst> >(content_seeds, openBrick(image, QSize(3, 2), WHITE));
	image = seedFill(content_seeds.release(), image, CONN8);

	// Clear margins.
	image.fillExcept(content_rect, WHITE);
}

/**
 * Returns false if the curve contains both significant convexities and concavities.
 */
bool
TextLineTracer::isCurvatureConsistent(std::vector<QPointF> const& polyline)
{
	size_t const num_nodes = polyline.size();
	
	if (num_nodes <= 1) {
		// Even though we can't say anything about curvature in this case,
		// we don't like such gegenerate curves, so we reject them.
		return false;
	} else if (num_nodes == 2) {
		// These are fine.
		return true;
	}

	// Threshold angle between a polyline segment and a normal to the previous one.
	float const cos_threshold = cos((90.0f - 6.0f) * constants::DEG2RAD);
	float const cos_sq_threshold = cos_threshold * cos_threshold;
	bool significant_positive = false;
	bool significant_negative = false;

	Vec2f prev_normal(polyline[1] - polyline[0]);
	std::swap(prev_normal[0], prev_normal[1]);
	prev_normal[0] = -prev_normal[0];
	float prev_normal_sqlen = prev_normal.squaredNorm();

	for (size_t i = 1; i < num_nodes - 1; ++i) {
		Vec2f const next_segment(polyline[i + 1] - polyline[i]);
		float const next_segment_sqlen = next_segment.squaredNorm();

		float cos_sq = 0;
		float const sqlen_mult = prev_normal_sqlen * next_segment_sqlen;
		if (sqlen_mult > std::numeric_limits<float>::epsilon()) {
			float const dot = prev_normal.dot(next_segment);
			cos_sq = fabs(dot) * dot / sqlen_mult;
		}

		if (fabs(cos_sq) >= cos_sq_threshold) {
			if (cos_sq > 0) {
				significant_positive = true;
			} else {
				significant_negative = true;
			}
		}

		prev_normal[0] = -next_segment[1];
		prev_normal[1] = next_segment[0];
		prev_normal_sqlen = next_segment_sqlen;
	}

	return !(significant_positive && significant_positive);
}

bool
TextLineTracer::isInsideBounds(
	QPointF const& pt, QLineF const& left_bound, QLineF const& right_bound)
{
	QPointF left_normal_inside(left_bound.normalVector().p2() - left_bound.p1());
	if (left_normal_inside.x() < 0) {
		left_normal_inside = -left_normal_inside;
	}
	QPointF const left_vec(pt - left_bound.p1());
	if (left_normal_inside.x() * left_vec.x() + left_normal_inside.y() * left_vec.y() < 0) {
		return false;
	}

	QPointF right_normal_inside(right_bound.normalVector().p2() - right_bound.p1());
	if (right_normal_inside.x() > 0) {
		right_normal_inside = -right_normal_inside;
	}
	QPointF const right_vec(pt - right_bound.p1());
	if (right_normal_inside.x() * right_vec.x() + right_normal_inside.y() * right_vec.y() < 0) {
		return false;
	}

	return true;
}

void
TextLineTracer::filterOutOfBoundsCurves(
	std::list<std::vector<QPointF> >& polylines,
	QLineF const& left_bound, QLineF const& right_bound)
{
	std::list<std::vector<QPointF> >::iterator it(polylines.begin());
	std::list<std::vector<QPointF> >::iterator const end(polylines.end());	
	while (it != end) {
		if (!isInsideBounds(it->front(), left_bound, right_bound) &&
			!isInsideBounds(it->back(), left_bound, right_bound)) {
			polylines.erase(it++);
		} else {
			++it;
		}
	}
}

void
TextLineTracer::filterEdgyCurves(std::list<std::vector<QPointF> >& polylines)
{
	std::list<std::vector<QPointF> >::iterator it(polylines.begin());
	std::list<std::vector<QPointF> >::iterator const end(polylines.end());	
	while (it != end) {
		if (!isCurvatureConsistent(*it)) {
			polylines.erase(it++);
		} else {
			++it;
		}
	}
}

struct TextLineTracer::TracedCurve
{
	std::vector<QPointF> polyline;
	std::vector<QPointF> extendedPolyline;
	double yCenter;

	TracedCurve(std::vector<QPointF> const& polyline,
		std::vector<QPointF> const& extended_polyline, double y_center)
		: polyline(polyline), extendedPolyline(extended_polyline), yCenter(y_center) {}

	bool operator<(TracedCurve const& rhs) const { return yCenter < rhs.yCenter; }
};

struct TextLineTracer::RansacModel
{
	TracedCurve const* topCurve;
	TracedCurve const* bottomCurve; 
	double totalError;

	RansacModel() : topCurve(0), bottomCurve(0), totalError(NumericTraits<double>::max()) {}

	bool isValid() const { return topCurve && bottomCurve; }
};

class TextLineTracer::RansacAlgo
{
public:
	RansacAlgo(std::vector<TracedCurve> const& all_curves)
		: m_rAllCurves(all_curves) {}

	void buildAndAssessModel(
		TracedCurve const* top_curve, TracedCurve const* bottom_curve, double scale);

	RansacModel& bestModel() { return m_bestModel; }

	RansacModel const& bestModel() const { return m_bestModel; }
private:
	RansacModel m_bestModel;
	std::vector<TracedCurve> const& m_rAllCurves;
};

void
TextLineTracer::RansacAlgo::buildAndAssessModel(
	TracedCurve const* top_curve, TracedCurve const* bottom_curve, double scale)
try {
	using namespace output;

	DistortionModel model;
	model.setTopCurve(Curve(top_curve->extendedPolyline));
	model.setBottomCurve(Curve(bottom_curve->extendedPolyline));
	if (!model.isValid()) {
		return;
	}

	double const depth_perception = 2.0; // Doesn't matter much here.
	CylindricalSurfaceDewarper const dewarper(
		top_curve->extendedPolyline, bottom_curve->extendedPolyline, depth_perception
	);

	double error = 0;
	BOOST_FOREACH(TracedCurve const& curve, m_rAllCurves) {
		double mean = 0;
		double sqmean = 0;
		BOOST_FOREACH(QPointF const& pt, curve.polyline) {
			// TODO: add another signature with hint for efficiency.
			double const y = dewarper.mapToDewarpedSpace(pt).y() * scale;
			mean += y;
			sqmean += y * y;
		}
		mean /= curve.polyline.size();
		sqmean /= curve.polyline.size();
		double const stddev = sqrt(sqmean - mean * mean);
		error += stddev;
	}

	if (error < m_bestModel.totalError) {
		m_bestModel.topCurve = top_curve;
		m_bestModel.bottomCurve = bottom_curve;
		m_bestModel.totalError = error;
	}
} catch (std::runtime_error const&) {
	// Probably CylindricalSurfaceDewarper didn't like something.
}

void
TextLineTracer::pickRepresentativeLines(
	std::list<std::vector<QPointF> >& polylines, QRectF const& image_rect,
	QLineF const& left_bound, QLineF const& right_bound)
{
	int const num_curves = polylines.size();

	if (num_curves < 2) {
		polylines.clear();
		return;
	}

	std::vector<TracedCurve> ordered_curves;
	ordered_curves.reserve(num_curves);

	// First, sort them top to bottom.
	// For this purpose, calculate where they intersect this line:
	QLineF const mid_vert_line(
		0.5 * (image_rect.topLeft() + image_rect.topRight()),
		0.5 * (image_rect.bottomLeft() + image_rect.bottomRight())
	);
	BOOST_FOREACH(std::vector<QPointF> const& polyline, polylines) {
		std::vector<QPointF> extended_polyline(polyline);
		intersectWithVerticalBoundaries(extended_polyline, left_bound, right_bound);
		
		PolylineIntersector const intersector(polyline);
		PolylineIntersector::Hint hint;
		QPointF const intersection(intersector.intersect(mid_vert_line, hint));

		ordered_curves.push_back(TracedCurve(polyline, extended_polyline, intersection.y()));
	}
	std::sort(ordered_curves.begin(), ordered_curves.end());

	// Select the best pair using RANSAC.
	RansacAlgo ransac(ordered_curves);

	// First let's try to combine each of the 3 top-most lines
	// with each of the 3 bottom-most ones.
	for (int i = 0; i < std::min<int>(3, num_curves); ++i) {
		for (int j = std::max<int>(0, num_curves - 3); j < num_curves; ++j) {
			if (i < j) {
				double const scale = ordered_curves[j].yCenter - ordered_curves[i].yCenter + 1;
				// The +1 thing just prevents it from being zero.
				assert(scale > 0);
				ransac.buildAndAssessModel(&ordered_curves[i], &ordered_curves[j], scale);
			}
		}
	}

	// Continue by throwing in some random pairs of lines.
	qsrand(0); // Repeatablity is important.
	int random_pairs_remaining = 10;
	while (random_pairs_remaining-- > 0) {
		int i = qrand() % num_curves;
		int j = qrand() % num_curves;
		if (i > j) {
			std::swap(i, j);
		}
		if (i < j) {
			double const scale = ordered_curves[j].yCenter - ordered_curves[i].yCenter + 1;
			// The +1 thing just prevents it from being zero.
			assert(scale > 0);
			ransac.buildAndAssessModel(&ordered_curves[i], &ordered_curves[j], scale);
		}
	}
	
	if (ransac.bestModel().isValid()) {
		std::list<std::vector<QPointF> > new_polylines;
		new_polylines.push_back(ransac.bestModel().topCurve->extendedPolyline);
		new_polylines.push_back(ransac.bestModel().bottomCurve->extendedPolyline);
		polylines.swap(new_polylines);
	}
}

void
TextLineTracer::makeLeftToRight(std::vector<QPointF>& polyline)
{
	assert(polyline.size() >= 2);

	if (polyline.front().x() > polyline.back().x()) {
		std::reverse(polyline.begin(), polyline.end());
	}
}

void
TextLineTracer::extendOrTrimPolyline(
	std::vector<QPointF>& polyline, QLineF const& left_bound, QLineF const right_bound,
	BinaryImage const& content, GrayImage const& blurred, BinaryImage const& thick_mask)
{
	assert(polyline.size() >= 2);

	QLineF front_bound(left_bound);
	QLineF back_bound(right_bound);
	if (polyline.front().x() > polyline.back().x()) {
		std::swap(front_bound, back_bound);
	}

	std::deque<QPointF> new_polyline(polyline.begin(), polyline.end());

	// Note that the reason we do trimming is to dodge the possibility
	// a snake would be attracted to some garbage on the other side of the line.

	if (!trimFront(new_polyline, front_bound)) {
		growFront(new_polyline, front_bound, content, blurred, thick_mask);
	}
	
	if (!trimBack(new_polyline, back_bound)) {
		growBack(new_polyline, back_bound, content, blurred, thick_mask);
	}

	polyline.clear();
	polyline.insert(polyline.end(), new_polyline.begin(), new_polyline.end());
}

void
TextLineTracer::growFront(
	std::deque<QPointF>& polyline, QLineF const& bound,
	BinaryImage const& content, GrayImage const& blurred,
	BinaryImage const& thick_mask)
{
	TowardsLineTracer tracer(content, blurred, thick_mask, bound, polyline.front().toPoint());
	while (QPoint const* pt = tracer.trace(40)) { // XXX: hardcoded constant
		polyline.push_front(*pt);
	}
}

void
TextLineTracer::growBack(
	std::deque<QPointF>& polyline, QLineF const& bound,
	BinaryImage const& content, GrayImage const& blurred,
	BinaryImage const& thick_mask)
{
	TowardsLineTracer tracer(content, blurred, thick_mask, bound, polyline.back().toPoint());
	while (QPoint const* pt = tracer.trace(40)) { // XXX: hardcoded constant
		polyline.push_back(*pt);
	}
}

bool
TextLineTracer::trimFront(std::deque<QPointF>& polyline, QLineF const& bound)
{
	if (sidesOfLine(bound, polyline.front(), polyline.back()) >= 0) {
		// Doesn't need trimming.
		return false;
	}

	while (polyline.size() > 2 && sidesOfLine(bound, polyline.front(), polyline[1]) > 0) {
		polyline.pop_front();
	}
	
	intersectFront(polyline, bound);
	
	return true;
}

bool
TextLineTracer::trimBack(std::deque<QPointF>& polyline, QLineF const& bound)
{
	if (sidesOfLine(bound, polyline.front(), polyline.back()) >= 0) {
		// Doesn't need trimming.
		return false;
	}

	while (polyline.size() > 2 && sidesOfLine(bound, polyline[polyline.size() - 2], polyline.back()) > 0) {
		polyline.pop_back();
	}
	
	intersectBack(polyline, bound);
	
	return true;
}

void
TextLineTracer::intersectFront(
	std::deque<QPointF>& polyline, QLineF const& bound)
{
	assert(polyline.size() >= 2);

	QLineF const front_segment(polyline.front(), polyline[1]);
	QPointF intersection;
	if (bound.intersect(front_segment, &intersection) != QLineF::NoIntersection) {
		polyline.front() = intersection;
	}
}

void
TextLineTracer::intersectBack(
	std::deque<QPointF>& polyline, QLineF const& bound)
{
	assert(polyline.size() >= 2);

	QLineF const back_segment(polyline[polyline.size() - 2], polyline.back());
	QPointF intersection;
	if (bound.intersect(back_segment, &intersection) != QLineF::NoIntersection) {
		polyline.back() = intersection;
	}
}

void
TextLineTracer::intersectWithVerticalBoundaries(
	std::vector<QPointF>& polyline, QLineF const& left_bound, QLineF const& right_bound)
{
	assert(polyline.size() >= 2);

	QLineF front_bound(left_bound);
	QLineF back_bound(right_bound);
	if (polyline.front().x() > polyline.back().x()) {
		std::swap(front_bound, back_bound);
	}

	std::deque<QPointF> new_polyline(polyline.begin(), polyline.end());

	if (!trimFront(new_polyline, front_bound)) {
		intersectFront(new_polyline, front_bound);
	}

	if (!trimBack(new_polyline, back_bound)) {
		intersectBack(new_polyline, back_bound);
	}

	polyline.clear();
	polyline.insert(polyline.end(), new_polyline.begin(), new_polyline.end());
}

QImage
TextLineTracer::visualizeVerticalBounds(
	QImage const& background, std::pair<QLineF, QLineF> const& bounds)
{
	QImage canvas(background.convertToFormat(QImage::Format_RGB32));

	QPainter painter(&canvas);
	painter.setRenderHint(QPainter::Antialiasing);
	QPen pen(Qt::blue);
	pen.setWidthF(2.0);
	painter.setPen(pen);
	painter.setOpacity(0.7);

	painter.drawLine(bounds.first);
	painter.drawLine(bounds.second);

	return canvas;
}

QImage
TextLineTracer::visualizeRegions(Grid<GridNode> const& grid)
{
	int const width = grid.width();
	int const height = grid.height();

	GridNode const* grid_line = grid.data();
	int const grid_stride = grid.stride();

	QImage canvas(width, height, QImage::Format_ARGB32_Premultiplied);
	uint32_t* canvas_line = (uint32_t*)canvas.bits();
	int const canvas_stride = canvas.bytesPerLine() / 4;

	for (int y = 0; y < height; ++y) {
		for (int x = 0; x < width; ++x) {
			uint32_t const region_idx = grid_line[x].regionIdx();
			if (region_idx == GridNode::INVALID_REGION_IDX) {
				canvas_line[x] = 0; // transparent
			} else {
				canvas_line[x] = colorForId(region_idx).rgba();
			}
		}
		grid_line += grid_stride;
		canvas_line += canvas_stride;
	}

	return canvas;
}

QImage
TextLineTracer::visualizePolylines(
	QImage const& background, std::list<std::vector<QPointF> > const& polylines,
	std::pair<QLineF, QLineF> const* vert_bounds)
{
	QImage canvas(background.convertToFormat(QImage::Format_ARGB32_Premultiplied));
	QPainter painter(&canvas);
	painter.setRenderHint(QPainter::Antialiasing);
	QPen pen(Qt::blue);
	pen.setWidthF(3.0);
	painter.setPen(pen);

	BOOST_FOREACH(std::vector<QPointF> const& polyline, polylines) {
		if (!polyline.empty()) {
			painter.drawPolyline(&polyline[0], polyline.size());
		}
	}

	if (vert_bounds) {
		painter.drawLine(vert_bounds->first);
		painter.drawLine(vert_bounds->second);
	}

	return canvas;
}

QImage
TextLineTracer::visualizeExtendedPolylines(
	QImage const& blurred, imageproc::BinaryImage const&  thick_mask,
	std::list<std::vector<QPointF> > const& polylines,
	QLineF const& left_bound, QLineF const& right_bound)
{
	QImage canvas(blurred.convertToFormat(QImage::Format_ARGB32_Premultiplied));
	QPainter painter(&canvas);
	painter.drawImage(0, 0, thick_mask.toAlphaMask(QColor(255, 0, 0, 20)));
	
	painter.setRenderHint(QPainter::Antialiasing);

	// Draw polylines.
	QPen pen(Qt::blue);
	pen.setWidthF(3.0);
	painter.setPen(pen);
	BOOST_FOREACH(std::vector<QPointF> const& polyline, polylines) {
		if (!polyline.empty()) {
			painter.drawPolyline(&polyline[0], polyline.size());
		}
	}

	// Draw vertical bounds.
	painter.drawLine(left_bound);
	painter.drawLine(right_bound);

	// Draw polyline nodes.
	painter.setPen(Qt::NoPen);
	QBrush brush(Qt::yellow);
	painter.setBrush(brush);
	painter.setOpacity(0.7);
	QRectF rect(0, 0, 15, 15);
	BOOST_FOREACH(std::vector<QPointF> const& polyline, polylines) {
		BOOST_FOREACH(QPointF const& pt, polyline) {
			rect.moveCenter(pt);
			painter.drawEllipse(rect);
		}
	}

	return canvas;
}
