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
#include "MatT.h"
#include "NumericTraits.h"
#include "ToLineProjector.h"
#include "adiff/SparseMap.h"
#include "adiff/Function.h"
#include <QLineF>
#include <QDebug>
#ifndef Q_MOC_RUN
#include <boost/foreach.hpp>
#endif
#include <string>
#include <stdexcept>
#include <algorithm>
#include <math.h>
#include <assert.h>

struct XSpline::TensionDerivedParams
{
	static double const t0;
	static double const t1;
	static double const t2;
	static double const t3;

	// These correspond to Tk- and Tk+ in [1].
	double T0p;
	double T1p;
	double T2m;
	double T3m;

	// q parameters for GBlendFunc and HBlendFunc.
	double q[4];

	// p parameters for GBlendFunc.
	double p[4];

	TensionDerivedParams(double tension1, double tension2);
};


class XSpline::GBlendFunc
{
public:
	GBlendFunc(double q, double p);

	double value(double u) const;

	double firstDerivative(double u) const;

	double secondDerivative(double u) const;
private:
	double m_c1;
	double m_c2;
	double m_c3;
	double m_c4;
	double m_c5;
};


class XSpline::HBlendFunc
{
public:
	HBlendFunc(double q);

	double value(double u) const;

	double firstDerivative(double u) const;

	double secondDerivative(double u) const;
private:
	double m_c1;
	double m_c2;
	double m_c4;
	double m_c5;
};


struct XSpline::DecomposedDerivs
{
	double zeroDerivCoeffs[4];
	double firstDerivCoeffs[4];
	double secondDerivCoeffs[4];
	int controlPoints[4];
	int numControlPoints;

	bool hasNonZeroCoeffs(int idx) const {
		double sum = fabs(zeroDerivCoeffs[idx]) + fabs(firstDerivCoeffs[idx]) + fabs(secondDerivCoeffs[idx]);
		return sum > std::numeric_limits<double>::epsilon();
	}
};


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

double
XSpline::controlPointIndexToT(int idx) const
{
	assert(idx >= 0 || idx <= (int)m_controlPoints.size());
	return double(idx) / (m_controlPoints.size() - 1);
}

void
XSpline::appendControlPoint(QPointF const& pos, double tension)
{
	m_controlPoints.push_back(ControlPoint(pos, tension));
}

void
XSpline::insertControlPoint(int idx, QPointF const& pos, double tension)
{
	assert(idx >= 0 || idx <= (int)m_controlPoints.size());
	m_controlPoints.insert(m_controlPoints.begin() + idx, ControlPoint(pos, tension));
}

void
XSpline::eraseControlPoint(int idx)
{
	assert(idx >= 0 || idx < (int)m_controlPoints.size());
	m_controlPoints.erase(m_controlPoints.begin() + idx);
}

QPointF
XSpline::controlPointPosition(int idx) const
{
	return m_controlPoints[idx].pos;
}

void
XSpline::moveControlPoint(int idx, QPointF const& pos)
{
	assert(idx >= 0 || idx < (int)m_controlPoints.size());
	m_controlPoints[idx].pos = pos;
}

double
XSpline::controlPointTension(int idx) const
{
	assert(idx >= 0 && idx < (int)m_controlPoints.size());
	return m_controlPoints[idx].tension;
}

void
XSpline::setControlPointTension(int idx, double tension)
{
	assert(idx >= 0 && idx < (int)m_controlPoints.size());
	m_controlPoints[idx].tension = tension;
}

QPointF
XSpline::pointAt(double t) const
{
	int const num_segments = numSegments();
	assert(num_segments > 0);
	assert(t >= 0 && t <= 1);

	if (t == 1.0) {
		// If we went with the branch below, we would end up with
		// segment == num_segments, which is an error.
		return pointAtImpl(num_segments - 1, 1.0);
	} else {
		double const t2 = t * num_segments;
		double const segment = floor(t2);
		return pointAtImpl((int)segment, t2 - segment);
	}
}

