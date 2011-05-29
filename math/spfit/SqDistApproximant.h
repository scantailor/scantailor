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
#include <QLineF>

namespace spfit
{

class FrenetFrame;

/**
 * A quadratic function of the form:\n
 * F(x) = x^T * A * x + b^T * x + c\n
 * where:
 * \li x: a column vector representing a point in 2D space.
 * \li A: a 2x2 positive semidefinite matrix.
 * \li b: a 2 element column vector.
 * \li c: some constant.
 *
 * The function is meant to approximate the squared distance
 * to the target model.  It's only accurate in a neighbourhood
 * of some unspecified point.
 * The iso-curves of the function are rotated ellipses in a general case.
 *
 * \see Eq 8 in [1], Fig 4, 5 in [2].
 */
struct SqDistApproximant
{
	Mat22d A;
	Vec2d b;
	double c;

	/**
	 * Constructs a distance function that always evaluates to zero.
	 * Passing it to Optimizer::addSample() will have no effect.
	 */
	SqDistApproximant() : c(0) {}

	/**
	 * \brief The general case constructor.
	 *
	 * We have a coordinate system at \p origin with orthonormal basis formed
	 * by vectors \p u and \p v.  Given a point p in the global coordinate system,
	 * the appoximant will evaluate to:
	 * \code
	 * sqdist = m * i^2 + n * j^2;
	 * // Where i and j are projections onto u and v respectively.
	 * // More precisely:
	 * i = (p - origin) . u;
	 * j = (p - origin) . v;
	 * \endcode
	 */
	SqDistApproximant(Vec2d const& origin, Vec2d const& u, Vec2d const& v, double m, double n);

	static SqDistApproximant pointDistance(Vec2d const& pt);

	static SqDistApproximant weightedPointDistance(Vec2d const& pt, double weight);

	static SqDistApproximant lineDistance(QLineF const& line);

	static SqDistApproximant weightedLineDistance(QLineF const& line, double weight);

	static SqDistApproximant curveDistance(
		Vec2d const& reference_point, FrenetFrame const& frenet_frame, double signed_curvature);

	static SqDistApproximant weightedCurveDistance(
		Vec2d const& reference_point, FrenetFrame const& frenet_frame, double signed_curvature, double weight);

	double evaluate(Vec2d const& pt) const;
};

} // namespace spfit

#endif
