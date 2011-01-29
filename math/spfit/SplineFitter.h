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
#include "ModelShape.h"
#include <boost/dynamic_bitset.hpp>
#include <vector>

namespace spfit
{

class SplineFitter
{
	DECLARE_NON_COPYABLE(SplineFitter)
public:
	SplineFitter(FittableSpline* spline, ModelShape* model_shape,
		boost::dynamic_bitset<> const* fixed_control_points = 0);

	void setSamplingParams(FittableSpline::SamplingParams const& params) {
		m_samplingParams = params;
	}

	void fit(int max_iterations = 20);
private:
	class SampleProcessor;
	class DisplacementVectorProcessor;

	void setupInitialSamplingParams();

	void setupControlPointMappings(boost::dynamic_bitset<> const* fixed_control_points);

	int numOrigControlPoints() const { return m_toReducedControlPoints.size(); }

	int numReducedControlPoints() const { return m_toOrigControlPoints.size(); }

	FittableSpline* m_pSpline;
	ModelShape* m_pModelShape;
	FittableSpline::SamplingParams m_samplingParams;
	std::vector<int> m_toReducedControlPoints;
	std::vector<int> m_toOrigControlPoints;
};

} // namespace spfit

#endif
