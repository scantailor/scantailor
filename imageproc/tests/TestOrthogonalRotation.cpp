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

#include "OrthogonalRotation.h"
#include "BinaryImage.h"
#include "Utils.h"
#include <QImage>
#include <QRect>
#ifndef Q_MOC_RUN
#include <boost/test/auto_unit_test.hpp>
#endif

namespace imageproc
{

namespace tests
{

using namespace utils;

BOOST_AUTO_TEST_SUITE(OrthogonalRotationTestSuite);

BOOST_AUTO_TEST_CASE(test_null_image)
{
	BinaryImage const null_img;
	BOOST_CHECK(orthogonalRotation(null_img, 90).isNull());
}

BOOST_AUTO_TEST_CASE(test_full_image)
{
	static int const inp[] = {
		1, 1, 1, 1, 1, 1, 1, 1, 1,
		1, 0, 0, 0, 0, 0, 0, 1, 1,
		1, 0, 0, 0, 0, 0, 1, 0, 1,
		1, 0, 0, 0, 0, 1, 0, 0, 1,
		1, 0, 0, 0, 1, 0, 0, 0, 1,
		1, 0, 0, 1, 0, 1, 0, 0, 1,
		1, 0, 1, 0, 0, 0, 0, 0, 1,
		1, 1, 0, 0, 0, 0, 0, 0, 1,
		1, 1, 1, 1, 1, 1, 1, 1, 1
	};
	
	static int const out1[] = {
		1, 1, 1, 1, 1, 1, 1, 1, 1,
		1, 1, 0, 0, 0, 0, 0, 0, 1,
		1, 0, 1, 0, 0, 0, 0, 0, 1,
		1, 0, 0, 1, 0, 0, 0, 0, 1,
		1, 0, 0, 0, 1, 0, 0, 0, 1,
		1, 0, 0, 1, 0, 1, 0, 0, 1,
		1, 0, 0, 0, 0, 0, 1, 0, 1,
		1, 0, 0, 0, 0, 0, 0, 1, 1,
		1, 1, 1, 1, 1, 1, 1, 1, 1
	};
	
	static int const out2[] = {
		1, 1, 1, 1, 1, 1, 1, 1, 1,
		1, 0, 0, 0, 0, 0, 0, 1, 1,
		1, 0, 0, 0, 0, 0, 1, 0, 1,
		1, 0, 0, 1, 0, 1, 0, 0, 1,
		1, 0, 0, 0, 1, 0, 0, 0, 1,
		1, 0, 0, 1, 0, 0, 0, 0, 1,
		1, 0, 1, 0, 0, 0, 0, 0, 1,
		1, 1, 0, 0, 0, 0, 0, 0, 1,
		1, 1, 1, 1, 1, 1, 1, 1, 1
	};
	
	static int const out3[] = {
		1, 1, 1, 1, 1, 1, 1, 1, 1,
		1, 1, 0, 0, 0, 0, 0, 0, 1,
		1, 0, 1, 0, 0, 0, 0, 0, 1,
		1, 0, 0, 1, 0, 1, 0, 0, 1,
		1, 0, 0, 0, 1, 0, 0, 0, 1,
		1, 0, 0, 0, 0, 1, 0, 0, 1,
		1, 0, 0, 0, 0, 0, 1, 0, 1,
		1, 0, 0, 0, 0, 0, 0, 1, 1,
		1, 1, 1, 1, 1, 1, 1, 1, 1
	};
	
	BinaryImage const img(makeBinaryImage(inp, 9, 9));
	BinaryImage const out1_img(makeBinaryImage(out1, 9, 9));
	BinaryImage const out2_img(makeBinaryImage(out2, 9, 9));
	BinaryImage const out3_img(makeBinaryImage(out3, 9, 9));
	
	BOOST_REQUIRE(orthogonalRotation(img, 0) == img);
	BOOST_REQUIRE(orthogonalRotation(img, 360) == img);
	BOOST_REQUIRE(orthogonalRotation(img, 90) == out1_img);
	BOOST_REQUIRE(orthogonalRotation(img, -270) == out1_img);
	BOOST_REQUIRE(orthogonalRotation(img, 180) == out2_img);
	BOOST_REQUIRE(orthogonalRotation(img, -180) == out2_img);
	BOOST_REQUIRE(orthogonalRotation(img, 270) == out3_img);
	BOOST_REQUIRE(orthogonalRotation(img, -90) == out3_img);
}

BOOST_AUTO_TEST_CASE(test_sub_image)
{
	static int const inp[] = {
		1, 1, 1, 1, 1, 1, 1, 1, 1,
		1, 1, 1, 1, 1, 1, 1, 1, 1,
		1, 0, 0, 0, 0, 0, 0, 1, 1,
		1, 0, 0, 0, 0, 0, 1, 0, 1,
		1, 0, 0, 0, 0, 1, 0, 0, 1,
		1, 0, 0, 0, 1, 0, 0, 0, 1,
		1, 0, 0, 1, 0, 1, 0, 0, 1,
		1, 0, 1, 0, 0, 0, 0, 0, 1,
		1, 1, 0, 0, 0, 0, 0, 0, 1
	};
	
	static int const out1[] = {
		0, 0, 0, 0, 0, 0, 1,
		0, 0, 0, 0, 0, 1, 0,
		0, 0, 0, 0, 1, 0, 0,
		0, 0, 0, 1, 0, 0, 0,
		0, 0, 1, 0, 1, 0, 0,
		0, 1, 0, 0, 0, 0, 0,
		1, 0, 0, 0, 0, 0, 0
	};
	
	static int const out2[] = {
		1, 0, 0, 0, 0, 0, 0,
		0, 1, 0, 0, 0, 0, 0,
		0, 0, 1, 0, 0, 0, 0,
		0, 0, 0, 1, 0, 0, 0,
		0, 0, 1, 0, 1, 0, 0,
		0, 0, 0, 0, 0, 1, 0,
		0, 0, 0, 0, 0, 0, 1
	};
	
	static int const out3[] = {
		0, 0, 0, 0, 0, 0, 1,
		0, 0, 0, 0, 0, 1, 0,
		0, 0, 1, 0, 1, 0, 0,
		0, 0, 0, 1, 0, 0, 0,
		0, 0, 1, 0, 0, 0, 0,
		0, 1, 0, 0, 0, 0, 0,
		1, 0, 0, 0, 0, 0, 0
	};
	
	static int const out4[] = {
		1, 0, 0, 0, 0, 0, 0,
		0, 1, 0, 0, 0, 0, 0,
		0, 0, 1, 0, 1, 0, 0,
		0, 0, 0, 1, 0, 0, 0,
		0, 0, 0, 0, 1, 0, 0,
		0, 0, 0, 0, 0, 1, 0,
		0, 0, 0, 0, 0, 0, 1
	};
	
	QRect const rect(1, 2, 7, 7);
	BinaryImage const img(makeBinaryImage(inp, 9, 9));
	BinaryImage const out1_img(makeBinaryImage(out1, 7, 7));
	BinaryImage const out2_img(makeBinaryImage(out2, 7, 7));
	BinaryImage const out3_img(makeBinaryImage(out3, 7, 7));
	BinaryImage const out4_img(makeBinaryImage(out4, 7, 7));
	
	BOOST_REQUIRE(orthogonalRotation(img, rect, 0) == out1_img);
	BOOST_REQUIRE(orthogonalRotation(img, rect, 360) == out1_img);
	BOOST_REQUIRE(orthogonalRotation(img, rect, 90) == out2_img);
	BOOST_REQUIRE(orthogonalRotation(img, rect, -270) == out2_img);
	BOOST_REQUIRE(orthogonalRotation(img, rect, 180) == out3_img);
	BOOST_REQUIRE(orthogonalRotation(img, rect, -180) == out3_img);
	BOOST_REQUIRE(orthogonalRotation(img, rect, 270) == out4_img);
	BOOST_REQUIRE(orthogonalRotation(img, rect, -90) == out4_img);
}

BOOST_AUTO_TEST_SUITE_END();

} // namespace tests

} // namespace imageproc
