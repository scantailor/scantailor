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

namespace spfit
{

void
SqDistApproximant::initWithWeightedPointDistance(Vec2d const& pt, double weight)
{
	A(0, 0) = weight;
	A(0, 1) = 0;
	A(1, 0) = 0;
	A(1, 1) = weight;

	b[0] = -2.0 * weight * pt[0];
	b[1] = -2.0 * weight * pt[1];
	
	c = weight * (pt[0] * pt[0] + pt[1] * pt[1]);
}

double
SqDistApproximant::evaluate(Vec2d const& pt) const
{
	StaticMatrixCalc<double, 8, 1> mc;
	return (mc(pt, 1, 2)*mc(A)*mc(pt, 2, 1) + mc(b, 1, 2)*mc(pt, 2, 1)).rawData()[0] + c;
}

} // namespace spfit
