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
#include "MatrixCalc.h"
#include "ToLineProjector.h"
#include <boost/scoped_array.hpp>
#include <string>
#include <stdexcept>
#include <algorithm>
#include <math.h>
#include <string.h>
#include <stddef.h>
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
XSpline::eraseControlPoint(int idx)
{
	if (idx < 0 || idx >= (int)m_controlPoints.size()) {
		throw std::invalid_argument("XSpline::eraseControlPoint: idx out of range");
	}

	m_controlPoints.erase(m_controlPoints.begin() + idx);
}

void
XSpline::moveControlPoint(int idx, QPointF const& pos)
{
	if (idx < 0 || idx >= (int)m_controlPoints.size()) {
		throw std::invalid_argument("XSpline::moveControlPoint: idx out of range");
	}

	m_controlPoints[idx].pos = pos;
}

void
XSpline::setControlPointWeight(int idx, double weight)
{
	if (idx < 0 || idx >= (int)m_controlPoints.size()) {
		throw std::invalid_argument("XSpline::setControlPoint: idx out of range");
	}

	m_controlPoints[idx].weight = weight;
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

	Vec4d A;
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

	A /= A.sum();

	QPointF pt(pts[0].pos * A[0] + pts[1].pos * A[1] + pts[2].pos * A[2] + pts[3].pos * A[3]);
	return pt;
}

void
XSpline::eval2Impl(double t, VirtualFunction2<void, int, double>& out) const
{
	assert(t >= 0 && t <= 1);

	int const num_segments = numSegments();
	assert(num_segments > 0);

	if (t == 1.0) {
		// If we went with the branch below, we would end up with
		// segment == num_segments, which is an error.
		return eval2Impl(num_segments - 1, 1.0, out);
	} else {
		double const t2 = t * num_segments;
		double const segment = floor(t2);
		return eval2Impl((int)segment, t2 - segment, out);
	}
}

void
XSpline::eval2Impl(int segment, double t, VirtualFunction2<void, int, double>& out) const
{
	assert(segment >= 0 && segment < (int)m_controlPoints.size() - 1);
	assert(t >= 0 && t <= 1);

	int idxs[4];
	idxs[0] = std::max<int>(0, segment - 1);
	idxs[1] = segment;
	idxs[2] = segment + 1;
	idxs[3] = std::min<int>(segment + 2, m_controlPoints.size() - 1);

	ControlPoint pts[4];
	for (int i = 0; i < 4; ++i) {
		pts[i] = m_controlPoints[idxs[i]];
	}

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

	Vec4d A;
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

	A /= A.sum();

	if (idxs[0] == idxs[1]) {
		out(idxs[0], A[0] + A[1]);
	} else {
		out(idxs[0], A[0]);
		out(idxs[1], A[1]);
	}
	
	if (idxs[2] == idxs[3]) {
		out(idxs[2], A[2] + A[3]);
	} else {
		out(idxs[2], A[2]);
		out(idxs[3], A[3]);
	}
}

QPointF
XSpline::pointClosestTo(QPointF const to, double* t, double accuracy) const
{
	if (m_controlPoints.empty()) {
		if (t) {
			*t = 0;
		}
		return QPointF();
	}

	int const num_segments = numSegments();
	if (num_segments == 0) {
		if (t) {
			*t = 0;
		}
		return m_controlPoints.front().pos;
	}

	QPointF prev_pt(eval(0, 0));
	QPointF next_pt;
	
	// Find the closest segment.
	int best_segment = 0;
	double best_sqdist = Vec2d(to - prev_pt).squaredNorm();
	for (int seg = 0; seg < num_segments; ++seg, prev_pt = next_pt) {
		next_pt = eval(seg, 1.0);

		double const sqdist = sqDistToLine(to, QLineF(prev_pt, next_pt));
		if (sqdist < best_sqdist) {
			best_segment = seg;
			best_sqdist = sqdist;
		}
	}

	// Continue with a binary search.
	double const sq_accuracy = accuracy * accuracy;
	double prev_t = 0;
	double next_t = 1;
	prev_pt = eval(best_segment, prev_t);
	next_pt = eval(best_segment, next_t);

	while (Vec2d(prev_pt - next_pt).squaredNorm() > sq_accuracy) {
		double const mid_t = 0.5 * (prev_t + next_t);
		QPointF const mid_pt(eval(best_segment, mid_t));
		
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
			*t = (best_segment + prev_t) / num_segments;
		}
		return prev_pt;
	} else {
		if (t) {
			*t = (best_segment + next_t) / num_segments;
		}
		return next_pt;
	}
}

