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
#include "VecNT.h"
#include "ToLineProjector.h"
#include <QPointF>
#include <QLineF>
#ifndef Q_MOC_RUN
#include <boost/test/auto_unit_test.hpp>
#include <boost/test/floating_point_comparison.hpp>
#endif
#include <stdlib.h>
#include <math.h>

namespace spfit
{

namespace tests
{

BOOST_AUTO_TEST_SUITE(SqDistApproximantTestSuite);

static double const PI = 3.14159265;

static double frand(double from, double to)
{
	double const rand_0_1 = rand() / double(RAND_MAX);
	return from + (to - from) * rand_0_1;
}

BOOST_AUTO_TEST_CASE(test_point_distance)
{
	for (int i = 0; i < 100; ++i) {
		Vec2d const origin(frand(-50, 50), frand(-50, 50));
		SqDistApproximant const approx(SqDistApproximant::pointDistance(origin));
		for (int j = 0; j < 10; ++j) {
			Vec2d const pt(frand(-50, 50),  frand(-50, 50));
			double const control = (pt - origin).squaredNorm();
			BOOST_REQUIRE_CLOSE(approx.evaluate(pt), control, 1e-06);
		}
	}
}

BOOST_AUTO_TEST_CASE(test_line_distance)
{
	for (int i = 0; i < 100; ++i) {
		Vec2d const pt1(frand(-50, 50), frand(-50, 50));
		double const angle = frand(0, 2.0 * PI);
		Vec2d const delta(cos(angle), sin(angle));
		QLineF const line(pt1, pt1 + delta);
		SqDistApproximant const approx(SqDistApproximant::lineDistance(line));
		ToLineProjector const proj(line);
		for (int j = 0; j < 10; ++j) {
			Vec2d const pt(frand(-50, 50), frand(-50, 50));
			double const control = proj.projectionSqDist(pt);
			BOOST_REQUIRE_CLOSE(approx.evaluate(pt), control, 1e-06);
		}
	}
}

BOOST_AUTO_TEST_CASE(test_general_case)
{
	for (int i = 0; i < 100; ++i) {
		Vec2d const origin(frand(-50, 50), frand(-50, 50));
		double const angle = frand(0, 2.0 * PI);
		Vec2d const u(cos(angle), sin(angle));
		Vec2d v(-u[1], u[0]);
		if (rand() & 1) {
			v = -v;
		}
		double const m = frand(0, 3);
		double const n = frand(0, 3);

		SqDistApproximant const approx(origin, u, v, m, n);

		for (int j = 0; j < 10; ++j) {
			Vec2d const pt(frand(-50, 50), frand(-50, 50));
			double const u_proj = u.dot(pt - origin);
			double const v_proj = v.dot(pt - origin);
			double const control = m * u_proj * u_proj + n * v_proj * v_proj;
			BOOST_REQUIRE_CLOSE(approx.evaluate(pt), control, 1e-06);
		}
	}
}

BOOST_AUTO_TEST_SUITE_END();

} // namespace tests

} // namespace spfit
