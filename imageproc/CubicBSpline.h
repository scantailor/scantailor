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

#ifndef IMAGEPROC_CUBIC_BSPLINE_H_
#define IMAGEPROC_CUBIC_BSPLINE_H_

#include <boost/array.hpp>
#include <QPointF>
#include <QLineF>
#include <vector>
#include <stddef.h>

namespace imageproc
{

class CubicBSpline
{
public:
	CubicBSpline();

	CubicBSpline(std::vector<QPointF> const& points);

	/**
	 * \brief Returns true if the spline has enough points for at least
	 *        one bezier segment.
	 */
	bool isValid() const;

	/**
	 * \brief Adds additional points (if necessary) to make the spline
	 *        have at least one bezier segment.
	 */
	void makeValid();

	std::vector<QPointF> const& controlPoints() const { return m_points; }

	size_t numSegments() const;

	void appendControlPoint(QPointF const& pt);

	void moveControlPoint(size_t idx, QPointF const& pos);

	double maxT() const { return numSegments(); }

	QPointF eval(double t) const;

	QPointF pointClosestTo(QPointF to, double accuracy = 0.1, double* t = 0) const;

	boost::array<QPointF, 4> toBezierSegment(size_t segment) const;

	void setBezierSegment(size_t segment, boost::array<QPointF, 4> const& bezier);

	void moveBezierPoint(size_t segment, size_t bezier_idx, QPointF const& pos);

	std::vector<QPointF> toPolyline(double accuracy = 1.0) const;

	void swap(CubicBSpline& other) { m_points.swap(other.m_points); }
private:
	QPointF threePointEval(int base) const;

	void maybeInsertMorePoints(
		std::vector<QPointF>& polyline, double sq_accuracy,
		double prev_t, QPointF const& prev_pt,
		double next_t, QPointF const& next_pt) const;

	static double sqDistToLine(QPointF const& pt, QLineF const& line);

	std::vector<QPointF> m_points;
};


inline void swap(CubicBSpline& o1, CubicBSpline& o2)
{
	o1.swap(o2);	
}

} // namespace imageproc

#endif
