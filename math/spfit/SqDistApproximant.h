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

#ifndef SQDIST_APPROXIMANT_H_
#define SQDIST_APPROXIMANT_H_

#include "VecNT.h"
#include "MatMNT.h"

namespace spfit
{

/**
 * A quadratic function of the form:\n
 * F(x) = x^T * A * x + b^T * x + c\n
 * where:
 * \li x: a column vector representing a point in 2D space.
 * \li A: a 2x2 positive semidefinite matrix.
 * \li b: a 2 element column vector.
 * \li c: some constant.
 *
 * The value of the function approximates the squared distance
 * to the target model.  It's only accurate in a neighbourhood
 * of some unspecified point.
 *
 * \see formulas 6 and 7 in [2].
 */
struct SqDistApproximant
{
	Mat22d A;
	Vec2d b;
	double c;

	SqDistApproximant() : c(0) {}

	void initWithWeightedPointDistance(Vec2d const& pt, double weight = 1);

	double evaluate(Vec2d const& pt) const;
};

} // namespace spfit

#endif
