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

#include "RastLineFinder.h"
#include <QPointF>
#include <QLineF>
#include <vector>
#include <set>
#include <boost/foreach.hpp>
#include <boost/test/auto_unit_test.hpp>

namespace imageproc
{

namespace tests
{

BOOST_AUTO_TEST_SUITE(RastLineFinderTestSuite);

static bool matchSupportPoints(std::vector<unsigned> const& idxs1, std::set<unsigned> const& idxs2)
{
	return std::set<unsigned>(idxs1.begin(), idxs1.end()) == idxs2;
}

BOOST_AUTO_TEST_CASE(test1)
{
	// 4- and 3-point lines with min_support_points == 3
	//--------------------------------------------------
	// x     x
	//   x x
	//   x x
	// x
	//--------------------------------------------------
	std::vector<QPointF> pts;
	pts.push_back(QPointF(-100, -100));
	pts.push_back(QPointF(0, 0));
	pts.push_back(QPointF(100, 100));
	pts.push_back(QPointF(200, 200));
	pts.push_back(QPointF(0, 100));
	pts.push_back(QPointF(100, 0));
	pts.push_back(QPointF(-100, 200));
	
	std::set<unsigned> line1_idxs;
	line1_idxs.insert(0);
	line1_idxs.insert(1);
	line1_idxs.insert(2);
	line1_idxs.insert(3);
	
	std::set<unsigned> line2_idxs;
	line2_idxs.insert(4);
	line2_idxs.insert(5);
	line2_idxs.insert(6);
	
	RastLineFinderParams params;
	params.setMinSupportPoints(3);
	RastLineFinder finder(pts, params);

	std::vector<unsigned> support_idxs;
	
	// line 1
	BOOST_REQUIRE(!finder.findNext(&support_idxs).isNull());
	BOOST_REQUIRE(matchSupportPoints(support_idxs, line1_idxs));

	// line2
	BOOST_REQUIRE(!finder.findNext(&support_idxs).isNull());
	BOOST_REQUIRE(matchSupportPoints(support_idxs, line2_idxs));

	// no more lines
	BOOST_REQUIRE(finder.findNext().isNull());
}

BOOST_AUTO_TEST_SUITE_END();

} // namespace tests

} // namespace imageproc
