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
#include "imageproc/BinaryImage.h"
#include "imageproc/Connectivity.h"
#include "imageproc/ConnectivityMap.h"
#include "imageproc/InfluenceMap.h"
#include "imageproc/BitOps.h"
#include <QPoint>
#include <QSize>
#include <QRect>
#include <QImage>
#include <QPainter>
#include <QColor>
#include <QPen>
#include <QDebug>
#include <boost/foreach.hpp>
#include <vector>
#include <stack>
#include <set>
#include <algorithm>
#include <math.h>
#include <stdint.h>

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

	void extend(int x, int y) {
		top = std::min(top, y);
		left = std::min(left, x);
		bottom = std::max(bottom, y);
		right = std::max(right, x);
	}

	QPoint center() const { return QPoint(left + width()/2, top + height()/2); }
};

struct TextLineTracer::Component
{
	std::vector<uint32_t> connectedLabels;
	BoundingBox bbox;
	uint32_t groupLabel;

	Component() : groupLabel(0) {}
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
		
		//int const bbox1_center_y = bbox1.top + bbox1.height() / 2;
		//int const bbox2_center_y = bbox2.top + bbox2.height() / 2;

		//if (bbox1_center_y >= bbox2.top && bbox1_center_y <= bbox2.bottom
		//	&& bbox2_center_y >= bbox1.top && bbox2_center_y <= bbox1.bottom) {
		//	return true;
		//}

		if (bbox1.top > bbox2.bottom) {
			return false;
		}

		if (bbox2.top > bbox1.bottom) {
			return false;
		}

		return true;
	}
};

void
TextLineTracer::trace(
	imageproc::BinaryImage const& input,
	TaskStatus const& status, DebugImages* dbg)
{
	int const w = input.width();
	int const h = input.height();

	ConnectivityMap cmap(input, CONN8);
	uint32_t const* cmap_line = cmap.data();
	int const cmap_stride = cmap.stride();

	std::vector<Component> components(cmap.maxLabel() + 1);
	
	// Determine bounding boxes for each component.
	for (int y = 0; y < h; ++y, cmap_line += cmap_stride) {
		for (int x = 0; x < w; ++x) {
			uint32_t const label = cmap_line[x];
			if (label) {
				components[label].bbox.extend(x, y);
			}
		}
	}

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
		if (it->goodConnection(components)) {
			components[it->lesserLabel].connectedLabels.push_back(it->greaterLabel);
			components[it->greaterLabel].connectedLabels.push_back(it->lesserLabel);
			++it;
		} else {
			connections.erase(it++);
		}
	}

	// Assign group labels to connected groups of components.
	uint32_t nextGroupLabel = 0;
	for (uint32_t comp_label = 1; comp_label <= imap.maxLabel(); ++comp_label) {
		labelGroup(components, components[comp_label], ++nextGroupLabel);
	}

	if (dbg) {
		dbg->add(visualizeConnections(input.toQImage(), connections, components), "connections");
	}
}

void
TextLineTracer::labelGroup(std::vector<Component>& components, Component& comp, uint32_t label)
{
	if (comp.groupLabel) {
		// Already assigned a label.
		return;
	}

	comp.groupLabel = label;
	BOOST_FOREACH(uint32_t connected_label, comp.connectedLabels) {
		labelGroup(components, components[connected_label], label);
	}
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
TextLineTracer::visualizeConnections(
	QImage const& background, std::set<Connection> const& connections,
	std::vector<Component> const& components)
{
	QImage canvas(background.convertToFormat(QImage::Format_ARGB32_Premultiplied));
	QPainter painter(&canvas);
	painter.setRenderHint(QPainter::Antialiasing);

	QPen pen;
	pen.setWidthF(10.0);
	
	BOOST_FOREACH(Connection const& conn, connections) {
		uint32_t const val = components[conn.lesserLabel].groupLabel;
		int const bits_unused = countMostSignificantZeroes(val);
		uint32_t const reversed = reverseBits(val) >> bits_unused;
		uint32_t const mask = ~uint32_t(0) >> bits_unused;

		double const H = 0.99 * (double(reversed) / mask);
		double const S = 1.0;
		double const V = 1.0;
		QColor color;
		color.setHsvF(H, S, V, 0.7);
		pen.setColor(color);
		painter.setPen(pen);

		BoundingBox const& bbox1 = components[conn.lesserLabel].bbox;
		BoundingBox const& bbox2 = components[conn.greaterLabel].bbox;
		painter.drawLine(bbox1.center(), bbox2.center());
	}

	return canvas;
}