QPointF
XSpline::pointAtImpl(int segment, double t) const
{
	LinearCoefficient coeffs[4];
	int const num_coeffs = linearCombinationFor(coeffs, segment, t);

	QPointF pt(0, 0);
	for (int i = 0; i < num_coeffs; ++i) {
		LinearCoefficient const& c = coeffs[i];
		pt += m_controlPoints[c.controlPointIdx].pos * c.coeff;
	}

	return pt;
}

void
XSpline::sample(
	VirtualFunction3<void, QPointF, double, SampleFlags>& sink,
	SamplingParams const& params, double from_t, double to_t) const
{
	if (m_controlPoints.empty()) {
		return;
	}

	double max_sqdist_to_spline = params.maxDistFromSpline;
	if (max_sqdist_to_spline != NumericTraits<double>::max()) {
		max_sqdist_to_spline *= params.maxDistFromSpline;
	}

	double max_sqdist_between_samples = params.maxDistBetweenSamples;
	if (max_sqdist_between_samples != NumericTraits<double>::max()) {
		max_sqdist_between_samples *= params.maxDistBetweenSamples;
	}

	QPointF const from_pt(pointAt(from_t));
	QPointF const to_pt(pointAt(to_t));
	sink(from_pt, from_t, HEAD_SAMPLE);
	
	int const num_segments = numSegments();
	if (num_segments == 0) {
		return;
	}
	double const r_num_segments = 1.0 / num_segments;

	maybeAddMoreSamples(
		sink, max_sqdist_to_spline, max_sqdist_between_samples,
		num_segments, r_num_segments, from_t, from_pt, to_t, to_pt
	);

	sink(to_pt, to_t, TAIL_SAMPLE);
}

void
XSpline::maybeAddMoreSamples(
	VirtualFunction3<void, QPointF, double, SampleFlags>& sink,
	double max_sqdist_to_spline, double max_sqdist_between_samples,
	double num_segments, double r_num_segments,
	double prev_t, QPointF const& prev_pt,
	double next_t, QPointF const& next_pt) const
{
	double const prev_next_sqdist = Vec2d(next_pt - prev_pt).squaredNorm();
	if (prev_next_sqdist < 1e-6) {
		// Too close. Projecting anything on such a small line segment is dangerous.
		return;
	}

	SampleFlags flags = DEFAULT_SAMPLE;
	double mid_t = 0.5 * (prev_t + next_t);
	double const nearby_junction_t = floor(mid_t * num_segments + 0.5) * r_num_segments;
	
	// If nearby_junction_t is between prev_t and next_t, make it our mid_t.
	if ((nearby_junction_t - prev_t) * (next_t - prev_t) > 0 &&
		(nearby_junction_t - next_t) * (prev_t - next_t) > 0) {
		mid_t = nearby_junction_t;
		flags = JUNCTION_SAMPLE;
	}

	QPointF const mid_pt(pointAt(mid_t));
	
	if (flags != JUNCTION_SAMPLE) {
		QPointF const projection(
			ToLineProjector(QLineF(prev_pt, next_pt)).projectionPoint(mid_pt)
		);

		if (prev_next_sqdist <= max_sqdist_between_samples) {
			if (Vec2d(mid_pt - projection).squaredNorm() <= max_sqdist_to_spline) {
				return;
			}
		}
	}

	maybeAddMoreSamples(
		sink, max_sqdist_to_spline, max_sqdist_between_samples,
		num_segments, r_num_segments, prev_t, prev_pt, mid_t, mid_pt
	);

	sink(mid_pt, mid_t, flags);
	
	maybeAddMoreSamples(
		sink, max_sqdist_to_spline, max_sqdist_between_samples,
		num_segments, r_num_segments, mid_t, mid_pt, next_t, next_pt
	);
}

