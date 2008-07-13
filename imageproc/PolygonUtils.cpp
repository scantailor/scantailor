/*
    Scan Tailor - Interactive post-processing tool for scanned pages.
    Copyright (C) 2007-2008  Joseph Artsimovich <joseph_a@mail.ru>

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

#include "PolygonUtils.h"
#include <QPolygonF>
#include <QPointF>
#include <QLineF>
#include <boost/foreach.hpp>
#include <algorithm>
#include <math.h>
#include <assert.h>

namespace imageproc
{

class PolygonUtils::Before
{
public:
	bool operator()(QPointF const& lhs, QPointF const& rhs) const {
		if (lhs.x() < rhs.x()) {
			return true;
		} else if (rhs.x() < lhs.x()) {
			return false;
		} else {
			return lhs.y() < rhs.y();
		}
	}
	
	bool operator()(QLineF const& lhs, QLineF const& rhs) {
		return operator()(lhs.p1(), rhs.p1());
	}
};


QPolygonF
PolygonUtils::round(QPolygonF const& poly)
{
	QPolygonF rounded;
	rounded.reserve(poly.size());
	
	BOOST_FOREACH (QPointF const& p, poly) {
		rounded.push_back(roundPoint(p));
	}
	
	return rounded;
}

bool
PolygonUtils::fuzzyCompare(QPolygonF const& poly1, QPolygonF const& poly2)
{
	if (poly1.size() < 2 && poly2.size() < 2) {
		return true;
	} else if (poly1.size() < 2 || poly2.size() < 2) {
		return false;
	}
	
	assert(poly1.size() >= 2 && poly2.size() >= 2);
	
	QPolygonF rounded1(round(poly1));
	QPolygonF rounded2(round(poly2));
	
	// Close if necessary.
	if (rounded1.back() != rounded1.front()) {
		rounded1.push_back(rounded1.front());
	}
	if (rounded2.back() != rounded2.front()) {
		rounded2.push_back(rounded2.front());
	}
	
	std::vector<QLineF> edges1(extractAndNormalizeEdges(rounded1));
	std::vector<QLineF> edges2(extractAndNormalizeEdges(rounded2));
	
	if (edges1.size() != edges2.size()) {
		return false;
	}
	
	std::sort(edges1.begin(), edges1.end(), Before());
	std::sort(edges2.begin(), edges2.end(), Before());
	
	return edges1 == edges2;
}

QPointF
PolygonUtils::roundPoint(QPointF const& p)
{
	return QPointF(roundValue(p.x()), roundValue(p.y()));
}

double
PolygonUtils::roundValue(double const val)
{
	double const multiplier = 1 << 14;
	double const r_multiplier = 1.0 / multiplier;
	return floor(val * multiplier + 0.5) * r_multiplier;
}

std::vector<QLineF>
PolygonUtils::extractAndNormalizeEdges(QPolygonF const& poly)
{
	std::vector<QLineF> edges;
	
	int const num_edges = poly.size();
	if (num_edges > 1) {
		for (int i = 1; i < num_edges; ++i) {
			maybeAddNormalizedEdge(edges, poly[i - 1], poly[i]);
		}
		maybeAddNormalizedEdge(edges, poly[num_edges - 1], poly[0]);
	}
	
	return edges;
}

void
PolygonUtils::maybeAddNormalizedEdge(
	std::vector<QLineF>& edges, QPointF const& p1, QPointF const& p2)
{
	if (p1 == p2) {
		return;
	}
	
	if (Before()(p2, p1)) {
		edges.push_back(QLineF(p2, p1));
	} else {
		edges.push_back(QLineF(p1, p2));
	}
}

} // namespace imageproc
