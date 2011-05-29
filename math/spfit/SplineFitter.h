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

#ifndef SPFIT_SPLINE_FITTER_H_
#define SPFIT_SPLINE_FITTER_H_

#include "NonCopyable.h"
#include "FittableSpline.h"
#include "Optimizer.h"
#include "VecNT.h"
#include <vector>

namespace spfit
{

class ConstraintSet;
class ModelShape;
struct SqDistApproximant;
class OptimizationResult;

class SplineFitter
{
	DECLARE_NON_COPYABLE(SplineFitter)
public:
	explicit SplineFitter(FittableSpline* spline);

	/**
	 * To be called after adding / moving / removing any of spline's control points.
	 * This will reset the optimizer, which means the current set of constraints
	 * is lost.  Any forces accumulated since the last optimize() call are lost as well.
	 */
	void splineModified();

	void setConstraints(ConstraintSet const& constraints);

	void setSamplingParams(FittableSpline::SamplingParams const& sampling_params);

	void addAttractionForce(Vec2d const& spline_point,
		std::vector<FittableSpline::LinearCoefficient> const& coeffs,
		SqDistApproximant const& sqdist_approx);

	void addAttractionForces(ModelShape const& model_shape,
		double from_t = 0.0, double to_t = 1.0);

	void addExternalForce(QuadraticFunction const& force);

	void addExternalForce(QuadraticFunction const& force, std::vector<int> const& sparse_map);

	void addInternalForce(QuadraticFunction const& force);

	void addInternalForce(QuadraticFunction const& force, std::vector<int> const& sparce_map);

	/** \see Optimizer::externalForce() */
	double externalForce() const { return m_optimizer.externalForce(); }

	/** \see Optimizer::internalForce() */
	double internalForce() const { return m_optimizer.internalForce(); }

	OptimizationResult optimize(double internal_force_weight);

	void undoLastStep();
private:
	FittableSpline* m_pSpline;
	Optimizer m_optimizer;
	FittableSpline::SamplingParams m_samplingParams;
	std::vector<double> m_tempVars;
	std::vector<int> m_tempSparseMap;
	std::vector<FittableSpline::LinearCoefficient> m_tempCoeffs;
};

} // namespace spfit

#endif
