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

#ifndef IMAGEPROC_POLYGONUTILS_H_
#define IMAGEPROC_POLYGONUTILS_H_

#include <vector>

class QPolygonF;
class QPointF;
class QLineF;

namespace imageproc
{

class PolygonUtils
{
public:
	/**
	 * \brief Adjust vertices to more round coordinates.
	 *
	 * This method exists to workaround bugs in QPainterPath and QPolygonF
	 * composition operations.  It turns out rounding vertex coordinates
	 * solves many of those bugs.  We don't round to integer values, we
	 * only make very minor adjustments.
	 */
	static QPolygonF round(QPolygonF const& poly);
	
	/**
	 * \brief Test if two polygons are logically equal.
	 *
	 * By logical equality we mean that the following differences don't matter:
	 * \li Direction (clockwise vs counter-clockwise).
	 * \li Closed vs unclosed.
	 * \li Tiny differences in vertex coordinates.
	 *
	 * \return true if polygons are logically equal, false otherwise.
	 */
	static bool fuzzyCompare(QPolygonF const& poly1, QPolygonF const& poly2);

	static QPolygonF convexHull(std::vector<QPointF> point_cloud);
private:
	class Before;
	
	static QPointF roundPoint(QPointF const& p);
	
	static double roundValue(double val);
	
	static std::vector<QLineF> extractAndNormalizeEdges(QPolygonF const& poly);
	
	static void maybeAddNormalizedEdge(
		std::vector<QLineF>& edges, QPointF const& p1, QPointF const& p2);
	
	static bool fuzzyCompareImpl(
		std::vector<QLineF> const& lines1,
		std::vector<QLineF> const& lines2);
	
	static bool fuzzyCompareImpl(QLineF const& line1, QLineF const& line2);
	
	static bool fuzzyCompareImpl(QPointF const& p1, QPointF const& p2);
	
	static double const ROUNDING_MULTIPLIER;
	static double const ROUNDING_RECIP_MULTIPLIER;
};

} // namespace imageproc

#endif
