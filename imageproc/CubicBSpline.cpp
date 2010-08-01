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

#include "CubicBSpline.h"
#include "ToLineProjector.h"
#include "VecNT.h"
#include <assert.h>

// The following reading is recommended:
// Thomas W. Sederberg, An Introduction to B-Spline Curves
// http://tom.cs.byu.edu/~455/bs.pdf

namespace imageproc
{

CubicBSpline::CubicBSpline()
{
}

CubicBSpline::CubicBSpline(std::vector<QPointF> const& points)
:	m_points(points)
{
}

bool
CubicBSpline::isValid() const
{
	return m_points.size() >= 4;
}

void
CubicBSpline::makeValid()
{
	m_points.reserve(4);

	switch (m_points.size()) {
		case 0:
			m_points.push_back(QPointF(0, 0));
			// fall through
		case 1:
			m_points.push_back(m_points.back());
			m_points.push_back(m_points.back());
			m_points.push_back(m_points.back());
			break;
		case 2: {
			QPointF pts[2];
			pts[0] = (2.0/3.0) * m_points.front() + (1.0/3.0) * m_points.back();
			pts[1] = (1.0/3.0) * m_points.front() + (2.0/3.0) * m_points.back();
			m_points.insert(m_points.begin() + 1, pts, pts + 2);
			break;
		}
		case 3: {
			std::vector<QPointF>::iterator it(m_points.begin());
			++it;
			m_points.insert(it, *it);
			break;
		}
	}
}

size_t
CubicBSpline::numSegments() const
{
	return m_points.size() <= 3 ? 0 : m_points.size() - 3;
}

void
CubicBSpline::appendControlPoint(QPointF const& pt)
{
	m_points.push_back(pt);
}

void
CubicBSpline::moveControlPoint(size_t idx, QPointF const& pos)
{
	m_points[idx] = pos;
}

QPointF
CubicBSpline::eval(double t) const
{
	int const base = (int)t;
	if (base == t) {
		// This branch is not just an optimiation.
		// It allows us to process the very last point
		// without accessing non-existing points.
		return threePointEval(base);
	}

	assert(base >= 0 && base + 3 < (int)m_points.size());

	// The code below assumes 3 <= t <= 4, so let's make it so.
	t -= base - 3;

	// B-spline points.
	QPointF const P123(m_points[base]);
	QPointF const P234(m_points[base + 1]);
	QPointF const P345(m_points[base + 2]);
	QPointF const P456(m_points[base + 3]);

	QPointF const P23t(((4.0 - t)/3.0) * P123 + ((t - 1.0)/3.0) * P234);
	QPointF const P34t(((5.0 - t)/3.0) * P234 + ((t - 2.0)/3.0) * P345);
	QPointF const P45t(((6.0 - t)/3.0) * P345 + ((t - 3.0)/3.0) * P456);
	QPointF const Pt3t(((4.0 - t)/2.0) * P23t + ((t - 2.0)/2.0) * P34t);
	QPointF const Pt4t(((5.0 - t)/2.0) * P34t + ((t - 3.0)/2.0) * P45t);
	QPointF const Pttt((4.0 - t) * Pt3t + (t - 3.0) * Pt4t);

	return Pttt;
}

QPointF
CubicBSpline::threePointEval(int const base) const
{
	assert(base >= 0 && base + 2 < (int)m_points.size());

	// The naming convention below assumes t == 3,
	// but it works with any round t, which in our case equals base.

	QPointF const P123(m_points[base]);
	QPointF const P234(m_points[base + 1]);
	QPointF const P345(m_points[base + 2]);

	QPointF const P233((1.0/3.0) * P123 + (2.0/3.0) * P234);
	QPointF const P343((2.0/3.0) * P234 + (1.0/3.0) * P345);
	QPointF const P333(0.5 * (P233 + P343));

	return P333;
}

QPointF
CubicBSpline::pointClosestTo(QPointF const to, double accuracy, double* t) const
{
	if (!isValid()) {
		if (t) {
			*t = 0;
		}
		return QPointF();
	}

	int const num_segments = numSegments();

	QPointF prev_pt(eval(0));
	QPointF next_pt;
	
	// Find the closest segment.
	int best_segment = 0;
	double best_sqdist = Vec2d(to - prev_pt).squaredNorm();
	for (int seg = 0; seg < num_segments; ++seg, prev_pt = next_pt) {
		next_pt = eval(seg + 1.0);

		double const sqdist = sqDistToLine(to, QLineF(prev_pt, next_pt));
		if (sqdist < best_sqdist) {
			best_segment = seg;
			best_sqdist = sqdist;
		}
	}

	// Continue with a binary search.
	double const sq_accuracy = accuracy * accuracy;
	double prev_t = best_segment;
	double next_t = prev_t + 1;
	prev_pt = eval(prev_t);
	next_pt = eval(next_t);

	while (Vec2d(prev_pt - next_pt).squaredNorm() > sq_accuracy) {
		double const mid_t = 0.5 * (prev_t + next_t);
		QPointF const mid_pt(eval(mid_t));
		
		ToLineProjector const projector(QLineF(prev_pt, next_pt));
		double const pt = projector.projectionScalar(to);
		double const pm = projector.projectionScalar(mid_pt);
		if (pt < pm) {
			next_t = mid_t;
			next_pt = mid_pt;
		} else {
			prev_t = mid_t;
			prev_pt = mid_pt;
		}
	}

	// Take the closest of prev_pt and next_pt.
	if (Vec2d(to - prev_pt).squaredNorm() < Vec2d(to - next_pt).squaredNorm()) {
		if (t) {
			*t = prev_t;
		}
		return prev_pt;
	} else {
		if (t) {
			*t = next_t;
		}
		return next_pt;
	}
}

boost::array<QPointF, 4>
CubicBSpline::toBezierSegment(size_t const segment) const
{
	assert(segment < numSegments());

	// We will be calculating the following points:
	// P333, P334, P344, P444
	// based on the following ones:
	// P123, P234, P345, P456

	// B-spline points.
	QPointF const P123(m_points[segment]);
	QPointF const P234(m_points[segment + 1]);
	QPointF const P345(m_points[segment + 2]);
	QPointF const P456(m_points[segment + 3]);

	// Intermediate points.
	QPointF const P233((1.0/3.0) * P123 + (2.0/3.0) * P234);
	QPointF const P544((2.0/3.0) * P345 + (1.0/3.0) * P456);

	// Bezier points.
	QPointF const P334((2.0/3.0) * P234 + (1.0/3.0) * P345);
	QPointF const P344((1.0/3.0) * P234 + (2.0/3.0) * P345);
	QPointF const P333(0.5 * (P233 + P334));
	QPointF const P444(0.5 * (P344 + P544));

	boost::array<QPointF, 4> res;
	res[0] = P333;
	res[1] = P334;
	res[2] = P344;
	res[3] = P444;

	return res;
}

void
CubicBSpline::setBezierSegment(size_t segment, boost::array<QPointF, 4> const& bezier)
{
	assert(segment < numSegments());

	// Bezier points.
	QPointF const P333(bezier[0]);
	QPointF const P334(bezier[1]);
	QPointF const P344(bezier[2]);
	QPointF const P444(bezier[3]);

	// Intermediate points.
	QPointF const P233(2.0 * P333 - P334);
	QPointF const P544(2.0 * P444 - P344);

	// B-spline points.
	QPointF& P123 = m_points[segment];
	QPointF& P234 = m_points[segment + 1];
	QPointF& P345 = m_points[segment + 2];
	QPointF& P456 = m_points[segment + 3];

	P234 = 2.0 * P334 - P344;
	P345 = 2.0 * P344 - P334;
	P123 = 3.0 * P233 - 2.0 * P234;
	P456 = 3.0 * P544 - 2.0 * P345;
}

void
CubicBSpline::moveBezierPoint(size_t segment, size_t bezier_idx, QPointF const& pos)
{
	assert(segment < numSegments());
	assert(bezier_idx < 4);

	boost::array<QPointF, 4> bezier(toBezierSegment(segment));
	bezier[bezier_idx] = pos;

	setBezierSegment(segment, bezier);
}

std::vector<QPointF>
CubicBSpline::toPolyline(double const accuracy) const
{
	if (!isValid()) {
		return std::vector<QPointF>();
	}

	double const sq_accuracy = accuracy * accuracy;
	std::vector<QPointF> polyline;
	QPointF next_pt(eval(0));
	polyline.push_back(next_pt);
	
	int const num_segments = numSegments();
	for (int segment = 0; segment < num_segments; ++segment) {
		double const prev_t = segment;
		double const next_t = prev_t + 1;
		QPointF const prev_pt(next_pt);
		next_pt = eval(next_t);
		maybeInsertMorePoints(polyline, sq_accuracy, prev_t, prev_pt, next_t, next_pt);
		polyline.push_back(next_pt);
	}

	return polyline;
}

void
CubicBSpline::maybeInsertMorePoints(
	std::vector<QPointF>& polyline, double sq_accuracy,
	double prev_t, QPointF const& prev_pt,
	double next_t, QPointF const& next_pt) const
{
	
	double const mid_t = 0.5 * (prev_t + next_t);
	QPointF const mid_pt(eval(mid_t));
	
	ToLineProjector const projector(QLineF(prev_pt, next_pt));
	QPointF const projection(projector.projectionPoint(mid_pt));
	QPointF const v(mid_pt - projection);
	if (v.x() * v.x() + v.y() * v.y() <= sq_accuracy) {
		return;
	}

	maybeInsertMorePoints(polyline, sq_accuracy, prev_t, prev_pt, mid_t, mid_pt);
	polyline.push_back(mid_pt);
	maybeInsertMorePoints(polyline, sq_accuracy, mid_t, mid_pt, next_t, next_pt);
}

double
CubicBSpline::sqDistToLine(QPointF const& pt, QLineF const& line)
{
	ToLineProjector const projector(line);
	double const p = projector.projectionScalar(pt);
	QPointF ppt;
	if (p <= 0) {
		ppt = line.p1();
	} else if (p >= 1) {
		ppt = line.p2();
	} else {
		ppt = line.pointAt(p);
	}
	return Vec2d(pt - ppt).squaredNorm();
}

} // namespace imageproc
