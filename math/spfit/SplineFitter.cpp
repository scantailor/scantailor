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

#include "SplineFitter.h"
#include "Optimizer.h"
#include "VirtualFunction.h"
#include <QRectF>
#include <QLineF>
#include <boost/foreach.hpp>
#include <algorithm>
#include <stdexcept>
#include <limits>

namespace spfit
{

class SplineFitter::SampleProcessor : public VirtualFunction2<void, QPointF, double>
{
public:
	SampleProcessor(FittableSpline& spline,
		ModelShape& model_shape, Optimizer& optimizer,
		std::vector<int> const& to_reduced_control_points);

	virtual void operator()(QPointF pt, double t);
private:
	void remapAndFilterCoeffs();

	FittableSpline& m_rSpline;
	ModelShape& m_rModelShape;
	Optimizer& m_rOptimizer;
	std::vector<int> const& m_rToReducedControlPoints;
	std::vector<FittableSpline::LinearCoefficient> m_coeffs;
};


class SplineFitter::DisplacementVectorProcessor : public VirtualFunction2<void, int, Vec2d>
{
public:
	DisplacementVectorProcessor(
		FittableSpline& spline, std::vector<int> const& to_orig_control_points)
	: m_rSpline(spline), m_rToOrigControlPoints(to_orig_control_points) {}

	virtual void operator()(int control_point_idx, Vec2d displacement) {
		int const orig_idx = m_rToOrigControlPoints[control_point_idx];
		QPointF const old_pos(m_rSpline.controlPointPosition(orig_idx));
		m_rSpline.moveControlPoint(orig_idx, old_pos + displacement);
	}
private:
	FittableSpline& m_rSpline;
	std::vector<int> const& m_rToOrigControlPoints;
};


SplineFitter::SplineFitter(
	FittableSpline* spline, ModelShape* model_shape,
	boost::dynamic_bitset<> const* fixed_control_points)
:	m_pSpline(spline),
	m_pModelShape(model_shape)
{
	setupInitialSamplingParams();
	setupControlPointMappings(fixed_control_points);
}

void
SplineFitter::fit(int max_iterations)
try {
	if (numReducedControlPoints() == 0) {
		return;
	}

	Optimizer optimizer(numReducedControlPoints());

	SampleProcessor sample_processor(*m_pSpline, *m_pModelShape, optimizer, m_toReducedControlPoints);
	DisplacementVectorProcessor displacement_vector_processor(*m_pSpline, m_toOrigControlPoints);
	
	for (int i = 0; i < max_iterations; ++i, optimizer.reset()) {
		m_pSpline->sample(sample_processor, m_samplingParams);
		optimizer.optimize(displacement_vector_processor);
		
		// The termination condition was taken directly from [1].
		double improvement = optimizer.origTotalSqDist() - optimizer.optimizedTotalSqDist();
		improvement /= (optimizer.origTotalSqDist() + std::numeric_limits<double>::epsilon());
		if (improvement < 0.005) {
			break;
		}
	}
} catch (std::runtime_error const&) {
	// Probably an exception coming from LinearSolver.
}

void
SplineFitter::setupInitialSamplingParams()
{
	QRectF const bbox(m_pModelShape->boundingBox());
	QLineF const diag(bbox.topLeft(), bbox.bottomRight());

	// Ensure there will be more than 2 samples on a straight
	// segment of a spline.
	m_samplingParams.maxDistBetweenSamples = diag.length()
		/ std::max<int>(m_pSpline->numControlPoints()*2 + 1, 50);
}

void
SplineFitter::setupControlPointMappings(boost::dynamic_bitset<> const* fixed_control_points)
{
	int const num_orig_control_points = m_pSpline->numControlPoints();
	m_toReducedControlPoints.reserve(num_orig_control_points);
	for (int orig_idx = 0; orig_idx < num_orig_control_points; ++orig_idx) {
		if (fixed_control_points && fixed_control_points->test(orig_idx)) {
			m_toReducedControlPoints.push_back(-1); // No mapping.
		} else {
			m_toReducedControlPoints.push_back(m_toOrigControlPoints.size());
			m_toOrigControlPoints.push_back(orig_idx);
		}
	}
}


/*========================= SampleProcessor =============================*/

SplineFitter::SampleProcessor::SampleProcessor(
	FittableSpline& spline, ModelShape& model_shape, Optimizer& optimizer,
	std::vector<int> const& to_reduced_control_points)
:	m_rSpline(spline),
	m_rModelShape(model_shape),
	m_rOptimizer(optimizer),
	m_rToReducedControlPoints(to_reduced_control_points)
{
}

void
SplineFitter::SampleProcessor::operator()(QPointF pt, double t)
{
	m_rSpline.linearCombinationAt(t, m_coeffs);
	remapAndFilterCoeffs();

	int flags = 0;
	if (fabs(t) < 1e-5) {
		flags |= ModelShape::SPLINE_HEAD;
	} else if (fabs(t - 1) < 1e-5) {
		flags |= ModelShape::SPLINE_TAIL;
	}

	m_rOptimizer.addSample(pt, m_coeffs, m_rModelShape.localSqDistApproximant(pt, flags));
}

void
SplineFitter::SampleProcessor::remapAndFilterCoeffs()
{
	int write_idx = 0;
	BOOST_FOREACH(FittableSpline::LinearCoefficient const& coeff, m_coeffs) {
		int const mapped_idx = m_rToReducedControlPoints[coeff.controlPointIdx];
		if (mapped_idx != -1) {
			m_coeffs[write_idx] = coeff;
			m_coeffs[write_idx].controlPointIdx = mapped_idx;
			++write_idx;
		}
	}
	m_coeffs.resize(write_idx);
}

} // namespace spfit
