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

#ifndef QUADRATIC_FUNCTION_H_
#define QUADRATIC_FUNCTION_H_

#include "MatT.h"
#include "VecT.h"
#include <stddef.h>

/**
 * A quadratic function from arbitrary number of variables
 * expressed in matrix form:
 * \code
 * F(x) = x^T * A * x + b^T * x + c
 * \endcode
 * With N being the number of variables, we have:\n
 * x: vector of N variables.\n
 * A: NxN matrix of coefficients.\n
 * b: vector of N coefficients.\n
 * c: constant component.\n
 */
class QuadraticFunction
{
	// Member-wise copying is OK.
public:
	/**
	 * Quadratic function's gradient can be written in matrix form as:
	 * \code
	 * nabla F(x) = A * x + b
	 * \endcode
	 */
	class Gradient
	{
	public:
		MatT<double> A;
		VecT<double> b;
	};

	/**
	 * Matrix A has column-major data storage, so that it can be used with MatrixCalc.
	 */
	MatT<double> A;
	VecT<double> b;
	double c;

	/**
	 * Constructs a quadratic functiono of the given number of variables,
	 * initializing everything to zero.
	 */
	QuadraticFunction(size_t num_vars = 0);

	/**
	 * Resets everything to zero, so that F(x) = 0
	 */
	void reset();

	size_t numVars() const { return b.size(); }

	/**
	 * Evaluates x^T * A * x + b^T * x + c
	 */
	double evaluate(double const* x) const;

	Gradient gradient() const;

	/**
	 * f(x) is our function.  This method will replace f(x) with g(x) so that
	 * g(x) = f(x + translation)
	 */
	void recalcForTranslatedArguments(double const* translation);

	void swap(QuadraticFunction& other);

	QuadraticFunction& operator+=(QuadraticFunction const& other);

	QuadraticFunction& operator*=(double scalar);
};


inline void swap(QuadraticFunction& f1, QuadraticFunction& f2)
{
	f1.swap(f2);
}

#endif
