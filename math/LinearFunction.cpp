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

#include "LinearFunction.h"
#include <algorithm>
#include <stddef.h>
#include <assert.h>

LinearFunction::LinearFunction(size_t num_vars)
: a(num_vars)
, b(0)
{
}

void
LinearFunction::reset()
{
	a.fill(0);
	b = 0;
}

double
LinearFunction::evaluate(double const* x) const
{
	size_t const num_vars = numVars();

	double sum = b;
	for (size_t i = 0; i < num_vars; ++i) {
		sum += a[i] * x[i];
	}

	return sum;
}

void
LinearFunction::swap(LinearFunction& other)
{
	a.swap(other.a);
	std::swap(b, other.b);
}

LinearFunction&
LinearFunction::operator+=(LinearFunction const& other)
{
	a += other.a;
	b += other.b;
	return *this;
}

LinearFunction&
LinearFunction::operator*=(double scalar)
{
	a *= scalar;
	b *= scalar;
	return *this;
}
