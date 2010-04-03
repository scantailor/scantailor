/*
	Levenberg-Marquardt C++ implementation.
	Ported from the Java version:
	http://www.idiom.com/~zilla/Computer/Javanumeric/LM.java

	initial author contact info:
	jplewis  www.idiom.com/~zilla  zilla # computer.org,   #=at

	Improvements by:
	dscherba  www.ncsa.uiuc.edu/~dscherba
	Jonathan Jackson   j.jackson # ucl.ac.uk

	Ported to C++ by:
	Joseph Artsimovich <joseph.artsimovich@gmail.com>

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

#ifndef LM_H_
#define LM_H_

#include <QSize>
#include <vector>

class LMfunc
{
public:
	virtual ~LMfunc() {}

	virtual int numParams() const = 0;

	/**
	 * x is a single point, but domain may be mulidimensional.
	 */
	virtual double val(double const* x, double const* a) const = 0;

	/**
	 * Return the kth component of the gradient df(x,a)/da_k
	 */
	virtual double grad(double const* x, double const* a, int ak) const = 0;
};


/**
 * Levenberg-Marquardt, implemented from the general description
 * in Numerical Recipes (NR), then tweaked slightly to mostly
 * match the results of their code.
 * Use for nonlinear least squares assuming Gaussian errors.
 *
 * TODO this holds some parameters fixed by simply not updating them.
 * this may be ok if the number if fixed parameters is small,
 * but if the number of varying parameters is larger it would
 * be more efficient to make a smaller hessian involving only
 * the variables.
 *
 * The NR code assumes a statistical context, e.g. returns
 * covariance of parameter errors; we do not do this.
 */
class LM
{
public:
	/**
	 * calculate the current sum-squared-error
	 * (Chi-squared is the distribution of squared Gaussian errors,
	 * thus the name)
	 */
	static double chiSquared(
		int npts, double const* x, int x_stride, double const* a,
		double const* y, double const* s, LMfunc* f);

	/**
	 * Minimize E = sum {(y[k] - f(x[k],a)) / s[k]}^2
	 * The individual errors are optionally scaled by s[k].
	 * Note that LMfunc implements the value and gradient of f(x,a),
	 * NOT the value and gradient of E with respect to a!
	 *
	 * \param npts The number of data points.
	 * \param x Array of domain points, each may be multidimensional.
	 * \param x_stride Distance in \p x from one point to another.
	 * \param y Corresponding array of values.
	 * \param a The parameters/state of the model.
	 * \param s Sigma for point i.
	 * \param vary False to indicate the corresponding a[k] is to be held fixed.
	 * \param f The function to fit.
	 * \param lambda Blend between steepest descent (lambda high) and
	 *	      jump to bottom of quadratic (lambda zero). Start with 0.001.
	 * \param termepsilon Termination accuracy (0.01).
	 * \param maxiter Stop and return after this many iterations if not done.
	 *
	 * \return the new lambda for future iterations.
	 *  Can use this and maxiter to interleave the LM descent with some other
	 *  task, setting maxiter to something small.
	 */
	static double solve(
		int npts, double const* x, int x_stride, double* a, double const* y,
		double const* s, std::vector<bool> const& vary, LMfunc* f,
		double lambda, double termepsilon, int maxiter);
};

#endif