void
XSpline::linearCombinationAt(double t, std::vector<LinearCoefficient>& coeffs) const
{
	assert(t >= 0 && t <= 1);

	int const num_segments = numSegments();
	assert(num_segments > 0);

	int num_coeffs = 4;
	coeffs.resize(num_coeffs);
	LinearCoefficient static_coeffs[4];

	if (t == 1.0) {
		// If we went with the branch below, we would end up with
		// segment == num_segments, which is an error.
		num_coeffs = linearCombinationFor(static_coeffs, num_segments - 1, 1.0);
	} else {
		double const t2 = t * num_segments;
		double const segment = floor(t2);
		num_coeffs = linearCombinationFor(static_coeffs, (int)segment, t2 - segment);
	}

	for (int i = 0; i < num_coeffs; ++i) {
		coeffs[i] = static_coeffs[i];
	}
	coeffs.resize(num_coeffs);
}

int
XSpline::linearCombinationFor(LinearCoefficient* coeffs, int segment, double t) const
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

	TensionDerivedParams const tdp(pts[1].tension, pts[2].tension);

	Vec4d A;

	if (t <= tdp.T0p) {
		A[0] = GBlendFunc(tdp.q[0], tdp.p[0]).value((t - tdp.T0p) / (tdp.t0 - tdp.T0p));
	} else {
		A[0] = HBlendFunc(tdp.q[0]).value((t - tdp.T0p) / (tdp.t0 - tdp.T0p));
	}

	A[1] = GBlendFunc(tdp.q[1], tdp.p[1]).value((t - tdp.T1p) / (tdp.t1 - tdp.T1p));
	A[2] = GBlendFunc(tdp.q[2], tdp.p[2]).value((t - tdp.T2m) / (tdp.t2 - tdp.T2m));

	if (t >= tdp.T3m) {
		A[3] = GBlendFunc(tdp.q[3], tdp.p[3]).value((t - tdp.T3m) / (tdp.t3 - tdp.T3m));
	} else {
		A[3] = HBlendFunc(tdp.q[3]).value((t - tdp.T3m) / (tdp.t3 - tdp.T3m));
	}

	A /= A.sum();

	int out_idx = 0;

	if (idxs[0] == idxs[1]) {
		coeffs[out_idx++] = LinearCoefficient(idxs[0], A[0] + A[1]);
	} else {
		coeffs[out_idx++] = LinearCoefficient(idxs[0], A[0]);
		coeffs[out_idx++] = LinearCoefficient(idxs[1], A[1]);
	}
	
	if (idxs[2] == idxs[3]) {
		coeffs[out_idx++] = LinearCoefficient(idxs[2], A[2] + A[3]);
	} else {
		coeffs[out_idx++] = LinearCoefficient(idxs[2], A[2]);
		coeffs[out_idx++] = LinearCoefficient(idxs[3], A[3]);
	}

	return out_idx;
}

XSpline::PointAndDerivs
XSpline::pointAndDtsAt(double t) const
{
	assert(t >= 0 && t <= 1);
	PointAndDerivs pd;

	DecomposedDerivs const derivs(decomposedDerivs(t));
	for (int i = 0; i < derivs.numControlPoints; ++i) {
		QPointF const& cp = m_controlPoints[derivs.controlPoints[i]].pos;
		pd.point += cp * derivs.zeroDerivCoeffs[i];
		pd.firstDeriv += cp * derivs.firstDerivCoeffs[i];
		pd.secondDeriv += cp * derivs.secondDerivCoeffs[i];
	}

	return pd;
}

XSpline::DecomposedDerivs
XSpline::decomposedDerivs(double const t) const
{
	assert(t >= 0 && t <= 1);

	int const num_segments = numSegments();
	assert(num_segments > 0);

	if (t == 1.0) {
		// If we went with the branch below, we would end up with
		// segment == num_segments, which is an error.
		return decomposedDerivsImpl(num_segments - 1, 1.0);
	} else {
		double const t2 = t * num_segments;
		double const segment = floor(t2);
		return decomposedDerivsImpl((int)segment, t2 - segment);
	}
}

