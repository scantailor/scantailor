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

#include "Function.h"
#include <algorithm>
#include <math.h>
#include <assert.h>

namespace adiff
{

// Note:
// 1. u in this file has the same meaning as in [1].
// 2. sparse_map(i, j) corresponds to l(i, j) in [1].

Function<2>::Function(size_t num_non_zero_vars)
: value()
, firstDerivs(num_non_zero_vars)
, secondDerivs(num_non_zero_vars)
{
}

Function<2>::Function(SparseMap<2> const& sparse_map)
: value()
, firstDerivs(sparse_map.numNonZeroElements())
, secondDerivs(sparse_map.numNonZeroElements())
{
}

Function<2>::Function(size_t arg_idx, double val, SparseMap<2> const& sparse_map)
: value(val)
, firstDerivs(sparse_map.numNonZeroElements())
, secondDerivs(sparse_map.numNonZeroElements())
{
	// An argument Xi is represented as a function:
	// f(X1, X2, ..., Xi, ...) = Xi
	// Derivatives are calculated accordingly.

	size_t const num_vars = sparse_map.numVars();

	// arg_idx row
	for (size_t i = 0; i < num_vars; ++i) {
		size_t const u = sparse_map.nonZeroElementIdx(arg_idx, i);
		if (u != sparse_map.ZERO_ELEMENT) {
			firstDerivs[u] = 1.0;
		}
	}

	// arg_idx column
	for (size_t i = 0; i < num_vars; ++i) {
		size_t const u = sparse_map.nonZeroElementIdx(i, arg_idx);
		if (u != sparse_map.ZERO_ELEMENT) {
			firstDerivs[u] = 1.0;
		}
	}
}

VecT<double>
Function<2>::gradient(SparseMap<2> const& sparse_map) const
{
	size_t const num_vars = sparse_map.numVars();
	VecT<double> grad(num_vars);

	for (size_t i = 0; i < num_vars; ++i) {
		size_t const u = sparse_map.nonZeroElementIdx(i, i);
		if (u != sparse_map.ZERO_ELEMENT) {
			grad[i] = firstDerivs[u];
		}
	}

	return grad;
}

MatT<double>
Function<2>::hessian(SparseMap<2> const& sparse_map) const
{
	size_t const num_vars = sparse_map.numVars();
	MatT<double> hess(num_vars, num_vars);
	
	for (size_t i = 0; i < num_vars; ++i) {
		for (size_t j = 0; j < num_vars; ++j) {
			double Fij = 0;
                        size_t const ij = sparse_map.nonZeroElementIdx(i, j);
			if (ij != sparse_map.ZERO_ELEMENT) {
				if (i == j) {
					Fij = secondDerivs[ij];
				} else {
                                        size_t const ii = sparse_map.nonZeroElementIdx(i, i);
                                        size_t const jj = sparse_map.nonZeroElementIdx(j, j);
					assert(ii != sparse_map.ZERO_ELEMENT && jj != sparse_map.ZERO_ELEMENT);
					Fij = 0.5 * (secondDerivs[ij] - (secondDerivs[ii] + secondDerivs[jj]));
				}
			}
			hess(i, j) = Fij;
		}
	}

	return hess;
}

void
Function<2>::swap(Function<2>& other)
{
	std::swap(value, other.value);
	firstDerivs.swap(other.firstDerivs);
	secondDerivs.swap(other.secondDerivs);
}

Function<2>&
Function<2>::operator+=(Function<2> const& other)
{
	size_t const p = firstDerivs.size();
	assert(secondDerivs.size() == p);
	assert(other.firstDerivs.size() == p);
	assert(other.secondDerivs.size() == p);

	value += other.value;
	
	for (size_t u = 0; u < p; ++u) {
		firstDerivs[u] += other.firstDerivs[u];
		secondDerivs[u] += other.secondDerivs[u];
	}

	return *this;
}

Function<2>&
Function<2>::operator-=(Function<2> const& other)
{
	size_t const p = firstDerivs.size();
	assert(secondDerivs.size() == p);
	assert(other.firstDerivs.size() == p);
	assert(other.secondDerivs.size() == p);

	value -= other.value;
	
	for (size_t u = 0; u < p; ++u) {
		firstDerivs[u] -= other.firstDerivs[u];
		secondDerivs[u] -= other.secondDerivs[u];
	}

	return *this;
}

Function<2>&
Function<2>::operator*=(double scalar)
{
	size_t const p = firstDerivs.size();
	value *= scalar;

	for (size_t u = 0; u < p; ++u) {
		firstDerivs[u] *= scalar;
	}

	return *this;
}

Function<2> operator+(Function<2> const& f1, Function<2> const& f2)
{
	size_t const p = f1.firstDerivs.size();
	assert(f1.secondDerivs.size() == p);
	assert(f2.firstDerivs.size() == p);
	assert(f2.secondDerivs.size() == p);

	Function<2> res(p);
	res.value = f1.value + f2.value;
	
	for (size_t u = 0; u < p; ++u) {
		res.firstDerivs[u] = f1.firstDerivs[u] + f2.firstDerivs[u];
		res.secondDerivs[u] = f1.secondDerivs[u] + f2.secondDerivs[u];
	}

	return res;
}

Function<2> operator-(Function<2> const& f1, Function<2> const& f2)
{
	size_t const p = f1.firstDerivs.size();
	assert(f1.secondDerivs.size() == p);
	assert(f2.firstDerivs.size() == p);
	assert(f2.secondDerivs.size() == p);

	Function<2> res(p);
	res.value = f1.value - f2.value;
	
	for (size_t u = 0; u < p; ++u) {
		res.firstDerivs[u] = f1.firstDerivs[u] - f2.firstDerivs[u];
		res.secondDerivs[u] = f1.secondDerivs[u] - f2.secondDerivs[u];
	}

	return res;
}

Function<2> operator*(Function<2> const& f1, Function<2> const& f2)
{
	size_t const p = f1.firstDerivs.size();
	assert(f1.secondDerivs.size() == p);
	assert(f2.firstDerivs.size() == p);
	assert(f2.secondDerivs.size() == p);

	Function<2> res(p);
	res.value = f1.value * f2.value;
	
	for (size_t u = 0; u < p; ++u) {
		res.firstDerivs[u] = f1.firstDerivs[u] * f2.value + f1.value * f2.firstDerivs[u];
		res.secondDerivs[u] = f1.secondDerivs[u] * f2.value +
			2.0 * f1.firstDerivs[u] * f2.firstDerivs[u] + f1.value * f2.secondDerivs[u];
	}

	return res;
}

Function<2> operator*(Function<2> const& f, double scalar)
{
	Function<2> res(f);
	res *= scalar;
	return res;
}

Function<2> operator*(double scalar, Function<2> const& f)
{
	Function<2> res(f);
	res *= scalar;
	return res;
}

Function<2> operator/(Function<2> const& num, Function<2> const& den)
{
	size_t const p = num.firstDerivs.size();
	assert(num.secondDerivs.size() == p);
	assert(den.firstDerivs.size() == p);
	assert(den.secondDerivs.size() == p);

	Function<2> res(p);
	res.value = num.value / den.value;
	
	double const den2 = den.value * den.value;
	double const den4 = den2 * den2;

	for (size_t u = 0; u < p; ++u) {
		// Derivative of: (num.value / den.value)
		double const d1 = num.firstDerivs[u] * den.value - num.value * den.firstDerivs[u];
		res.firstDerivs[u] = d1 / den2;

		// Derivative of: (num.firstDerivs[u] * den.value - num.value * den.firstDerivs[u])
		double const d2 = num.secondDerivs[u] * den.value - num.value * den.secondDerivs[u];

		// Derivative of: den2
		double const d3 = 2.0 * den.value * den.firstDerivs[u];

		// Derivative of: (d1 / den2)
		res.secondDerivs[u] = (d2 * den2 - d1 * d3) / den4;
	}

	return res;
}

} // namespace adiff
