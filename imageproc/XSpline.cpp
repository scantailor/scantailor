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

#include "XSpline.h"
#include "VecNT.h"
#include <string>
#include <stdexcept>
#include <algorithm>
#include <math.h>
#include <assert.h>

namespace imageproc
{

static double g_blend(double u, double q, double p)
{
	double const u2 = u * u;
	double const u3 = u2 * u;
	double const u4 = u3 * u;
	double const u5 = u4 * u;
  
	return q * u + 2 * q * u2 + (10 - 12 * q - p) * u3
		+ (2 * p + 14 * q - 15) * u4 + (6 - 5 * q - p) * u5;
}

static double h_blend(double u, double q)
{
	double const u2 = u * u;
	double const u3 = u2 * u;
	double const u4 = u3 * u;
	double const u5 = u4 * u;
  
	return q * u + 2 * q * u2 - 2 * q * u4 - q * u5;
}

int
XSpline::numControlPoints() const
{
	return m_controlPoints.size();
}

int
XSpline::numSegments() const
{
	return std::max<int>(m_controlPoints.size() - 1, 0);
}

void
XSpline::appendControlPoint(QPointF const& pos, double weight)
{
	m_controlPoints.push_back(ControlPoint(pos, weight));
}

void
XSpline::insertControlPoint(int idx, QPointF const& pos, double weight)
{
	if (idx < 0 || idx > (int)m_controlPoints.size()) {
		throw std::invalid_argument("XSpline::insertControlPoint: idx out of range");
	}

	m_controlPoints.insert(m_controlPoints.begin() + idx, ControlPoint(pos, weight));
}

void
XSpline::moveControlPoint(int idx, QPointF const& pos)
{
	if (idx < 0 || idx >= (int)m_controlPoints.size()) {
		throw std::invalid_argument("XSpline::moveControlPoint: idx out of range");
	}

	m_controlPoints[idx].pos = pos;
}

QPointF
XSpline::eval(double t) const
{
	if (t < 0 || t > 1) {
		throw std::invalid_argument("XSpline::eval: t out of range");
	}

	int const num_segments = numSegments();
	if (num_segments == 0) {
		throw std::logic_error("XSpline::eval: spline doesn't have any segments");
	}

	if (t == 1.0) {
		// If we went with the branch below, we would end up with
		// segment == num_segments, which is an error.
		return eval(num_segments - 1, 1.0);
	} else {
		double const t2 = t * num_segments;
		double const segment = floor(t2);
		return eval((int)segment, t2 - segment);
	}
}

QPointF
XSpline::eval(int segment, double t) const
{
	if (segment < 0 || segment >= (int)m_controlPoints.size() - 1) {
		// This will also cover the case of none or one control point.
		throw std::invalid_argument("XSpline::eval: segment out of range");
	} else if (t < 0 || t > 1) {
		throw std::invalid_argument("XSpline::eval: t out of range");
	}

	std::vector<ControlPoint> pts;
	pts.reserve(4);

	if (segment == 0) {
		pts.push_back(m_controlPoints[0]);
	} else {
		pts.push_back(m_controlPoints[segment - 1]);
	}

	pts.push_back(m_controlPoints[segment]);
	pts.push_back(m_controlPoints[segment + 1]);

	if (segment + 2 >= (int)m_controlPoints.size()) {
		pts.push_back(m_controlPoints[segment + 1]);
	} else {
		pts.push_back(m_controlPoints[segment + 2]);
	}

	return evalImpl(pts, t);
}

QPointF
XSpline::evalImpl(std::vector<ControlPoint> const& pts, double t)
{
	assert(pts.size() == 4);

	double const T0p = std::max<double>(pts[1].weight, 0);
	double const T1p = 1.0 + std::max<double>(pts[2].weight, 0);
	double const T2m = -T0p;
	double const T3m = 1.0 - std::max<double>(pts[2].weight, 0);

	double const pm1 = (1.0 + T0p) * (1.0 + T0p) * 2.0;
	double const pm0 = T1p * T1p * 2.0;
	double const pp1 = (1.0 - T2m) * (1.0 - T2m) * 2.0;
	double const pp2 = (2.0 - T3m) * (2.0 - T3m) * 2.0;

	double const qp0 = -0.5 * std::min<double>(pts[1].weight, 0);
	double const qp1 = -0.5 * std::min<double>(pts[2].weight, 0);
	double const qp2 = qp0;
	double const qp3 = qp1;

	double A[4];
	if (t <= T0p) {
		A[0] = g_blend((t - T0p) / (-1.0 - T0p), qp0, pm1);
	} else {
		A[0] = qp0 > 0.0 ? h_blend((t - T0p) / (-1.0 - T0p), qp0) : 0.0;
	}

	A[1] = g_blend((T1p - t) / T1p, qp1, pm0);
	A[2] = g_blend((T2m - t) / (T2m - 1.0), qp2, pp1);

	if (t >= T3m) {
		A[3] = g_blend((t - T3m) / T1p, qp3, pp2);
	} else {
		A[3] = qp3 > 0.0 ? h_blend((t - T3m) / T1p, qp3) : 0.0;
	}

	QPointF pt(pts[0].pos * A[0] + pts[1].pos * A[1] + pts[2].pos * A[2] + pts[3].pos * A[3]);
	pt /= A[0] + A[1] + A[2] + A[3];
	return pt;
}

std::vector<QPointF>
XSpline::toPolyline(double const accuracy) const
{
	if (m_controlPoints.empty()) {
		return std::vector<QPointF>();
	}

	double const sq_accuracy = accuracy * accuracy;
	std::vector<QPointF> polyline;
	QPointF next_pt(eval(0, 0.0));
	polyline.push_back(next_pt);
	
	int const num_segments = numSegments();
	for (int segment = 0; segment < num_segments; ++segment) {
		double const prev_t = 0;
		double const next_t = 1;
		QPointF const prev_pt(next_pt);
		next_pt = eval(segment, next_t);
		maybeInsertMorePoints(polyline, sq_accuracy, segment, prev_t, prev_pt, next_t, next_pt);
		polyline.push_back(next_pt);
	}

	return polyline;
}

void
XSpline::maybeInsertMorePoints(
	std::vector<QPointF>& polyline, double sq_accuracy,
	int segment, double prev_t, QPointF const& prev_pt,
	double next_t, QPointF const& next_pt) const
{
	
	double const mid_t = 0.5 * (prev_t + next_t);
	QPointF const mid_pt(eval(segment, mid_t));
	QPointF const projection(projectToLine(mid_pt, QLineF(prev_pt, next_pt)));
	QPointF const v(mid_pt - projection);
	if (v.x() * v.x() + v.y() * v.y() <= sq_accuracy) {
		return;
	}

	maybeInsertMorePoints(polyline, sq_accuracy, segment, prev_t, prev_pt, mid_t, mid_pt);
	polyline.push_back(mid_pt);
	maybeInsertMorePoints(polyline, sq_accuracy, segment, mid_t, mid_pt, next_t, next_pt);
}

QPointF
XSpline::projectToLine(QPointF const& pt, QLineF const& line)
{
	Vec2d const A(line.p2() - line.p1());
	// trans(A)*A*p = trans(A)*B
	double const AtA = A.dot(A);
	double const AtB = A.dot(pt - line.p1());
	return line.p1() + A * (AtB / AtA);
}

} // namespace imageproc