XSpline::DecomposedDerivs
XSpline::decomposedDerivsImpl(int const segment, double const t) const
{
	assert(segment >= 0 && segment < (int)m_controlPoints.size() - 1);
	assert(t >= 0 && t <= 1);

	DecomposedDerivs derivs;

	derivs.numControlPoints = 4; // May be modified later in this function.
	derivs.controlPoints[0] = std::max<int>(0, segment - 1);
	derivs.controlPoints[1] = segment;
	derivs.controlPoints[2] = segment + 1;
	derivs.controlPoints[3] = std::min<int>(segment + 2, m_controlPoints.size() - 1);

	ControlPoint pts[4];
	for (int i = 0; i < 4; ++i) {
		pts[i] = m_controlPoints[derivs.controlPoints[i]];
	}

	TensionDerivedParams const tdp(pts[1].tension, pts[2].tension);

	// Note that we don't want the derivate with respect to t that's
	// passed to us (ranging from 0 to 1 within a segment).
	// Rather we want it with respect to the t that's passed to
	// decomposedDerivs(), ranging from 0 to 1 across all segments.
	// Let's call the latter capital T.  Their relationship is:
	// t = T*num_segments - C
	// dt/dT = num_segments
	double const dtdT = numSegments();

	Vec4d A;
	Vec4d dA; // First derivatives with respect to T.
	Vec4d ddA; // Second derivatives with respect to T.

	// Control point 0.
	{
		// u = (t - tdp.T0p) / (tdp.t0 - tdp.T0p)
		double const ta = 1.0 / (tdp.t0 - tdp.T0p);
		double const tb = -tdp.T0p * ta;
		double const u = ta * t + tb;		
		if (t <= tdp.T0p) {
			// u(t) = ta * tt + tb
			// u'(t) = ta
			// g(t) = g(u(t), <tension derived params>)
			GBlendFunc g(tdp.q[0], tdp.p[0]);
			A[0] = g.value(u);

			// g'(u(t(T))) = g'(u)*u'(t)*t'(T)
			dA[0] = g.firstDerivative(u) * (ta * dtdT);

			// Note that u'(t) and t'(T) are constant functions.
			// g"(u(t(T))) = g"(u)*u'(t)*t'(T)*u'(t)*t'(T)
			ddA[0] = g.secondDerivative(u) * (ta * dtdT) * (ta * dtdT);
		} else {
			HBlendFunc h(tdp.q[0]);
			A[0] = h.value(u);
			dA[0] = h.firstDerivative(u) * (ta * dtdT);
			ddA[0] = h.secondDerivative(u) * (ta * dtdT) * (ta * dtdT);
		}
	}

	// Control point 1.
	{
		// u = (t - tdp.T1p) / (tdp.t1 - tdp.T1p)
		double const ta = 1.0 / (tdp.t1 - tdp.T1p);
		double const tb = -tdp.T1p * ta;
		double const u = ta * t + tb;
		GBlendFunc g(tdp.q[1], tdp.p[1]);
		A[1] = g.value(u);
		dA[1] = g.firstDerivative(u) * (ta * dtdT);
		ddA[1] = g.secondDerivative(u) * (ta * dtdT) * (ta * dtdT);
	}

	// Control point 2.
	{
		// u = (t - tdp.T2m) / (tdp.t2 - tdp.T2m)
		double const ta = 1.0 / (tdp.t2 - tdp.T2m);
		double const tb = -tdp.T2m * ta;
		double const u = ta * t + tb;
		GBlendFunc g(tdp.q[2], tdp.p[2]);
		A[2] = g.value(u);
		dA[2] = g.firstDerivative(u) * (ta * dtdT);
		ddA[2] = g.secondDerivative(u) * (ta * dtdT) * (ta * dtdT);
	}

	// Control point 3.
	{
		// u = (t - tdp.T3m) / (tdp.t3 - tdp.T3m)
		double const ta = 1.0 / (tdp.t3 - tdp.T3m);
		double const tb = -tdp.T3m * ta;
		double const u = ta * t + tb;
		if (t >= tdp.T3m) {
			GBlendFunc g(tdp.q[3], tdp.p[3]);
			A[3] = g.value(u);
			dA[3] = g.firstDerivative(u) * (ta * dtdT);
			ddA[3] = g.secondDerivative(u) * (ta * dtdT) * (ta * dtdT);
		} else {
			HBlendFunc h(tdp.q[3]);
			A[3] = h.value(u);
			dA[3] = h.firstDerivative(u) * (ta * dtdT);
			ddA[3] = h.secondDerivative(u) * (ta * dtdT) * (ta * dtdT);
		}
	}
	
	double const sum = A.sum();
	double const sum2 = sum * sum;
	double const sum4 = sum2 * sum2;
	double const d_sum = dA.sum();
	double const dd_sum = ddA.sum();

	for (int i = 0; i < 4; ++i) {
		derivs.zeroDerivCoeffs[i] = A[i] / sum;
 
		double const d1 = dA[i] * sum - A[i] * d_sum;
		derivs.firstDerivCoeffs[i] = d1 / sum2; // Derivative of: A[i] / sum
		
		// Derivative of: dA[i] * sum
		double const dd1 = ddA[i] * sum + dA[i] * d_sum;

		// Derivative of: A[i] * d_sum
		double const dd2 = dA[i] * d_sum + A[i] * dd_sum;

		// Derivative of (dA[i] * sum - A[i] * d_sum) / sum2
		double const dd3 = ((dd1 - dd2) * sum2 - d1 * (2 * sum * d_sum)) / sum4;
		derivs.secondDerivCoeffs[i] = dd3;
	}

	// Merge / throw away some control points.
	int write_idx = 0;
	int merge_idx = 0;
	int read_idx = 1;
	int const end = 4;
	for (;;) {
		assert(merge_idx != read_idx);
		for (; read_idx != end && derivs.controlPoints[read_idx] == derivs.controlPoints[merge_idx]; ++read_idx) {
			// Merge
			derivs.zeroDerivCoeffs[merge_idx] += derivs.zeroDerivCoeffs[read_idx];
			derivs.firstDerivCoeffs[merge_idx] += derivs.firstDerivCoeffs[read_idx];
			derivs.secondDerivCoeffs[merge_idx] += derivs.secondDerivCoeffs[read_idx];
		}

		if (derivs.hasNonZeroCoeffs(merge_idx)) {
			// Copy
			derivs.zeroDerivCoeffs[write_idx] = derivs.zeroDerivCoeffs[merge_idx];
			derivs.firstDerivCoeffs[write_idx] = derivs.firstDerivCoeffs[merge_idx];
			derivs.secondDerivCoeffs[write_idx] = derivs.secondDerivCoeffs[merge_idx];
			derivs.controlPoints[write_idx] = derivs.controlPoints[merge_idx];
			++write_idx;
		}
		
		if (read_idx == end) {
			break;
		}

		merge_idx = read_idx;
		++read_idx;
	}
	derivs.numControlPoints = write_idx;

	return derivs;
}

