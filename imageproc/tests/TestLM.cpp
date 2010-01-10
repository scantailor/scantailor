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

#include "LM.h"
#include <boost/test/auto_unit_test.hpp>
#include <vector>
#include <math.h>

namespace imageproc
{

namespace tests
{

BOOST_AUTO_TEST_SUITE(LMTestSuite);

BOOST_AUTO_TEST_CASE(test_sine)
{
	static int const PHASE = 0;
	static int const AMP = 1;
	static int const FREQ = 2;

	class SineFunc : public LMfunc
	{
	public:
		virtual int numParams() const { return 3; }

		virtual double val(double const* x, double const* a) const {
			return a[AMP] * sin(a[FREQ] * x[0] + a[PHASE]);
		}

		virtual double grad(double const* x, double const* a, int a_k) const {
			if (a_k == AMP) {
				return sin(a[FREQ] * x[0] + a[PHASE]);
			} else if (a_k == FREQ) {
				return a[AMP] * cos(a[FREQ]*x[0] + a[PHASE]) * x[0];
			} else if (a_k == PHASE) {
				return a[AMP] * cos(a[FREQ]*x[0] + a[PHASE]);
			} else {
				assert(!"Unreachable");
			}
		}
	};

	SineFunc f;

	int const nparm = f.numParams();
	std::vector<double> a(nparm);
	a[PHASE] = 0.111;
	a[AMP] = 1.222;
	a[FREQ] = 1.333;

	int npts = 10;
	std::vector<double> x(npts);
	std::vector<double> y(npts);
	std::vector<double> s(npts);
	for (int i = 0; i < npts; ++i) {
		x[i] = (double)i / npts;
		y[i] = f.val(&x[i], &a[0]);
		s[i] = 1.;
	}

	std::vector<bool> vary(nparm, true);

	// Initial values.
	std::vector<double> res(nparm);
	res[PHASE] = 0.0;
	res[AMP] = 1.0;
	res[FREQ] = 1.0;

	LM::solve(npts, &x[0], 1, &res[0], &y[0], &s[0], vary, &f, 0.001, 0.01, 100);

	for (int i = 0; i < nparm; ++i) {
		BOOST_CHECK_CLOSE(res[i], a[i], 0.001);
	}
}

BOOST_AUTO_TEST_SUITE_END();

} // namespace tests

} // namespace imageproc
