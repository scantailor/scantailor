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

#include "SqDistApproximant.h"
#include "MatrixCalc.h"
#include <assert.h>
#include <math.h>

namespace spfit
{

SqDistApproximant::SqDistApproximant(
	Vec2d const& origin, Vec2d const& u, Vec2d const& v, double m, double n)
{
	assert(fabs(u.squaredNorm() - 1.0) < 1e-06 && "u is not normalized");
	assert(fabs(v.squaredNorm() - 1.0) < 1e-06 && "v is not normalized");
	assert(fabs(u.dot(v)) < 1e-06 && "u and v are not orthogonal");

	// Consider the following equation:
	// w = R*x + c
	// w: vector in the local coordinate system.
	// R: rotation matrix.  Actually the inverse of [u v].
	// x: vector in the global coordinate system.
	// t: translation component.
	
	// R = | u1 u2 |
	//     | v1 v2 |
	Mat22d R;
	R(0, 0) = u[0];  R(0, 1) = u[1];
	R(1, 0) = v[0];  R(1, 1) = v[1];

	StaticMatrixCalc<double, 4> mc;
	Vec2d t; // Translation component.
	(-(mc(R) * mc(origin, 2, 1))).write(t);

	A(0, 0) = m * R(0, 0) * R(0, 0) + n * R(1, 0) * R(1, 0);
	A(0, 1) = A(1, 0) = m * R(0, 0) * R(0, 1) + n * R(1, 0) * R(1, 1);
	A(1, 1) = m * R(0, 1) * R(0, 1) + n * R(1, 1) * R(1, 1);
	b[0] = 2 * (m * t[0] * R(0, 0) + n * t[1] * R(1, 0));
	b[1] = 2 * (m * t[0] * R(0, 1) + n * t[1] * R(1, 1));
	c = m * t[0] * t[0] + n * t[1] * t[1];
}

SqDistApproximant
SqDistApproximant::pointDistance(Vec2d const& pt)
{
	return weightedPointDistance(pt, 1);
}

SqDistApproximant
SqDistApproximant::weightedPointDistance(Vec2d const& pt, double weight)
{
	return SqDistApproximant(pt, Vec2d(1, 0), Vec2d(0, 1), weight, weight);
}

SqDistApproximant
SqDistApproximant::lineDistance(QLineF const& line)
{
	return weightedLineDistance(line, 1);
}

SqDistApproximant
SqDistApproximant::weightedLineDistance(QLineF const& line, double weight)
{
	Vec2d u(line.p2() - line.p1());
	double const sqlen = u.squaredNorm();
	if (sqlen > 1e-6) {
		u /= sqrt(sqlen);
	} else {
		return pointDistance(line.p1());
	}

	// Unit normal to line.
	Vec2d const v(-u[1], u[0]);

	return SqDistApproximant(line.p1(), u, v, 0, weight);
}

double
SqDistApproximant::evaluate(Vec2d const& pt) const
{
	StaticMatrixCalc<double, 8, 1> mc;
	return (mc(pt, 1, 2)*mc(A)*mc(pt, 2, 1) + mc(b, 1, 2)*mc(pt, 2, 1)).rawData()[0] + c;
}

} // namespace spfit