QuadraticFunction
XSpline::controlPointsAttractionForce() const
{
	return controlPointsAttractionForce(0, numSegments());
}

QuadraticFunction
XSpline::controlPointsAttractionForce(int seg_begin, int seg_end) const
{
	using namespace adiff;

	assert(seg_begin >= 0 && seg_begin <= numSegments());
	assert(seg_end >= 0 && seg_end <= numSegments());
	assert(seg_begin <= seg_end);

	int const num_control_points = numControlPoints();

	SparseMap<2> sparse_map(num_control_points * 2);
	sparse_map.markAllNonZero();

	Function<2> force(sparse_map);
	if (seg_begin != seg_end) {
		Function<2> prev_x(seg_begin * 2, m_controlPoints[seg_begin].pos.x(), sparse_map);
		Function<2> prev_y(seg_begin * 2 + 1, m_controlPoints[seg_begin].pos.y(), sparse_map);

		for (int i = seg_begin + 1; i <= seg_end; ++i) {
			Function<2> next_x(i * 2, m_controlPoints[i].pos.x(), sparse_map);
			Function<2> next_y(i * 2 + 1, m_controlPoints[i].pos.y(), sparse_map);

			Function<2> const dx(next_x - prev_x);
			Function<2> const dy(next_y - prev_y);
			force += dx * dx + dy * dy;

			next_x.swap(prev_x);
			next_y.swap(prev_y);
		}
	}

	QuadraticFunction f(num_control_points * 2);
	f.A = 0.5 * force.hessian(sparse_map);
	f.b = force.gradient(sparse_map);
	f.c = force.value;

	return f;
}

