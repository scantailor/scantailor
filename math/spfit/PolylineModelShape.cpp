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

#include "PolylineModelShape.h"
#include "NumericTraits.h"
#include "MatrixCalc.h"
#include "XSpline.h"
#include "MatMNT.h"
#include "VecNT.h"
#include "ToLineProjector.h"
#include <boost/foreach.hpp>
#include <stdexcept>
#include <limits>
#include <math.h>

using namespace imageproc;

namespace spfit
{

PolylineModelShape::PolylineModelShape(std::vector<QPointF> const& polyline)
{
	if (polyline.size() <= 1) {
		throw std::invalid_argument("PolylineModelShape: polyline must have at least 2 vertices");
	}

	double left = NumericTraits<double>::max();
	double top = NumericTraits<double>::max();
	double right = NumericTraits<double>::min();
	double bottom = NumericTraits<double>::min();

	// We build an interpolating X-spline with control points at the vertices
	// of our polyline.  We'll use it to calculate curvature at polyline vertices.
	XSpline spline;

	BOOST_FOREACH(QPointF const& pt, polyline) {
		spline.appendControlPoint(pt, -1);
		left = std::min<double>(left, pt.x());
		top = std::min<double>(top, pt.y());
		right = std::max<double>(right, pt.x());
		bottom = std::max<double>(bottom, pt.y());
	}
	
	int const num_control_points = spline.numControlPoints();
	double const scale = 1.0 / (num_control_points - 1);
	for (int i = 0; i < num_control_points; ++i) {
		m_vertices.push_back(spline.pointAndDtsAt(i * scale));
	}

	if (left <= right) {
		m_boundingBox.setTopLeft(QPointF(left, top));
		m_boundingBox.setBottomRight(QPointF(right, bottom));
	}
}

QRectF
PolylineModelShape::boundingBox() const
{
	return m_boundingBox;
}

SqDistApproximant
PolylineModelShape::localSqDistApproximant(QPointF const& pt, int flags) const
{
	if (m_vertices.empty()) {
		return SqDistApproximant();
	}

	if (flags & SPLINE_HEAD) {
		SqDistApproximant approx;
		approx.initWithWeightedPointDistance(m_vertices.front().point);
		return approx;
	} else if (flags & SPLINE_TAIL) {
		SqDistApproximant approx;
		approx.initWithWeightedPointDistance(m_vertices.back().point);
		return approx;
	}

	// First, find the point on the polyline closest to pt.
	QPointF best_foot_point;
	double best_sqdist = NumericTraits<double>::max();
	int segment_idx = -1; // If best_foot_point is on a segment, its index goes here.
	int vertex_idx = -1; // If best_foot_point is a vertex, its index goes here.

	// Project pt to each segment.
	int const num_segments = int(m_vertices.size()) - 1;
	for (int i = 0; i < num_segments; ++i) {
		QPointF const pt1(m_vertices[i].point);
		QPointF const pt2(m_vertices[i + 1].point);
		QLineF const segment(pt1, pt2);
		double const s = ToLineProjector(segment).projectionScalar(pt);
		if (s > 0 && s < 1) {
			QPointF const foot_point(segment.pointAt(s));
			Vec2d const vec(pt - foot_point);
			double const sqdist = vec.squaredNorm();
			if (sqdist < best_sqdist) {
				best_sqdist = sqdist;
				best_foot_point = foot_point;
				segment_idx = i;
				vertex_idx = -1;
			}
		}
	}

	// Check if pt is closer to a vertex than to any segment.
	int const num_points = m_vertices.size();
	for (int i = 0; i < num_points; ++i) {
		QPointF const vtx(m_vertices[i].point);
		Vec2d const vec(pt - vtx);
		double const sqdist = vec.squaredNorm();
		if (sqdist < best_sqdist) {
			best_sqdist = sqdist;
			best_foot_point = vtx;
			vertex_idx = i;
			segment_idx = -1;
		}
	}

	if (segment_idx != -1) {
		// The foot point is on a line segment.

		QPointF const pt1(m_vertices[segment_idx].point);
		QPointF const pt2(m_vertices[segment_idx + 1].point);
		QLineF const segment(pt1, pt2);

		Vec2d unit_tangent(pt2 - pt1);
		double const tangent_sqlen = unit_tangent.squaredNorm();
		if (tangent_sqlen > std::numeric_limits<double>::epsilon()) {
			unit_tangent /= sqrt(tangent_sqlen);
		}
		Vec2d unit_normal(-unit_tangent[1], unit_tangent[0]);

		double const curvature = 0;
		return calcApproximant(pt, best_foot_point, unit_tangent, unit_normal, curvature);
	} else {
		// The foot point is a vertex of the polyline.
		assert(vertex_idx != -1);

		Vec2d unit_tangent(m_vertices[vertex_idx].firstDeriv);
		double const tangent_sqlen = unit_tangent.squaredNorm();
		if (tangent_sqlen > std::numeric_limits<double>::epsilon()) {
			unit_tangent /= sqrt(tangent_sqlen);
		}
		Vec2d unit_normal(-unit_tangent[1], unit_tangent[0]);

		double const curvature = m_vertices[vertex_idx].curvature();
		return calcApproximant(pt, best_foot_point, unit_tangent, unit_normal, curvature);
	}
}

SqDistApproximant
PolylineModelShape::calcApproximant(
	Vec2d const& region_origin, Vec2d const& frenet_frame_origin,
	Vec2d const& unit_tangent, Vec2f const& unit_normal, double curvature)
{
	double m = 0;
	if (fabs(curvature) > 1e-6) {
		double const p = fabs(1.0 / curvature);
		double const d = fabs(unit_normal.dot(region_origin - frenet_frame_origin));
		m = d / (d + p); // Formula 7 in [2].
	}

	// Consider the following equation:
	// u = R*x + c
	// u: vector in a Frenet frame.
	// R: rotation matrix.
	// x: vector in the global coordinate system.
	// t: translation component.
	
	// R = | t1 t2 |
	//     | n1 n2 |
	// where [t1 t2] is the tangent unit vector and [n1 n2] is the normal unit vector.
	Mat22d R;
	R(0, 0) = unit_tangent[0];
	R(0, 1) = unit_tangent[1];
	R(1, 0) = unit_normal[0];
	R(1, 1) = unit_normal[1];

	StaticMatrixCalc<double, 4> mc;
	Vec2d c; // translation component
	(-(mc(R) * mc(frenet_frame_origin, 2, 1))).write(c);

	SqDistApproximant approx;
	approx.A(0, 0) = m * R(0, 0) * R(0, 0) + R(1, 0) * R(1, 0);
	approx.A(1, 0) = m * R(0, 0) * R(0, 1) + R(1, 0) * R(1, 1);
	approx.A(0, 1) = approx.A(1, 0);
	approx.A(1, 1) = m * R(0, 1) * R(0, 1) + R(1, 1) * R(1, 1);
	approx.b[0] = 2 * (m * c[0] * R(0, 0) + c[1] * R(1, 0));
	approx.b[1] = 2 * (m * c[0] * R(0, 1) + c[1] * R(1, 1));
	approx.c = m * c[0] * c[0] + c[1] * c[1];

	return approx;
}

} // namespace spfit
