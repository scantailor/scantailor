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

#include "LU.h"
#include <boost/test/auto_unit_test.hpp>

namespace imageproc
{

namespace tests
{

BOOST_AUTO_TEST_SUITE(LUTestSuite);

BOOST_AUTO_TEST_CASE(test)
{
	static double const mat[] = {
		1, 1, 1,
		2, 4, -3,
		3, 6, -5
	};

	static double const vec[] = {
		9, 1, 0
	};

	static double const control[] = {
		7, -1, 3
	};

	double res[3];
	LU(3, 3, mat).solve(1, res, vec);

	for (int i = 0; i < 3; ++i) {
		BOOST_CHECK_CLOSE(res[i], control[i], 1e-6);
	}
}

BOOST_AUTO_TEST_SUITE_END();

} // namespace tests

} // namespace imageproc
