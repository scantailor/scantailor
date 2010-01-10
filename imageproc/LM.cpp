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

#include "LM.h"
#include "LU.h"
#include <boost/math/special_functions/fpclassify.hpp>
#include <math.h>

double
LM::chiSquared(int const npts, double const* x, int const x_stride,
			   double const* a, double const* y, double const* s, LMfunc* f)
{
	double sum = 0.0;

	for (int i = 0; i < npts; ++i) {
		double d = y[i] - f->val(x + i * x_stride, a);
		d = d / s[i];
		sum = sum + (d*d);
	}

	return sum;
}

double
LM::solve(int const npts, double const* x, int const x_stride, double* a,
		  double const* y, double const* s, std::vector<bool> const& vary,
		  LMfunc* f, double lambda, double const termepsilon, int const maxiter)
{
	int const nparm = f->numParams();

	double e0 = chiSquared(npts, x, x_stride, a, y, s, f);
	bool done = false;

	// g = gradient, H = hessian, d = step to minimum
	// H d = -g, solve for d
	std::vector<double> H(nparm * nparm);
	std::vector<double> g(nparm);
	std::vector<double> d(nparm);
	std::vector<double> na(nparm);

	std::vector<double> oos2(npts);
	for (int i = 0; i < npts; ++i) {
		oos2[i] = 1.0 / (s[i] * s[i]);
	}

	int iter = 0;
	int term = 0;	// termination count test

	do {
		++iter;

		// hessian approximation
		for (int r = 0; r < nparm; ++r) {
			for( int c = 0; c < nparm; ++c) {
				for( int i = 0; i < npts; ++i) {
					if (i == 0) {
						H[r * nparm + c] = 0.0;
					}
					double const* xi = x + i * x_stride;
					H[r * nparm + c] += (oos2[i] * f->grad(xi, a, r) * f->grad(xi, a, c));
				}  //npts
			} //c
		} //r

		// boost diagonal towards gradient descent
		for (int r = 0; r < nparm; ++r) {
			H[r * nparm + r] *= (1.0 + lambda);
		}

		// gradient
		for (int r = 0; r < nparm; ++r) {
			for (int i = 0; i < npts; ++i) {
				if (i == 0) {
					g[r] = 0.0;
				}
				double const* xi = x + i * x_stride;
				g[r] += (oos2[i] * (y[i] - f->val(xi, a)) * f->grad(xi, a, r));
			}
		} //npts

		// solve H d = -g, evaluate error at new location

		LU(nparm, nparm, &H[0]).solve(1, &d[0], &g[0]);
		for (int i = 0; i < nparm; ++i) {
			na[i] = d[i] + a[i];
		}
		double e1 = chiSquared(npts, x, x_stride, &na[0], y, s, f);

		// termination test (slightly different than NR)
		if (fabs(e1 - e0) > termepsilon) {
			term = 0;
		} else {
			term++;
			if (term == 4) {
				done = true;
			}
		}

		if (iter >= maxiter) {
			done = true;
		}

		// in the C++ version, found that changing this to e1 >= e0
		// was not a good idea.
		if (e1 > e0 || boost::math::isnan(e1)) { // new location worse than before
			lambda *= 10.0;
		} else { // new location better, accept new parameters
			lambda *= 0.1;
			e0 = e1;
			for (int i = 0; i < nparm; ++i) {
				if (vary[i]) {
					a[i] = na[i];
				}
			}
		}
	} while(!done);

	return lambda;
} //solve
