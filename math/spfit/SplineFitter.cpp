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
#include "OptimizationResult.h"
#include "Optimizer.h"
#include "ConstraintSet.h"
#include "ModelShape.h"
#include "VirtualFunction.h"
#include <boost/foreach.hpp>
#include <stdexcept>

namespace spfit
{

SplineFitter::SplineFitter(FittableSpline* spline)
: m_pSpline(spline)
, m_optimizer(spline->numControlPoints() * 2)
// Each control point is a pair of (x, y) variables.
{
}

void
SplineFitter::splineModified()
{
	Optimizer(m_pSpline->numControlPoints() * 2).swap(m_optimizer);
}

void
SplineFitter::setConstraints(ConstraintSet const& constraints)
{
	m_optimizer.setConstraints(constraints.constraints());
}

void
SplineFitter::setSamplingParams(FittableSpline::SamplingParams const& params)
{
	m_samplingParams = params;
}

void
SplineFitter::addAttractionForce(
	Vec2d const& spline_point,
	std::vector<FittableSpline::LinearCoefficient> const& coeffs,
	SqDistApproximant const& sqdist_approx)
{
	int const num_coeffs = coeffs.size();
	int const num_vars = num_coeffs * 2;
	QuadraticFunction f(num_vars);

	// Right now we basically have F(x) = Q(L(x)),
	// where Q is a quadratic function represented by sqdist_approx,
	// and L is a vector of linear combinations of control points,
	// represented by coeffs.  L[0] = is a linear combination of x coordinats
	// of control points and L[1] is a linear combination of y coordinates of
	// control points.  What we are after is F(x) = Q(x), that is a quadratic
	// function of control points.  We consider control points to be a flat
	// vector of variables, with the following layout: [x0 y0 x1 y1 x2 y2 ...]
	
	// First deal with the quadratic portion of our function.
	for (int i = 0; i < 2; ++i) {
		for (int j = 0; j < 2; ++j) {
			// Here we have Li * Aij * Lj, which gives us a product of
			// two linear functions times a constant.
			// L0 is a linear combination of x components: L0 = [c1 0  c2  0 ...]
			// L1 is a linear combination of y components: L1 = [ 0 c1  0 c2 ...]
			// Note that the same coefficients are indeed present in both L0 and L1.
			double const a = sqdist_approx.A(i, j);

			// Now let's multiply Li by Lj
			for (int m = 0; m < num_coeffs; ++m) {
				double const c1 = coeffs[m].coeff;
				int const Li_idx = m * 2 + i;
				for (int n = 0; n < num_coeffs; ++n) {
					double const c2 = coeffs[n].coeff;
					int const Lj_idx = n * 2 + j;
					f.A(Li_idx, Lj_idx) += a * c1 * c2;
				}
			}
		}
	}
	
	// Moving on to the linear part of the function.
	for (int i = 0; i < 2; ++i) {
		// Here we have Li * Bi, that is a linear function times a constant.
		double const b = sqdist_approx.b[i];
		for (int m = 0; m < num_coeffs; ++m) {
			int const Li_idx = m * 2 + i;
			f.b[Li_idx] += b * coeffs[m].coeff;
		}
	}

	// The constant part is easy.
	f.c = sqdist_approx.c;

	// What we've got at this point is a function of control point positions.
	// What we need however, is a function from control point displacements.
	m_tempVars.resize(num_vars);
	for (int i = 0; i < num_coeffs; ++i) {
		int const cp_idx = coeffs[i].controlPointIdx;
		QPointF const cp(m_pSpline->controlPointPosition(cp_idx));
		m_tempVars[i * 2] = cp.x();
		m_tempVars[i * 2 + 1] = cp.y();
	}
	f.recalcForTranslatedArguments(num_vars ? &m_tempVars[0] : 0);

	// What remains is a mapping from the reduced set of variables to the full set.
	m_tempSparseMap.resize(num_vars);
	for (int i = 0; i < num_coeffs; ++i) {
		m_tempSparseMap[i * 2] = coeffs[i].controlPointIdx * 2;
		m_tempSparseMap[i * 2 + 1] = coeffs[i].controlPointIdx * 2 + 1;
	}

	m_optimizer.addExternalForce(f, m_tempSparseMap);
}

void
SplineFitter::addAttractionForces(ModelShape const& model_shape, double from_t, double to_t)
{
	class SampleProcessor :
		public VirtualFunction3<void, QPointF, double, FittableSpline::SampleFlags>
	{
	public:
		SampleProcessor(SplineFitter& owner, ModelShape const& model_shape)
			: m_rOwner(owner), m_rModelShape(model_shape) {}

		virtual void operator()(QPointF pt, double t, FittableSpline::SampleFlags flags) {
			m_rOwner.m_pSpline->linearCombinationAt(t, m_rOwner.m_tempCoeffs);
			SqDistApproximant const approx(m_rModelShape.localSqDistApproximant(pt, flags));
			m_rOwner.addAttractionForce(pt, m_rOwner.m_tempCoeffs, approx);
		}
	private:
		SplineFitter& m_rOwner;
		ModelShape const& m_rModelShape;
	};

	SampleProcessor sample_processor(*this, model_shape);
	m_pSpline->sample(sample_processor, m_samplingParams, from_t, to_t);
}

void
SplineFitter::addExternalForce(QuadraticFunction const& force)
{
	m_optimizer.addExternalForce(force);
}

void
SplineFitter::addExternalForce(QuadraticFunction const& force, std::vector<int> const& sparse_map)
{
	m_optimizer.addExternalForce(force, sparse_map);
}

void
SplineFitter::addInternalForce(QuadraticFunction const& force)
{
	m_optimizer.addInternalForce(force);
}

void
SplineFitter::addInternalForce(
	QuadraticFunction const& force, std::vector<int> const& sparse_map)
{
	m_optimizer.addInternalForce(force, sparse_map);
}

OptimizationResult
SplineFitter::optimize(double internal_force_weight)
{
	OptimizationResult const res(m_optimizer.optimize(internal_force_weight));
	
	int const num_control_points = m_pSpline->numControlPoints();
	for (int i = 0; i < num_control_points; ++i) {
		Vec2d const delta(m_optimizer.displacementVector() + i * 2);
		m_pSpline->moveControlPoint(i, m_pSpline->controlPointPosition(i) + delta);
	}

	return res;
}

void
SplineFitter::undoLastStep()
{
	int const num_control_points = m_pSpline->numControlPoints();
	for (int i = 0; i < num_control_points; ++i) {
		Vec2d const delta(m_optimizer.displacementVector() + i * 2);
		m_pSpline->moveControlPoint(i, m_pSpline->controlPointPosition(i) - delta);
	}

	m_optimizer.undoLastStep(); // Zeroes the displacement vector among other things.
}

} // namespace spfit
