/*
    Scan Tailor - Interactive post-processing tool for scanned pages.
    Copyright (C) 2007-2008  Joseph Artsimovich <joseph_a@mail.ru>

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

#include "ReduceThreshold.h"
#include "BinaryImage.h"
#include "Utils.h"
#include <QImage>
#ifndef Q_MOC_RUN
#include <boost/test/auto_unit_test.hpp>
#endif

namespace imageproc
{

namespace tests
{

using namespace utils;

BOOST_AUTO_TEST_SUITE(ReduceThresholdTestSuite);

BOOST_AUTO_TEST_CASE(test_null_image)
{
	BOOST_CHECK(ReduceThreshold(BinaryImage())(2).image().isNull());
}

BOOST_AUTO_TEST_CASE(test_small_image)
{
	int const inp[] = {
		0, 0,  1, 0,  0, 1,  1, 1,  0, 0,
		0, 0,  0, 0,  0, 0,  0, 0,  0, 0,
		
		0, 0,  1, 0,  0, 1,  1, 1,  1, 0,
		1, 0,  1, 0,  1, 0,  1, 0,  1, 0,
		
		0, 0,  1, 0,  0, 1,  1, 1,  0, 1,
		0, 1,  0, 1,  0, 1,  0, 1,  0, 1,
		
		0, 0,  1, 0,  0, 1,  1, 1,  1, 1,
		1, 1,  1, 1,  1, 1,  1, 1,  1, 1
	};
	int const out1[] = {
		0, 1, 1, 1, 0,
		1, 1, 1, 1, 1,
		1, 1, 1, 1, 1,
		1, 1, 1, 1, 1
	};
	int const out2[] = {
		0, 0, 0, 1, 0,
		0, 1, 1, 1, 1,
		0, 1, 1, 1, 1,
		1, 1, 1, 1, 1
	};
	int const out3[] = {
		0, 0, 0, 0, 0,
		0, 0, 0, 1, 0,
		0, 0, 0, 1, 0,
		0, 1, 1, 1, 1
	};
	int const out4[] = {
		0, 0, 0, 0, 0,
		0, 0, 0, 0, 0,
		0, 0, 0, 0, 0,
		0, 0, 0, 1, 1
	};
	
	BinaryImage const img(makeBinaryImage(inp, 10, 8));
	
	BOOST_CHECK(makeBinaryImage(out1, 5, 4) == ReduceThreshold(img)(1));
	BOOST_CHECK(makeBinaryImage(out2, 5, 4) == ReduceThreshold(img)(2));
	BOOST_CHECK(makeBinaryImage(out3, 5, 4) == ReduceThreshold(img)(3));
	BOOST_CHECK(makeBinaryImage(out4, 5, 4) == ReduceThreshold(img)(4));
}

BOOST_AUTO_TEST_CASE(test_lines)
{
	static int const inp[] = {
		0, 0, 0, 1, 1, 0, 1, 1, 0
	};
	static int const out1[] = {
		0, 1, 1, 1
	};
	static int const out2[] = {
		0, 1, 1, 1
	};
	static int const out3[] = {
		0, 0, 0, 1
	};
	static int const out4[] = {
		0, 0, 0, 1
	};
	
	BinaryImage img(makeBinaryImage(inp, 9, 1));
	
	BOOST_CHECK(makeBinaryImage(out1, 4, 1) == ReduceThreshold(img)(1));
	BOOST_CHECK(makeBinaryImage(out2, 4, 1) == ReduceThreshold(img)(2));
	BOOST_CHECK(makeBinaryImage(out3, 4, 1) == ReduceThreshold(img)(3));
	BOOST_CHECK(makeBinaryImage(out4, 4, 1) == ReduceThreshold(img)(4));
	
	img = makeBinaryImage(inp, 1, 9);
	
	BOOST_CHECK(makeBinaryImage(out1, 1, 4) == ReduceThreshold(img)(1));
	BOOST_CHECK(makeBinaryImage(out2, 1, 4) == ReduceThreshold(img)(2));
	BOOST_CHECK(makeBinaryImage(out3, 1, 4) == ReduceThreshold(img)(3));
	BOOST_CHECK(makeBinaryImage(out4, 1, 4) == ReduceThreshold(img)(4));
}

BOOST_AUTO_TEST_SUITE_END();

} // namespace tests

} // namespace imageproc