QuadraticFunction
XSpline::junctionPointsAttractionForce() const
{
	return junctionPointsAttractionForce(0, numSegments());
}

QuadraticFunction
XSpline::junctionPointsAttractionForce(int seg_begin, int seg_end) const
{
	using namespace adiff;

	assert(seg_begin >= 0 && seg_begin <= numSegments());
	assert(seg_end >= 0 && seg_end <= numSegments());
	assert(seg_begin <= seg_end);

	int const num_control_points = numControlPoints();

	SparseMap<2> sparse_map(num_control_points * 2);
	sparse_map.markAllNonZero();

	Function<2> force(sparse_map);

	if (seg_begin != seg_end) {
		QPointF pt(pointAt(controlPointIndexToT(seg_begin)));
		std::vector<LinearCoefficient> coeffs;
		Function<2> prev_x(0);
		Function<2> prev_y(0);

		for (int i = seg_begin; i <= seg_end; ++i) {
			Function<2> next_x(sparse_map);
			Function<2> next_y(sparse_map);
			
			linearCombinationAt(controlPointIndexToT(i), coeffs);
			BOOST_FOREACH(LinearCoefficient const& coeff, coeffs) {
				QPointF const cp(m_controlPoints[coeff.controlPointIdx].pos);
				Function<2> x(coeff.controlPointIdx * 2, cp.x(), sparse_map);
				Function<2> y(coeff.controlPointIdx * 2 + 1, cp.y(), sparse_map);
				x *= coeff.coeff;
				y *= coeff.coeff;
				next_x += x;
				next_y += y;
			}

			if (i != seg_begin) {
				Function<2> const dx(next_x - prev_x);
				Function<2> const dy(next_y - prev_y);
				force += dx * dx + dy * dy;
			}

			next_x.swap(prev_x);
			next_y.swap(prev_y);
		}
	}

	QuadraticFunction f(num_control_points * 2);
	f.A = 0.5 * force.hessian(sparse_map);
	f.b = force.gradient(sparse_map);
	f.c = force.value;

	return f;
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

	QPointF prev_pt(pointAtImpl(0, 0));
	QPointF next_pt;
	
	// Find the closest segment.
	int best_segment = 0;
	double best_sqdist = Vec2d(to - prev_pt).squaredNorm();
	for (int seg = 0; seg < num_segments; ++seg, prev_pt = next_pt) {
		next_pt = pointAtImpl(seg, 1.0);

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
	prev_pt = pointAtImpl(best_segment, prev_t);
	next_pt = pointAtImpl(best_segment, next_t);

	while (Vec2d(prev_pt - next_pt).squaredNorm() > sq_accuracy) {
		double const mid_t = 0.5 * (prev_t + next_t);
		QPointF const mid_pt(pointAtImpl(best_segment, mid_t));
		
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
XSpline::toPolyline(SamplingParams const& params, double from_t, double to_t) const
{
	struct Sink : public VirtualFunction3<void, QPointF, double, SampleFlags>
	{
		std::vector<QPointF> polyline;

		virtual void operator()(QPointF pt, double, SampleFlags) {
			polyline.push_back(pt);
		}
	};

	Sink sink;
	sample(sink, params, from_t, to_t);

	return sink.polyline;
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


/*===================== TensionDerivedParams =====================*/

// We make t lie in [0 .. 1] within a segment,
// which gives us delta == 1 and the following tk's:
double const XSpline::TensionDerivedParams::t0 = -1;
double const XSpline::TensionDerivedParams::t1 = 0;
double const XSpline::TensionDerivedParams::t2 = 1;
double const XSpline::TensionDerivedParams::t3 = 2;

static double square(double v)
{
	return v*v;
}

XSpline::TensionDerivedParams::TensionDerivedParams(
	double const tension1, double const tension2)
{
	// tension1, tension2 lie in [-1 .. 1]

	// Tk+ = t(k+1) + s(k+1)
	// Tk- = t(k-1) - s(k-1)
	double const s1 = std::max<double>(tension1, 0);
	double const s2 = std::max<double>(tension2, 0);
	T0p = t1 + s1;
	T1p = t2 + s2;
	T2m = t1 - s1;
	T3m = t2 - s2;

	// q's lie in [0 .. 0.5]
	double const s_1 = -0.5 * std::min<double>(tension1, 0);
	double const s_2 = -0.5 * std::min<double>(tension2, 0);
	q[0] = s_1;
	q[1] = s_2;
	q[2] = s_1;
	q[3] = s_2;

	// Formula 17 in [1]:
	p[0] = 2.0 * square(t0 - T0p);
	p[1] = 2.0 * square(t1 - T1p);
	p[2] = 2.0 * square(t2 - T2m);
	p[3] = 2.0 * square(t3 - T3m);
}


/*========================== GBlendFunc ==========================*/

XSpline::GBlendFunc::GBlendFunc(double q, double p)
:	m_c1(q), // See formula 20 in [1].
	m_c2(2 * q),
	m_c3(10 - 12 * q - p),
	m_c4(2 * p + 14 * q - 15),
	m_c5(6 - 5 * q - p)
{
}

double
XSpline::GBlendFunc::value(double u) const
{
	double const u2 = u * u;
	double const u3 = u2 * u;
	double const u4 = u3 * u;
	double const u5 = u4 * u;

	return m_c1 * u + m_c2 * u2 + m_c3 * u3 + m_c4 * u4 + m_c5 * u5;
}

double
XSpline::GBlendFunc::firstDerivative(double u) const
{
	double const u2 = u * u;
	double const u3 = u2 * u;
	double const u4 = u3 * u;

	return m_c1 + 2 * m_c2 * u + 3 * m_c3 * u2 + 4 * m_c4 * u3 + 5 * m_c5 * u4;
}

double
XSpline::GBlendFunc::secondDerivative(double u) const
{
	double const u2 = u * u;
	double const u3 = u2 * u;

	return 2 * m_c2 + 6 * m_c3 * u + 12 * m_c4 * u2 + 20 * m_c5 * u3;
}


/*========================== HBlendFunc ==========================*/

XSpline::HBlendFunc::HBlendFunc(double q)
:	m_c1(q), // See formula 20 in [1].
	m_c2(2 * q),
	m_c4(-2 * q),
	m_c5(-q)
{
}

double
XSpline::HBlendFunc::value(double u) const
{
	double const u2 = u * u;
	double const u3 = u2 * u;
	double const u4 = u3 * u;
	double const u5 = u4 * u;

	return m_c1 * u + m_c2 * u2 + m_c4 * u4 + m_c5 * u5;
}

double
XSpline::HBlendFunc::firstDerivative(double u) const
{
	double const u2 = u * u;
	double const u3 = u2 * u;
	double const u4 = u3 * u;

	return m_c1 + 2 * m_c2 * u + 4 * m_c4 * u3 + 5 * m_c5 * u4;
}

double
XSpline::HBlendFunc::secondDerivative(double u) const
{
	double const u2 = u * u;
	double const u3 = u2 * u;
	
	return 2 * m_c2 + 12 * m_c4 * u2 + 20 * m_c5 * u3;
}


/*======================== PointAndDerivs ========================*/

double
XSpline::PointAndDerivs::signedCurvature() const
{
	double const cross = firstDeriv.x()*secondDeriv.y() - firstDeriv.y()*secondDeriv.x();
	double tlen = sqrt(firstDeriv.x()*firstDeriv.x() + firstDeriv.y()*firstDeriv.y());
	return cross / (tlen * tlen * tlen);
}
