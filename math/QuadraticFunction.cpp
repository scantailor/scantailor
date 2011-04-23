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

#include "QuadraticFunction.h"
#include <algorithm>
#include <stddef.h>
#include <assert.h>

QuadraticFunction::QuadraticFunction(size_t num_vars)
:	A(num_vars, num_vars),
	b(num_vars),
	c(0)
{
}

void
QuadraticFunction::reset()
{
	A.fill(0);
	b.fill(0);
	c = 0;
}

double
QuadraticFunction::evaluate(double const* x) const
{
	size_t const num_vars = numVars();

	double sum = c;
	for (size_t i = 0; i < num_vars; ++i) {
		sum += b[i] * x[i];
		for (size_t j = 0; j < num_vars; ++j) {
			sum += x[i] * x[j] * A(i, j); 
		}
	}
	
	return sum;
}

QuadraticFunction::Gradient
QuadraticFunction::gradient() const
{
	size_t const num_vars = numVars();
	Gradient grad;
	
	MatT<double>(num_vars, num_vars).swap(grad.A);
	for (size_t i = 0; i < num_vars; ++i) {
		for (size_t j = 0; j < num_vars; ++j) {
			grad.A(i, j) = A(i, j) + A(j, i);
		}
	}

	grad.b = b;
	return grad;
}

void
QuadraticFunction::recalcForTranslatedArguments(double const* translation)
{
	size_t const num_vars = numVars();

	for (size_t i = 0; i < num_vars; ++i) {
		// Bi * (Xi + Ti) = Bi * Xi + Bi * Ti
		c += b[i] * translation[i];
	}

	for (size_t i = 0; i < num_vars; ++i) {
		for (size_t j = 0; j < num_vars; ++j) {
			// (Xi + Ti)*Aij*(Xj + Tj) = Xi*Aij*Xj + Aij*Tj*Xi + Aij*Ti*Xj + Aij*Ti*Tj
			double const a = A(i, j);
			b[i] += a * translation[j];
			b[j] += a * translation[i];
			c += a * translation[i] * translation[j];
		}
	}
}

void
QuadraticFunction::swap(QuadraticFunction& other)
{
	A.swap(other.A);
	b.swap(other.b);
	std::swap(c, other.c);
}

QuadraticFunction&
QuadraticFunction::operator+=(QuadraticFunction const& other)
{
	A += other.A;
	b += other.b;
	c += other.c;
	return *this;
}

QuadraticFunction&
QuadraticFunction::operator*=(double scalar)
{
	A *= scalar;
	b *= scalar;
	c *= scalar;
	return *this;
}