QPointF
XSpline::pointClosestTo(QPointF const to, double accuracy) const
{
	return pointClosestTo(to, 0, accuracy);
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
	QPointF const projection(
		ToLineProjector(QLineF(prev_pt, next_pt)).projectionPoint(mid_pt)
	);
	QPointF const v(mid_pt - projection);
	if (v.x() * v.x() + v.y() * v.y() <= sq_accuracy) {
		return;
	}

	maybeInsertMorePoints(polyline, sq_accuracy, segment, prev_t, prev_pt, mid_t, mid_pt);
	polyline.push_back(mid_pt);
	maybeInsertMorePoints(polyline, sq_accuracy, segment, mid_t, mid_pt, next_t, next_pt);
}

double
XSpline::sqDistToLine(QPointF const& pt, QLineF const& line)
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

void
XSpline::fit(std::vector<QPointF> const& samples, boost::dynamic_bitset<> const* fixed_points)
{
	if (fixed_points) {
		if (m_controlPoints.size() != fixed_points->size()) {
			throw std::invalid_argument("XSpline::fit: fixed_points has incorrect size");
		}
	}

	struct Mat : public VirtualFunction2<void, int, double>
	{
		boost::scoped_array<double> A;
		size_t rowsA;
		size_t row;

		Mat(size_t num_samples, size_t num_control_points)
		:	A(new double[num_samples * num_control_points]),
			rowsA(num_samples),
			row(0)
		{
			memset(A.get(), 0, num_samples * num_control_points * sizeof(A[0]));
		}

		void operator()(int idx, double coeff) {
			A[idx * rowsA + row] = coeff;
		}
	};

	size_t const num_control_points = m_controlPoints.size();
	size_t const num_samples = samples.size();
	Mat mat(num_samples, num_control_points);
	double const t_step = 1.0 / (num_samples - 1);
	for (mat.row = 0; mat.row < num_samples; ++mat.row) {
		eval2Impl(mat.row * t_step, mat);	
	}

	boost::scoped_array<double> A;
	
	size_t const rowsA = num_samples;
	size_t colsA = num_control_points;

	QPointF const* B = &samples[0];
	std::vector<QPointF> adjustedB;

	if (!fixed_points || !fixed_points->any()) {
		A.swap(mat.A);
	} else {
		colsA -= fixed_points->count();
		A.reset(new double[rowsA * colsA]);
		adjustedB = samples;
		B = &adjustedB[0];

		// Populate A and adjust B.
		size_t out_col = 0;
		for (size_t cp = 0; cp < num_control_points; ++cp) {
			if (fixed_points->test(cp)) {
				// Fixed -> adjust B.
				for (size_t sp = 0; sp < num_samples; ++sp) {
					adjustedB[sp] -= mat.A[cp * num_samples + sp] * m_controlPoints[cp].pos;
				}
			} else {
				// Not fixed -> copy a column from mat.A into A.
				memcpy(&A[out_col * num_samples], &mat.A[cp * num_samples], num_samples * sizeof(A[0]));
				++out_col;
			}
		}
	}

	DynamicMatrixCalc<double> mc;

	boost::scoped_array<double> At(new double[colsA * rowsA]);
	mc(A.get(), rowsA, colsA).transWrite(At.get());

	(
		(mc(At.get(), colsA, rowsA) * mc(A.get(), rowsA, colsA)).inv()
		* mc(At.get(), colsA, rowsA)
		
		// Re-use At for a projection matrix.
	).write(At.get());
	
	size_t col = 0;
	for (size_t cp = 0; cp < num_control_points; ++cp) {
		if (fixed_points && fixed_points->test(cp)) {
			// Skip fixed points.
			continue;
		}

		QPointF pt(0, 0);
		for (size_t j = 0; j < num_samples; ++j) {
			pt += B[j] * At[j * colsA + col];
		}
		m_controlPoints[cp].pos = pt;

		++col;
	}
}

} // namespace imageproc
