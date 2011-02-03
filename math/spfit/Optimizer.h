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

#ifndef SPFIT_OPTIMIZER_H_
#define SPFIT_OPTIMIZER_H_

#include "FittableSpline.h"
#include "SqDistApproximant.h"
#include "VirtualFunction.h"
#include "VecNT.h"
#include <boost/multi_array.hpp>
#include <vector>

namespace spfit
{

class Optimizer
{
public:
	Optimizer(int num_control_points);

	void reset();

	void addSample(Vec2d const& spline_point,
		std::vector<FittableSpline::LinearCoefficient> const& coeffs,
		SqDistApproximant const& sqdist_approx);

	void optimize(VirtualFunction2<void, int, Vec2d>& displacement_vectors_sink);

	double origTotalSqDist() const { return m_origTotalSqDist; }

	double optimizedTotalSqDist() const { return m_optimizedTotalSqDist; }
private:
	void matrixMakeSymmetric();

	double m_origTotalSqDist;
	double m_optimizedTotalSqDist;
	boost::multi_array<double, 2> m_A;
	std::vector<double> m_b;
	int m_numControlPoints;
	
};

} // namespace spfit

#endif
