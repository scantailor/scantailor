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
#include "NumericTraits.h"
#include "ToLineProjector.h"
#include <QLineF>
#include <boost/foreach.hpp>
#include <string>
#include <stdexcept>
#include <algorithm>
#include <math.h>
#include <assert.h>

namespace imageproc
{

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
	VirtualFunction2<void, QPointF, double>& sink, SamplingParams const& params) const
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

	QPointF next_pt(pointAtImpl(0, 0.0));
	sink(next_pt, 0.0);
	
	int const num_segments = numSegments();
	if (num_segments == 0) {
		return;
	}

	double const r_num_segments = 1.0 / num_segments;

	for (int segment = 0; segment < num_segments; ++segment) {
		double const prev_t = 0;
		double const next_t = 1;
		QPointF const prev_pt(next_pt);
		next_pt = pointAtImpl(segment, next_t);
		maybeAddMoreSamples(
			sink, max_sqdist_to_spline, max_sqdist_between_samples,
			r_num_segments, segment, prev_t, prev_pt, next_t, next_pt
		);
		sink(next_pt, (segment + next_t) * r_num_segments);
	}
}

void
XSpline::maybeAddMoreSamples(
	VirtualFunction2<void, QPointF, double>& sink,
	double max_sqdist_to_spline, double max_sqdist_between_samples,
	double r_num_segments, int segment,
	double prev_t, QPointF const& prev_pt,
	double next_t, QPointF const& next_pt) const
{
	double const mid_t = 0.5 * (prev_t + next_t);
	QPointF const mid_pt(pointAtImpl(segment, mid_t));
	QPointF const projection(
		ToLineProjector(QLineF(prev_pt, next_pt)).projectionPoint(mid_pt)
	);

	QPointF const v1(mid_pt - projection);
	if (v1.x() * v1.x() + v1.y() * v1.y() <= max_sqdist_to_spline) {
		QPointF const v2(next_pt - prev_pt);
		if (v2.x() * v2.x() + v2.y() * v2.y() <= max_sqdist_between_samples) {
			return;
		}
	}

	maybeAddMoreSamples(
		sink, max_sqdist_to_spline, max_sqdist_between_samples,
		r_num_segments, segment, prev_t, prev_pt, mid_t, mid_pt
	);

	sink(mid_pt, (segment + mid_t) * r_num_segments);
	
	maybeAddMoreSamples(
		sink, max_sqdist_to_spline, max_sqdist_between_samples,
		r_num_segments, segment, mid_t, mid_pt, next_t, next_pt
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

	int const num_segments = numSegments();
	assert(num_segments > 0);

	if (t == 1.0) {
		// If we went with the branch below, we would end up with
		// segment == num_segments, which is an error.
		return pointAndDtsAtImpl(num_segments - 1, 1.0);
	} else {
		double const t2 = t * num_segments;
		double const segment = floor(t2);
		return pointAndDtsAtImpl((int)segment, t2 - segment);
	}
}

XSpline::PointAndDerivs
XSpline::pointAndDtsAtImpl(int segment, double t) const
{
	ControlPoint pts[4];

	if (segment == 0) {
		pts[0] = m_controlPoints[0];
	} else {
		pts[0] = m_controlPoints[segment - 1];
	}

	pts[1] = m_controlPoints[segment];
	pts[2] = m_controlPoints[segment + 1];

	if (segment + 2 >= (int)m_controlPoints.size()) {
		pts[3] = m_controlPoints[segment + 1];
	} else {
		pts[3] = m_controlPoints[segment + 2];
	}

	TensionDerivedParams const tdp(pts[1].tension, pts[2].tension);

	Vec4d A;
	Vec4d dA; // First derivatives with respect to t.
	Vec4d ddA; // Second derivatives with respect to t.

	// Control point 0.
	{
		// u = (t - tdp.T0p) / (tdp.t0 - tdp.T0p)
		double const ta = 1.0 / (tdp.t0 - tdp.T0p);
		double const tb = -tdp.T0p * ta;
		double const u = ta * t + tb;
		
		if (t <= tdp.T0p) {
			// u(t) = ta * t + tb
			// u'(t) = ta
			// g(t) = g(u(t), <tension derived params>)
			// g'(t) = g'(u(t)) * u'(t)
			// g'(t) = g'(u(t)) * ta
			// g"(t) = g"(u(t)) * u'(t) * ta
			// g"(t) = g"(u(t)) * ta * ta
			GBlendFunc g(tdp.q[0], tdp.p[0]);
			A[0] = g.value(u);
			dA[0] = g.firstDerivative(u) * ta;
			ddA[0] = g.secondDerivative(u) * ta * ta;
		} else {
			HBlendFunc h(tdp.q[0]);
			A[0] = h.value(u);
			dA[0] = h.firstDerivative(u) * ta;
			ddA[0] = h.secondDerivative(u) * ta * ta;
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
		dA[1] = g.firstDerivative(u) * ta;
		ddA[1] = g.secondDerivative(u) * ta * ta;
	}

	// Control point 2.
	{
		// u = (t - tdp.T2m) / (tdp.t2 - tdp.T2m)
		double const ta = 1.0 / (tdp.t2 - tdp.T2m);
		double const tb = -tdp.T2m * ta;
		double const u = ta * t + tb;
		GBlendFunc g(tdp.q[2], tdp.p[2]);
		A[2] = g.value(u);
		dA[2] = g.firstDerivative(u) * ta;
		ddA[2] = g.secondDerivative(u) * ta * ta;
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
			dA[3] = g.firstDerivative(u) * ta;
			ddA[3] = g.secondDerivative(u) * ta * ta;
		} else {
			HBlendFunc h(tdp.q[3]);
			A[3] = h.value(u);
			dA[3] = h.firstDerivative(u) * ta;
			ddA[3] = h.secondDerivative(u) * ta * ta;
		}
	}
	
	double const sum = A.sum();
	double const d_sum = dA.sum();
	double const dd_sum = ddA.sum();

	PointAndDerivs pd;
#if 0
	for (int i = 0; i < 4; ++i) {
		pd.point += pts[i].pos * A[i] / sum;

		// Derivative of: A[i] / sum
		double const d1 = (dA[i] * sum - A[i] * d_sum) / (sum * sum);
		pd.firstDeriv += pts[i].pos * d1;
		
		// Derivative of: dA[i] * sum
		double const dd1 = ddA[i] * sum + dA[i] * d_sum;

		// Derivative of: A[i] * d_sum
		double const dd2 = dA[i] * d_sum + A[i] * dd_sum;

		// Derivative of: (dA[i] * sum - A[i] * d_sum) / (sum * sum)
		double const dd3 = ((dd1 - dd2) * (sum * sum) - (dA[i] * sum - A[i] * d_sum) * 2 * sum * d_sum) / (sum * sum * sum * sum);
		pd.secondDeriv += pts[i].pos * dd3;
	}
#else
	// The optimized version of the above.
	for (int i = 0; i < 4; ++i) {
		pd.point += pts[i].pos * A[i];

		double const d1 = dA[i] * sum - A[i] * d_sum;
		pd.firstDeriv += pts[i].pos * d1;
		
		// Derivative of: dA[i] * sum
		double const dd1 = ddA[i] * sum + dA[i] * d_sum;

		// Derivative of: A[i] * d_sum
		double const dd2 = dA[i] * d_sum + A[i] * dd_sum;

		double const dd3 = (dd1 - dd2) * (sum * sum) - d1 * (2 * sum * d_sum);
		pd.secondDeriv += pts[i].pos * dd3;
	}

	pd.firstDeriv /= sum * sum;
	pd.secondDeriv /= sum * sum * sum * sum;
#endif

	return pd;
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
XSpline::toPolyline(SamplingParams const& params) const
{
	struct Sink : public VirtualFunction2<void, QPointF, double>
	{
		std::vector<QPointF> polyline;

		virtual void operator()(QPointF pt, double) {
			polyline.push_back(pt);
		}
	};

	Sink sink;
	sample(sink, params);

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
XSpline::PointAndDerivs::curvature() const
{
	double curvature = firstDeriv.x()*secondDeriv.y() - firstDeriv.y()*secondDeriv.x();
	curvature /= pow(firstDeriv.x()*firstDeriv.x() + firstDeriv.y()*firstDeriv.y(), 1.5);
	return curvature;
};

} // namespace imageproc
