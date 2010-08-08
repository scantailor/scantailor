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

#include "MatrixCalc.h"
#include <boost/test/auto_unit_test.hpp>
#include <boost/test/floating_point_comparison.hpp>

namespace imageproc
{

namespace tests
{

BOOST_AUTO_TEST_SUITE(MatrixCalcSuite);

BOOST_AUTO_TEST_CASE(test1)
{
	static double const A[] = {
		1, 1, 1,
		2, 4, -3,
		3, 6, -5
	};

	static double const B[] = {
		9, 1, 0
	};

	static double const control[] = {
		7, -1, 3
	};

	double x[3];

	MatrixCalc<double> mc;
	mc(A, 3, 3).trans().solve(mc(B, 3, 1)).write(x);

	for (int i = 0; i < 3; ++i) {
		BOOST_REQUIRE_CLOSE(x[i], control[i], 1e-6);
	}
}

BOOST_AUTO_TEST_CASE(test2)
{
	static double const A[] = {
		1, 1, 1,
		2, 4, -3,
		3, 6, -5,
		3, 5, -2,
		5, 10, -8
	};

	double B[] = {
		9, 1, 0, 10, 1
	};

	static double const control[] = {
		7, -1, 3
	};

	double x[3];

	MatrixCalc<double> mc;
	mc(A, 3, 5).trans().solve(mc(B, 5, 1)).write(x);

	for (int i = 0; i < 3; ++i) {
		BOOST_REQUIRE_CLOSE(x[i], control[i], 1e-6);
	}

	// Now make the system inconsistent.
	B[4] += 1.0;
	BOOST_CHECK_THROW(mc(A, 3, 5).trans().solve(mc(B, 5, 1)), std::runtime_error);

}

BOOST_AUTO_TEST_CASE(test3)
{
	static double const A[] = {
		1, 3, 1,
		1, 1, 2,
		2, 3, 4
	};

	static double const control[] = {
		2, 9, -5,
		0, -2, 1,
		-1, -3, 2
	};

	double inv[9];

	MatrixCalc<double> mc;
	mc(A, 3, 3).trans().inv().transWrite(inv);

	for (int i = 0; i < 9; ++i) {
		BOOST_REQUIRE_CLOSE(inv[i], control[i], 1e-6);
	}
}

BOOST_AUTO_TEST_CASE(test4)
{
	static double const A[] = {
		4, 1, 9,
		6, 2, 8,
		7, 3, 5,
		11, 10, 12
	};

	static double const B[] = {
		2, 9,
		5, 12,
		8, 10
	};

	static double const control[] = {
		85, 138,
		86, 158,
		69, 149,
		168, 339
	};

	double mul[8];

	MatrixCalc<double> mc;
	(mc(A, 3, 4).trans()*(mc(B, 2, 3).trans())).transWrite(mul);

	for (int i = 0; i < 8; ++i) {
		BOOST_REQUIRE_CLOSE(mul[i], control[i], 1e-6);
	}
}

BOOST_AUTO_TEST_SUITE_END();

} // namespace tests

} // namespace imageproc
