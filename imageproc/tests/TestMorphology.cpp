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

#include "Morphology.h"
#include "GrayImage.h"
#include "BinaryImage.h"
#include "BWColor.h"
#include "Utils.h"
#include <QImage>
#include <QSize>
#include <QPoint>
#ifndef Q_MOC_RUN
#include <boost/test/auto_unit_test.hpp>
#endif

namespace imageproc
{

namespace tests
{

using namespace utils;

BOOST_AUTO_TEST_SUITE(MorphologyTestSuite);

BOOST_AUTO_TEST_CASE(test_dilate_1x1)
{
	static int const inp[] = {
		0, 0, 0, 0, 0, 0, 0, 0, 1,
		0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 1, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 1, 1, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 1, 0,
		0, 0, 0, 1, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0
	};
	
	BinaryImage const img(makeBinaryImage(inp, 9, 9));
	BOOST_CHECK(dilateBrick(img, QSize(1, 1), img.rect()) == img);
}

BOOST_AUTO_TEST_CASE(test_dilate_1x1_gray)
{
	static int const inp[] = {
		0, 0, 0, 0, 0, 0, 0, 0, 2,
		0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 7, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 6, 5, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 3, 0,
		0, 0, 0, 4, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0
	};
	
	GrayImage const img(makeGrayImage(inp, 9, 9));
	BOOST_CHECK(dilateGray(img, QSize(1, 1), img.rect()) == img);
}

BOOST_AUTO_TEST_CASE(test_dilate_1x1_shift_black)
{
	static int const inp[] = {
		0, 0, 0, 0, 0, 0, 0, 0, 1,
		0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 1, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 1, 1, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 1, 0,
		0, 0, 0, 1, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0
	};
	
	static int const out[] = {
		0, 0, 0, 0, 0, 0, 0, 1, 1,
		0, 0, 0, 0, 0, 0, 0, 1, 1,
		1, 1, 0, 0, 0, 0, 0, 1, 1,
		0, 0, 0, 0, 0, 1, 0, 1, 1,
		0, 1, 0, 0, 0, 0, 0, 1, 1,
		0, 0, 0, 0, 0, 0, 0, 1, 1,
		0, 0, 0, 0, 0, 0, 0, 1, 1,
		1, 1, 1, 1, 1, 1, 1, 1, 1,
		1, 1, 1, 1, 1, 1, 1, 1, 1
	};
	
	BinaryImage const img(makeBinaryImage(inp, 9, 9));
	BinaryImage const control(makeBinaryImage(out, 9, 9));
	
	BOOST_CHECK(dilateBrick(img, QSize(1, 1), img.rect().translated(2, 2), BLACK) == control);
}

BOOST_AUTO_TEST_CASE(test_dilate_1x1_shift_gray)
{
	static int const inp[] = {
		0, 0, 0, 0, 0, 0, 0, 0, 2,
		0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 7, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 6, 5, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 3, 0,
		0, 0, 0, 4, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0
	};
	
	static int const out[] = {
		0, 0, 0, 0, 0, 0, 0, 5, 5,
		0, 0, 0, 0, 0, 0, 0, 5, 5,
		6, 5, 0, 0, 0, 0, 0, 5, 5,
		0, 0, 0, 0, 0, 3, 0, 5, 5,
		0, 4, 0, 0, 0, 0, 0, 5, 5,
		0, 0, 0, 0, 0, 0, 0, 5, 5,
		0, 0, 0, 0, 0, 0, 0, 5, 5,
		5, 5, 5, 5, 5, 5, 5, 5, 5,
		5, 5, 5, 5, 5, 5, 5, 5, 5
	};
	
	GrayImage const img(makeGrayImage(inp, 9, 9));
	GrayImage const control(makeGrayImage(out, 9, 9));
	BOOST_CHECK(dilateGray(img, QSize(1, 1), img.rect().translated(2, 2), 5) == control);
}

BOOST_AUTO_TEST_CASE(test_dilate_3x1_gray)
{
	static int const inp[] = {
		9, 4, 2, 3, 9, 9, 9, 9, 1,
		9, 9, 9, 9, 9, 9, 9, 9, 9,
		9, 9, 9, 9, 9, 9, 9, 9, 9,
		9, 3, 9, 3, 9, 9, 9, 9, 9,
		9, 9, 9, 2, 2, 9, 9, 9, 9,
		9, 9, 9, 9, 9, 9, 5, 2, 9,
		9, 9, 9, 9, 9, 9, 2, 9, 9,
		9, 9, 9, 9, 9, 9, 9, 9, 9,
		9, 9, 9, 9, 9, 9, 9, 9, 9
	};
	
	static int const out[] = {
		4, 2, 2, 2, 3, 9, 9, 1, 1,
		9, 9, 9, 9, 9, 9, 9, 9, 9,
		9, 9, 9, 9, 9, 9, 9, 9, 9,
		3, 3, 3, 3, 3, 9, 9, 9, 9,
		9, 9, 2, 2, 2, 2, 9, 9, 9,
		9, 9, 9, 9, 9, 5, 2, 2, 2,
		9, 9, 9, 9, 9, 2, 2, 2, 9,
		9, 9, 9, 9, 9, 9, 9, 9, 9,
		9, 9, 9, 9, 9, 9, 9, 9, 9
	};
	
	GrayImage const img(makeGrayImage(inp, 9, 9));
	GrayImage const control(makeGrayImage(out, 9, 9));
	BOOST_CHECK(dilateGray(img, QSize(3, 1), img.rect()) == control);
}

BOOST_AUTO_TEST_CASE(test_dilate_1x3_gray)
{
	static int const inp[] = {
		9, 4, 9, 9, 9, 9, 9, 9, 1,
		9, 9, 9, 9, 9, 9, 9, 9, 9,
		9, 2, 9, 9, 9, 9, 9, 9, 9,
		9, 3, 9, 9, 9, 9, 9, 9, 9,
		9, 9, 9, 2, 9, 9, 9, 9, 9,
		9, 9, 9, 2, 9, 9, 5, 2, 9,
		9, 9, 9, 2, 9, 9, 9, 9, 9,
		9, 9, 9, 9, 9, 9, 9, 9, 9,
		9, 9, 9, 9, 9, 9, 9, 9, 9
	};
	
	static int const out[] = {
		9, 4, 9, 9, 9, 9, 9, 9, 1,
		9, 2, 9, 9, 9, 9, 9, 9, 1,
		9, 2, 9, 9, 9, 9, 9, 9, 9,
		9, 2, 9, 2, 9, 9, 9, 9, 9,
		9, 3, 9, 2, 9, 9, 5, 2, 9,
		9, 9, 9, 2, 9, 9, 5, 2, 9,
		9, 9, 9, 2, 9, 9, 5, 2, 9,
		9, 9, 9, 2, 9, 9, 9, 9, 9,
		9, 9, 9, 9, 9, 9, 9, 9, 9
	};
	
	GrayImage const img(makeGrayImage(inp, 9, 9));
	GrayImage const control(makeGrayImage(out, 9, 9));
	BOOST_CHECK(dilateGray(img, QSize(1, 3), img.rect()) == control);
}

BOOST_AUTO_TEST_CASE(test_dilate_1x20_gray)
{
	static int const inp[] = {
		9, 4, 9, 9, 9, 9, 9, 9, 1,
		9, 9, 9, 9, 9, 9, 9, 9, 9,
		9, 2, 9, 9, 9, 9, 9, 9, 9,
		9, 3, 9, 9, 9, 9, 9, 9, 9,
		9, 9, 9, 2, 9, 9, 9, 9, 9,
		9, 9, 9, 2, 9, 9, 5, 2, 9,
		9, 9, 9, 2, 9, 9, 9, 9, 9,
		9, 9, 9, 9, 9, 9, 9, 9, 9,
		9, 9, 9, 9, 9, 9, 9, 9, 9
	};
	
	static int const out[] = {
		9, 2, 9, 2, 9, 9, 5, 2, 1,
		9, 2, 9, 2, 9, 9, 5, 2, 1,
		9, 2, 9, 2, 9, 9, 5, 2, 1,
		9, 2, 9, 2, 9, 9, 5, 2, 1,
		9, 2, 9, 2, 9, 9, 5, 2, 1,
		9, 2, 9, 2, 9, 9, 5, 2, 1,
		9, 2, 9, 2, 9, 9, 5, 2, 1,
		9, 2, 9, 2, 9, 9, 5, 2, 1,
		9, 2, 9, 2, 9, 9, 5, 2, 1
	};
	
	GrayImage const img(makeGrayImage(inp, 9, 9));
	GrayImage const control(makeGrayImage(out, 9, 9));
	BOOST_CHECK(dilateGray(img, QSize(1, 20), img.rect()) == control);
}

BOOST_AUTO_TEST_CASE(test_dilate_3x3_white)
{
	static int const inp[] = {
		0, 0, 0, 0, 0, 0, 0, 0, 1,
		0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 1, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 1, 1, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 1, 0,
		0, 0, 0, 1, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0
	};
	
	static int const out[] = {
		0, 0, 0, 0, 0, 0, 0, 1, 1,
		1, 1, 1, 0, 0, 0, 0, 1, 1,
		1, 1, 1, 0, 0, 0, 0, 0, 0,
		1, 1, 1, 1, 1, 0, 0, 0, 0,
		0, 1, 1, 1, 1, 0, 1, 1, 1,
		0, 1, 1, 1, 1, 0, 1, 1, 1,
		0, 0, 1, 1, 1, 0, 1, 1, 1,
		0, 0, 1, 1, 1, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0
	};
	
	BinaryImage const img(makeBinaryImage(inp, 9, 9));
	BinaryImage const control(makeBinaryImage(out, 9, 9));
	
	BOOST_CHECK(dilateBrick(img, QSize(3, 3), img.rect(), WHITE) == control);
}

BOOST_AUTO_TEST_CASE(test_dilate_3x3_gray)
{
	static int const inp[] = {
		9, 4, 9, 9, 9, 9, 9, 9, 1,
		9, 9, 9, 9, 9, 9, 9, 9, 9,
		9, 2, 9, 9, 9, 9, 9, 9, 9,
		9, 3, 9, 9, 9, 9, 9, 9, 9,
		9, 9, 9, 2, 9, 9, 9, 9, 9,
		9, 9, 9, 2, 9, 9, 5, 2, 9,
		9, 9, 9, 2, 9, 9, 9, 9, 9,
		9, 9, 9, 9, 9, 9, 9, 9, 9,
		9, 9, 9, 9, 9, 9, 9, 9, 9
	};
	
	static int const out[] = {
		4, 4, 4, 9, 9, 9, 9, 1, 1,
		2, 2, 2, 9, 9, 9, 9, 1, 1,
		2, 2, 2, 9, 9, 9, 9, 9, 9,
		2, 2, 2, 2, 2, 9, 9, 9, 9,
		3, 3, 2, 2, 2, 5, 2, 2, 2,
		9, 9, 2, 2, 2, 5, 2, 2, 2,
		9, 9, 2, 2, 2, 5, 2, 2, 2,
		9, 9, 2, 2, 2, 9, 9, 9, 9,
		9, 9, 9, 9, 9, 9, 9, 9, 9
	};
	
	GrayImage const img(makeGrayImage(inp, 9, 9));
	GrayImage const control(makeGrayImage(out, 9, 9));
	BOOST_CHECK(dilateGray(img, QSize(3, 3), img.rect()) == control);
}

BOOST_AUTO_TEST_CASE(test_dilate_3x3_gray_shrinked)
{
	static int const inp[] = {
		9, 4, 9, 9, 9, 9, 9, 9, 1,
		9, 9, 9, 9, 9, 9, 9, 9, 9,
		9, 2, 9, 9, 9, 9, 9, 9, 9,
		9, 3, 9, 9, 9, 9, 9, 9, 9,
		9, 9, 9, 2, 9, 9, 9, 9, 9,
		9, 9, 9, 2, 9, 9, 5, 2, 9,
		9, 9, 9, 2, 9, 9, 9, 9, 9,
		9, 9, 9, 9, 9, 9, 9, 9, 9,
		9, 9, 9, 9, 9, 9, 9, 9, 9
	};
	
	static int const out[] = {
		2, 2, 9, 9, 9, 9, 1,
		2, 2, 9, 9, 9, 9, 9,
		2, 2, 2, 2, 9, 9, 9,
		3, 2, 2, 2, 5, 2, 2,
		9, 2, 2, 2, 5, 2, 2,
		9, 2, 2, 2, 5, 2, 2,
		9, 2, 2, 2, 9, 9, 9
	};
	
	GrayImage const img(makeGrayImage(inp, 9, 9));
	GrayImage const control(makeGrayImage(out, 7, 7));
	QRect const dst_area(img.rect().adjusted(1, 1, -1, -1));
	BOOST_CHECK(dilateGray(img, QSize(3, 3), dst_area) == control);
}

BOOST_AUTO_TEST_CASE(test_open_1x2_gray)
{
	static int const inp[] = {
		9, 4, 9, 9, 9, 9, 9, 9, 1,
		9, 9, 9, 9, 9, 9, 9, 9, 9,
		9, 2, 9, 9, 9, 9, 9, 9, 9,
		9, 3, 9, 9, 9, 9, 9, 9, 9,
		9, 9, 9, 2, 9, 9, 9, 9, 9,
		9, 9, 9, 2, 9, 9, 5, 2, 9,
		9, 9, 9, 2, 9, 9, 9, 9, 9,
		9, 9, 9, 9, 9, 9, 9, 9, 9,
		1, 2, 3, 4, 5, 6, 7, 8, 9
	};

	static int const out[] = {
		9, 4, 9, 9, 9, 9, 9, 9, 1,
		9, 9, 9, 9, 9, 9, 9, 9, 9,
		9, 3, 9, 9, 9, 9, 9, 9, 9,
		9, 3, 9, 9, 9, 9, 9, 9, 9,
		9, 9, 9, 2, 9, 9, 9, 9, 9,
		9, 9, 9, 2, 9, 9, 9, 9, 9,
		9, 9, 9, 2, 9, 9, 9, 9, 9,
		9, 9, 9, 9, 9, 9, 9, 9, 9,
		1, 2, 3, 4, 5, 6, 7, 8, 9
	};

	GrayImage const img(makeGrayImage(inp, 9, 9));
	GrayImage const control(makeGrayImage(out, 9, 9));
	BOOST_CHECK(openGray(img, QSize(1, 2), 0x00) == control);
}

BOOST_AUTO_TEST_CASE(test_dilate_5x5_white)
{
	static int const inp[] = {
		0, 0, 0, 0, 0, 0, 0, 0, 1,
		0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 1, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 1, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0
	};
	
	static int const out[] = {
		1, 1, 1, 1, 0, 0, 1, 1, 1,
		1, 1, 1, 1, 0, 0, 1, 1, 1,
		1, 1, 1, 1, 0, 0, 1, 1, 1,
		1, 1, 1, 1, 0, 0, 0, 0, 0,
		1, 1, 1, 1, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 1, 1, 1, 1,
		0, 0, 0, 0, 0, 1, 1, 1, 1,
		0, 0, 0, 0, 0, 1, 1, 1, 1,
		0, 0, 0, 0, 0, 1, 1, 1, 1
	};
	
	BinaryImage const img(makeBinaryImage(inp, 9, 9));
	BinaryImage const control(makeBinaryImage(out, 9, 9));
	
	BOOST_CHECK(dilateBrick(img, QSize(5, 5), img.rect(), WHITE) == control);
}

BOOST_AUTO_TEST_CASE(test_dilate_3x3_narrowing_white)
{
	static int const inp[] = {
		0, 0, 0, 0, 0, 0, 0, 0, 1,
		0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 1, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 1, 1, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 1, 0,
		0, 0, 0, 1, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0
	};
	
	static int const out[] = {
		               0, 0, 1, 1,
		               0, 0, 1, 1,
		               0, 0, 0, 0,
		               0, 0, 0, 0,
		               0, 1, 1, 1, 
		               0, 1, 1, 1,
		               0, 1, 1, 1,
		               0, 0, 0, 0,
		               0, 0, 0, 0
	};
	
	BinaryImage const img(makeBinaryImage(inp, 9, 9));
	BinaryImage const control(makeBinaryImage(out, 4, 9));
	QRect const dst_rect(5, 0, 4, 9);
	
	BOOST_CHECK(dilateBrick(img, QSize(3, 3), dst_rect, WHITE) == control);
}

BOOST_AUTO_TEST_CASE(test_dilate_5x5_narrowing_white)
{
	static int const inp[] = {
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
	};
	
	static int const out[] = {
	//	            1, 1, 0, 0, 1, 1,
		            1, 1, 0, 0, 1, 1,
		            1, 1, 0, 0, 1, 1,
		            1, 1, 0, 0, 0, 0,
		            1, 1, 0, 0, 0, 0,
		            0, 0, 0, 1, 1, 1
	//	            0, 0, 0, 1, 1, 1,
	//	            0, 0, 0, 1, 1, 1,
	//	            0, 0, 0, 1, 1, 1
	};
	
	BinaryImage const img(makeBinaryImage(inp, 11, 9));
	BinaryImage const control(makeBinaryImage(out, 6, 5));
	QRect const dst_rect(4, 1, 6, 5);
	
	BOOST_CHECK(dilateBrick(img, QSize(5, 5), dst_rect, WHITE) == control);
}

BOOST_AUTO_TEST_CASE(test_dilate_3x3_narrowing_black)
{
	static int const inp[] = {
		0, 0, 0, 0, 0, 0, 0, 0, 1,
		0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 1, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 1, 1, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 1, 0,
		0, 0, 0, 1, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0
	};
	
	static int const out[] = {
		               1, 1, 1, 1,
		               0, 0, 1, 1,
		               0, 0, 0, 1,
		               0, 0, 0, 1,
		               0, 1, 1, 1, 
		               0, 1, 1, 1,
		               0, 1, 1, 1,
		               0, 0, 0, 1,
		               1, 1, 1, 1
	};
	
	BinaryImage const img(makeBinaryImage(inp, 9, 9));
	BinaryImage const control(makeBinaryImage(out, 4, 9));
	QRect const dst_rect(QRect(5, 0, 4, 9));
	
	BOOST_CHECK(dilateBrick(img, QSize(3, 3), dst_rect, BLACK) == control);
}

BOOST_AUTO_TEST_CASE(test_dilate_3x3_widening_white)
{
	static int const inp[] = {
		0, 0, 0, 0, 0, 0, 0, 0, 1,
		0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 1, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 1, 1, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 1, 0,
		0, 0, 0, 1, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0
	};
	
	static int const out[] = {
		0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1,
		0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1,
		0, 1, 1, 1, 0, 0, 0, 0, 1, 1, 1,
		0, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0,
		0, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0,
		0, 0, 1, 1, 1, 1, 0, 1, 1, 1, 0,
		0, 0, 1, 1, 1, 1, 0, 1, 1, 1, 0,
		0, 0, 0, 1, 1, 1, 0, 1, 1, 1, 0,
		0, 0, 0, 1, 1, 1, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	};
	
	BinaryImage const img(makeBinaryImage(inp, 9, 9));
	BinaryImage const control(makeBinaryImage(out, 11, 11));
	QRect const dst_rect(img.rect().adjusted(-1, -1, 1, 1));
	
	BOOST_CHECK(dilateBrick(img, QSize(3, 3), dst_rect, WHITE) == control);
}

BOOST_AUTO_TEST_CASE(test_dilate_3x3_widening_black)
{
	static int const inp[] = {
		0, 0, 0, 0, 0, 0, 0, 0, 1,
		0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 1, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 1, 1, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 1, 0,
		0, 0, 0, 1, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0
	};
	
	static int const out[] = {
		1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
		1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
		1, 1, 1, 1, 0, 0, 0, 0, 1, 1, 1,
		1, 1, 1, 1, 0, 0, 0, 0, 0, 1, 1,
		1, 1, 1, 1, 1, 1, 0, 0, 0, 1, 1,
		1, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1,
		1, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1,
		1, 1, 0, 1, 1, 1, 0, 1, 1, 1, 1,
		1, 1, 0, 1, 1, 1, 0, 0, 0, 1, 1,
		1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
		1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
	};
	
	BinaryImage const img(makeBinaryImage(inp, 9, 9));
	BinaryImage const control(makeBinaryImage(out, 11, 11));
	QRect const dst_rect(img.rect().adjusted(-1, -1, 1, 1));
	
	BOOST_CHECK(dilateBrick(img, QSize(3, 3), dst_rect, BLACK) == control);
}

BOOST_AUTO_TEST_CASE(test_dilate_3x1_out_of_brick_white)
{
	static int const inp[] = {
		0, 0, 0, 0, 0, 0, 0, 0, 1,
		0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 1, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 1, 1, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 1, 0,
		0, 0, 0, 1, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0
	};
	
	static int const out[] = {
		0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 1, 1, 1, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 1, 1, 1, 1, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 1,
		0, 0, 0, 0, 1, 1, 1, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0
	};
	
	BinaryImage const img(makeBinaryImage(inp, 9, 9));
	BinaryImage const control(makeBinaryImage(out, 9, 9));
	Brick const brick(QSize(3, 1), QPoint(-1, 0));
	
	BOOST_CHECK(dilateBrick(img, brick, img.rect(), WHITE) == control);
}

BOOST_AUTO_TEST_CASE(test_dilate_1x3_out_of_brick_black)
{
	static int const inp[] = {
		0, 0, 0, 0, 0, 0, 0, 0, 1,
		0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 1, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 1, 1, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 1, 0,
		0, 0, 0, 1, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0
	};
	
	static int const out[] = {
		1, 1, 1, 1, 1, 1, 1, 1, 1,
		1, 1, 1, 1, 1, 1, 1, 1, 1,
		1, 1, 1, 1, 1, 1, 1, 1, 1,
		0, 1, 0, 0, 0, 0, 0, 0, 1,
		0, 1, 0, 0, 0, 0, 0, 0, 0,
		0, 1, 1, 1, 0, 0, 0, 0, 0,
		0, 0, 1, 1, 0, 0, 0, 1, 0,
		0, 0, 1, 1, 0, 0, 0, 1, 0,
		0, 0, 0, 1, 0, 0, 0, 1, 0
	};
	
	BinaryImage const img(makeBinaryImage(inp, 9, 9));
	BinaryImage const control(makeBinaryImage(out, 9, 9));
	Brick const brick(QSize(1, 3), QPoint(0, -1));
	
	BOOST_CHECK(dilateBrick(img, brick, img.rect(), BLACK) == control);
}

BOOST_AUTO_TEST_CASE(test_large_dilate)
{
	BinaryImage img(110, 110);
	img.fill(WHITE);
	QRect const initial_rect(img.rect().center(), QSize(1, 1));
	img.fill(initial_rect, BLACK);
	
	Brick const brick(QSize(80, 80));
	QRect const extended_rect(
		initial_rect.adjusted(
			brick.minX(), brick.minY(), brick.maxX(), brick.maxY()
		)
	);
	
	BinaryImage control(img);
	control.fill(extended_rect, BLACK);
	
	BOOST_CHECK(dilateBrick(img, brick, img.rect(), WHITE) == control);
}

BOOST_AUTO_TEST_CASE(test_erode_1x1)
{
	static int const inp[] = {
		1, 1, 1, 1, 1, 1, 1, 1, 1,
		1, 1, 1, 1, 1, 1, 1, 1, 1,
		1, 1, 1, 1, 0, 1, 1, 1, 1,
		1, 1, 1, 1, 1, 1, 1, 1, 1,
		1, 1, 1, 1, 1, 1, 1, 1, 0,
		1, 1, 1, 1, 1, 1, 1, 1, 1,
		1, 1, 1, 1, 1, 1, 1, 1, 1,
		1, 1, 1, 1, 0, 0, 1, 1, 1,
		0, 1, 1, 1, 1, 1, 1, 1, 1
	};
	
	BinaryImage const img(makeBinaryImage(inp, 9, 9));
	BOOST_CHECK(erodeBrick(img, QSize(1, 1), img.rect()) == img);
}

BOOST_AUTO_TEST_CASE(test_erode_3x3_assymmetric_black)
{
	static int const inp[] = {
		1, 1, 1, 1, 1, 1, 1, 1, 1,
		1, 1, 1, 1, 1, 1, 1, 1, 1,
		1, 1, 1, 1, 0, 1, 1, 1, 1,
		1, 1, 1, 1, 1, 1, 1, 1, 1,
		1, 1, 1, 1, 1, 1, 1, 1, 0,
		1, 1, 1, 1, 1, 1, 1, 1, 1,
		1, 1, 1, 1, 1, 1, 1, 1, 1,
		1, 1, 1, 1, 0, 0, 1, 1, 1,
		0, 1, 1, 1, 1, 1, 1, 1, 1
	};
	
	static int const out[] = {
		1, 1, 1, 1, 1, 1, 1, 1, 1,
		1, 1, 1, 1, 0, 0, 0, 1, 1,
		1, 1, 1, 1, 0, 0, 0, 1, 1,
		1, 1, 1, 1, 0, 0, 0, 1, 0,
		1, 1, 1, 1, 1, 1, 1, 1, 0,
		1, 1, 1, 1, 1, 1, 1, 1, 0,
		1, 1, 1, 1, 0, 0, 0, 0, 1,
		0, 0, 0, 1, 0, 0, 0, 0, 1,
		0, 0, 0, 1, 0, 0, 0, 0, 1
	};
	
	BinaryImage const img(makeBinaryImage(inp, 9, 9));
	BinaryImage const control(makeBinaryImage(out, 9, 9));
	Brick const brick(QSize(3, 3), QPoint(0, 1));
	
	BOOST_CHECK(erodeBrick(img, brick, img.rect(), BLACK) == control);
}

BOOST_AUTO_TEST_CASE(test_erode_3x3_assymmetric_white)
{
	static int const inp[] = {
		1, 1, 1, 1, 1, 1, 1, 1, 1,
		1, 1, 1, 1, 1, 1, 1, 1, 1,
		1, 1, 1, 1, 0, 1, 1, 1, 1,
		1, 1, 1, 1, 1, 1, 1, 1, 1,
		1, 1, 1, 1, 1, 1, 1, 1, 0,
		1, 1, 1, 1, 1, 1, 1, 1, 1,
		1, 1, 1, 1, 1, 1, 1, 1, 1,
		1, 1, 1, 1, 0, 0, 1, 1, 1,
		0, 1, 1, 1, 1, 1, 1, 1, 1
	};
	
	static int const out[] = {
		0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 1, 1, 0, 0, 0, 1, 1,
		0, 0, 1, 1, 0, 0, 0, 1, 1,
		0, 0, 1, 1, 0, 0, 0, 1, 0,
		0, 0, 1, 1, 1, 1, 1, 1, 0,
		0, 0, 1, 1, 1, 1, 1, 1, 0,
		0, 0, 1, 1, 0, 0, 0, 0, 1,
		0, 0, 0, 1, 0, 0, 0, 0, 1,
		0, 0, 0, 0, 0, 0, 0, 0, 0
	};
	
	BinaryImage const img(makeBinaryImage(inp, 9, 9));
	BinaryImage const control(makeBinaryImage(out, 9, 9));
	Brick const brick(QSize(3, 3), QPoint(0, 1));
	
	BOOST_CHECK(erodeBrick(img, brick, img.rect(), WHITE) == control);
}

BOOST_AUTO_TEST_CASE(test_erode_11x11_white)
{
	static int const inp[] = {
		1, 1, 1, 1, 1, 1, 1, 1, 1,
		1, 1, 1, 1, 1, 1, 1, 1, 1,
		1, 1, 1, 1, 0, 1, 1, 1, 1,
		1, 1, 1, 1, 1, 1, 1, 1, 1,
		1, 1, 1, 1, 1, 1, 1, 1, 0,
		1, 1, 1, 1, 1, 1, 1, 1, 1,
		1, 1, 1, 1, 1, 1, 1, 1, 1,
		1, 1, 1, 1, 0, 0, 1, 1, 1,
		0, 1, 1, 1, 1, 1, 1, 1, 1
	};
	
	static int const out[] = {
		0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0
	};
	
	BinaryImage const img(makeBinaryImage(inp, 9, 9));
	BinaryImage const control(makeBinaryImage(out, 9, 9));
	Brick const brick(QSize(11, 11));
	
	BOOST_CHECK(erodeBrick(img, brick, img.rect(), WHITE) == control);
}

BOOST_AUTO_TEST_CASE(test_open_2x2_white)
{
	static int const inp[] = {
		0, 0, 0, 0, 0, 0, 0, 0, 1,
		0, 0, 0, 0, 0, 0, 0, 0, 1,
		0, 1, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 1, 1, 0, 0,
		0, 0, 0, 0, 0, 1, 1, 0, 0,
		0, 0, 1, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 1, 0, 0, 0, 0, 0,
		1, 1, 0, 0, 0, 0, 0, 0, 0,
		1, 1, 0, 0, 0, 0, 0, 0, 1
	};
	
	static int const out[] = {
		0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 1, 1, 0, 0,
		0, 0, 0, 0, 0, 1, 1, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0,
		1, 1, 0, 0, 0, 0, 0, 0, 0,
		1, 1, 0, 0, 0, 0, 0, 0, 0
	};
	
	BinaryImage const img(makeBinaryImage(inp, 9, 9));
	BinaryImage const control(makeBinaryImage(out, 9, 9));
	
	BOOST_CHECK(openBrick(img, QSize(2, 2), img.rect(), WHITE) == control);
}

BOOST_AUTO_TEST_CASE(test_open_2x2_black)
{
	static int const inp[] = {
		0, 0, 0, 0, 0, 0, 0, 0, 1,
		0, 0, 0, 0, 0, 0, 0, 0, 1,
		0, 1, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 1, 1, 0, 0,
		0, 0, 0, 0, 0, 1, 1, 0, 0,
		0, 0, 1, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 1, 0, 0, 0, 0, 0,
		1, 1, 0, 0, 0, 0, 0, 0, 0,
		1, 1, 0, 0, 0, 0, 0, 0, 1
	};
	
	static int const out[] = {
		0, 0, 0, 0, 0, 0, 0, 0, 1,
		0, 0, 0, 0, 0, 0, 0, 0, 1,
		0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 1, 1, 0, 0,
		0, 0, 0, 0, 0, 1, 1, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0,
		1, 1, 0, 0, 0, 0, 0, 0, 0,
		1, 1, 0, 0, 0, 0, 0, 0, 1
	};
	
	BinaryImage const img(makeBinaryImage(inp, 9, 9));
	BinaryImage const control(makeBinaryImage(out, 9, 9));
	
	BOOST_CHECK(openBrick(img, QSize(2, 2), img.rect(), BLACK) == control);
}

BOOST_AUTO_TEST_CASE(test_open_2x2_shifted_white)
{
	static int const inp[] = {
		0, 0, 0, 0, 0, 0, 0, 0, 1,
		0, 0, 0, 0, 0, 0, 0, 0, 1,
		0, 1, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 1, 1, 0, 0,
		0, 0, 0, 0, 0, 1, 1, 0, 0,
		0, 0, 1, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 1, 0, 0, 0, 0, 0,
		1, 1, 0, 0, 0, 0, 0, 0, 0,
		1, 1, 0, 0, 0, 0, 0, 0, 1
	};
	
	static int const out[] = {
		//     0, 0, 0, 0, 0, 0, 0, 0, 0,
		       0, 0, 0, 0, 0, 0, 0, 0, 0,
		       0, 0, 0, 0, 0, 0, 0, 0, 0,
		       0, 0, 0, 1, 1, 0, 0, 0, 0,
		       0, 0, 0, 1, 1, 0, 0, 0, 0,
		       0, 0, 0, 0, 0, 0, 0, 0, 0,
		       0, 0, 0, 0, 0, 0, 0, 0, 0,
		       0, 0, 0, 0, 0, 0, 0, 0, 0,
		       0, 0, 0, 0, 0, 0, 0, 0, 0,
		       0, 0, 0, 0, 0, 0, 0, 0, 0
	};
	
	BinaryImage const img(makeBinaryImage(inp, 9, 9));
	BinaryImage const control(makeBinaryImage(out, 9, 9));
	QRect const dst_rect(img.rect().translated(2, 1));
	
	BOOST_CHECK(openBrick(img, QSize(2, 2), dst_rect, WHITE) == control);
}

BOOST_AUTO_TEST_CASE(test_open_2x2_shifted_black)
{
	static int const inp[] = {
		0, 0, 0, 0, 0, 0, 0, 0, 1,
		0, 0, 0, 0, 0, 0, 0, 0, 1,
		0, 1, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 1, 1, 0, 0,
		0, 0, 0, 0, 0, 1, 1, 0, 0,
		0, 0, 1, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 1, 0, 0, 0, 0, 0,
		1, 1, 0, 0, 0, 0, 0, 0, 0,
		1, 1, 0, 0, 0, 0, 0, 0, 1
	};
	
	static int const out[] = {
		//    0, 0, 0, 0, 0, 0, 1, 1, 1
		      0, 0, 0, 0, 0, 0, 1, 1, 1,
		      0, 0, 0, 0, 0, 0, 0, 1, 1,
		      0, 0, 0, 1, 1, 0, 0, 1, 1,
		      0, 0, 0, 1, 1, 0, 0, 1, 1,
		      0, 0, 0, 0, 0, 0, 0, 1, 1,
		      0, 0, 0, 0, 0, 0, 0, 1, 1,
		      0, 0, 0, 0, 0, 0, 0, 1, 1,
		      0, 0, 0, 0, 0, 0, 1, 1, 1,
		      1, 1, 1, 1, 1, 1, 1, 1, 1
	};
	
	BinaryImage const img(makeBinaryImage(inp, 9, 9));
	BinaryImage const control(makeBinaryImage(out, 9, 9));
	QRect const dst_rect(img.rect().translated(2, 1));
	
	BOOST_CHECK(openBrick(img, QSize(2, 2), dst_rect, BLACK) == control);
}

BOOST_AUTO_TEST_CASE(test_open_2x2_narrowing)
{
	static int const inp[] = {
		0, 0, 0, 0, 0, 0, 0, 0, 1,
		0, 0, 0, 0, 0, 0, 0, 0, 1,
		0, 1, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 1, 1, 0, 0,
		0, 0, 0, 0, 0, 1, 1, 0, 0,
		0, 0, 1, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 1, 0, 0, 0, 0, 0,
		1, 1, 0, 0, 0, 0, 0, 0, 0,
		1, 1, 0, 0, 0, 0, 0, 0, 1
	};
	
	static int const out[] = {
		//    0, 0, 0, 0,
		//    0, 0, 0, 0
		      0, 0, 0, 0,
		      0, 0, 0, 1,
		      0, 0, 0, 1,
		      0, 0, 0, 0
	};
	
	BinaryImage const img(makeBinaryImage(inp, 9, 9));
	BinaryImage const control(makeBinaryImage(out, 4, 4));
	QRect const dst_rect(img.rect().adjusted(2, 2, -3, -3));
	
	BOOST_CHECK(openBrick(img, QSize(2, 2), dst_rect, WHITE) == control);
	BOOST_CHECK(openBrick(img, QSize(2, 2), dst_rect, BLACK) == control);
}

BOOST_AUTO_TEST_CASE(test_close_2x2_white)
{
	static int const inp[] = {
		1, 1, 1, 1, 1, 1, 1, 1, 1,
		1, 1, 1, 1, 1, 1, 1, 1, 1,
		1, 0, 0, 0, 1, 1, 1, 1, 1,
		1, 0, 0, 0, 1, 1, 1, 0, 1,
		1, 1, 1, 1, 1, 1, 1, 1, 1,
		0, 0, 0, 0, 1, 1, 1, 1, 1,
		0, 1, 0, 0, 1, 0, 0, 1, 1,
		0, 1, 0, 0, 1, 1, 1, 1, 1,
		0, 0, 0, 0, 1, 1, 1, 1, 0
	};
	
	static int const out[] = {
		1, 1, 1, 1, 1, 1, 1, 1, 1,
		1, 1, 1, 1, 1, 1, 1, 1, 1,
		1, 0, 0, 0, 1, 1, 1, 1, 1,
		1, 0, 0, 0, 1, 1, 1, 1, 1,
		1, 1, 1, 1, 1, 1, 1, 1, 1,
		0, 1, 0, 0, 1, 1, 1, 1, 1,
		0, 1, 0, 0, 1, 1, 1, 1, 1,
		0, 1, 0, 0, 1, 1, 1, 1, 1,
		0, 0, 0, 0, 1, 1, 1, 1, 0
	};
	
	BinaryImage const img(makeBinaryImage(inp, 9, 9));
	BinaryImage const control(makeBinaryImage(out, 9, 9));
	
	BOOST_CHECK(closeBrick(img, QSize(2, 2), img.rect(), WHITE) == control);
}
BOOST_AUTO_TEST_CASE(test_close_2x2_black)
{
	static int const inp[] = {
		1, 1, 1, 1, 1, 1, 1, 1, 1,
		1, 1, 1, 1, 1, 1, 1, 1, 1,
		1, 0, 0, 0, 1, 1, 1, 1, 1,
		1, 0, 0, 0, 1, 1, 1, 0, 1,
		1, 1, 1, 1, 1, 1, 1, 1, 1,
		0, 0, 0, 0, 1, 1, 1, 1, 1,
		0, 1, 0, 0, 1, 0, 0, 1, 1,
		0, 1, 0, 0, 1, 1, 1, 1, 1,
		0, 0, 0, 0, 1, 1, 1, 1, 0
	};
	
	static int const out[] = {
		1, 1, 1, 1, 1, 1, 1, 1, 1,
		1, 1, 1, 1, 1, 1, 1, 1, 1,
		1, 0, 0, 0, 1, 1, 1, 1, 1,
		1, 0, 0, 0, 1, 1, 1, 1, 1,
		1, 1, 1, 1, 1, 1, 1, 1, 1,
		1, 1, 0, 0, 1, 1, 1, 1, 1,
		1, 1, 0, 0, 1, 1, 1, 1, 1,
		1, 1, 0, 0, 1, 1, 1, 1, 1,
		1, 1, 0, 0, 1, 1, 1, 1, 1
	};
	
	BinaryImage const img(makeBinaryImage(inp, 9, 9));
	BinaryImage const control(makeBinaryImage(out, 9, 9));
	
	BOOST_CHECK(closeBrick(img, QSize(2, 2), img.rect(), BLACK) == control);
}

BOOST_AUTO_TEST_CASE(test_close_2x2_shifted_white)
{
	static int const inp[] = {
		1, 1, 1, 1, 1, 1, 1, 1, 1,
		1, 1, 1, 1, 1, 1, 1, 1, 1,
		1, 0, 0, 0, 1, 1, 1, 1, 1,
		1, 0, 0, 0, 1, 1, 1, 0, 1,
		1, 1, 1, 1, 1, 1, 1, 1, 1,
		0, 0, 0, 0, 1, 1, 1, 1, 1,
		0, 1, 0, 0, 1, 0, 0, 1, 1,
		0, 1, 0, 0, 1, 1, 1, 1, 1,
		0, 0, 0, 0, 1, 1, 1, 1, 0
	};
	
	static int const out[] = {
		//    1, 1, 1, 1, 1, 1, 1, 0, 0,
		      1, 1, 1, 1, 1, 1, 1, 0, 0,
		      0, 0, 1, 1, 1, 1, 1, 0, 0,
		      0, 0, 1, 1, 1, 1, 1, 0, 0,
		      1, 1, 1, 1, 1, 1, 1, 0, 0,
		      0, 0, 1, 1, 1, 1, 1, 0, 0,
		      0, 0, 1, 1, 1, 1, 1, 0, 0,
		      0, 0, 1, 1, 1, 1, 1, 0, 0,
		      0, 0, 1, 1, 1, 1, 0, 0, 0,
		      0, 0, 0, 0, 0, 0, 0, 0, 0
	};
	
	BinaryImage const img(makeBinaryImage(inp, 9, 9));
	BinaryImage const control(makeBinaryImage(out, 9, 9));
	QRect const dst_rect(img.rect().translated(2, 1));
	
	BOOST_CHECK(closeBrick(img, QSize(2, 2), dst_rect, WHITE) == control);
}

BOOST_AUTO_TEST_CASE(test_close_2x2_shifted_black)
{
	static int const inp[] = {
		1, 1, 1, 1, 1, 1, 1, 1, 1,
		1, 1, 1, 1, 1, 1, 1, 1, 1,
		1, 0, 0, 0, 1, 1, 1, 1, 1,
		1, 0, 0, 0, 1, 1, 1, 0, 1,
		1, 1, 1, 1, 1, 1, 1, 1, 1,
		0, 0, 0, 0, 1, 1, 1, 1, 1,
		0, 1, 0, 0, 1, 0, 0, 1, 1,
		0, 1, 0, 0, 1, 1, 1, 1, 1,
		0, 0, 0, 0, 1, 1, 1, 1, 0
	};
	
	static int const out[] = {
		//    1, 1, 1, 1, 1, 1, 1, 1, 1,
		      1, 1, 1, 1, 1, 1, 1, 1, 1,
		      0, 0, 1, 1, 1, 1, 1, 1, 1,
		      0, 0, 1, 1, 1, 1, 1, 1, 1,
		      1, 1, 1, 1, 1, 1, 1, 1, 1,
		      0, 0, 1, 1, 1, 1, 1, 1, 1,
		      0, 0, 1, 1, 1, 1, 1, 1, 1,
		      0, 0, 1, 1, 1, 1, 1, 1, 1,
		      0, 0, 1, 1, 1, 1, 1, 1, 1,
		      1, 1, 1, 1, 1, 1, 1, 1, 1
	};
	
	BinaryImage const img(makeBinaryImage(inp, 9, 9));
	BinaryImage const control(makeBinaryImage(out, 9, 9));
	QRect const dst_rect(img.rect().translated(2, 1));
	
	BOOST_CHECK(closeBrick(img, QSize(2, 2), dst_rect, BLACK) == control);
}

BOOST_AUTO_TEST_CASE(test_close_2x2_narrowing)
{
	static int const inp[] = {
		1, 1, 1, 1, 1, 1, 1, 1, 1,
		1, 1, 1, 1, 1, 1, 1, 1, 1,
		1, 0, 0, 0, 1, 1, 1, 1, 1,
		1, 0, 0, 0, 1, 1, 1, 0, 1,
		1, 1, 1, 1, 1, 1, 1, 1, 1,
		0, 0, 0, 0, 1, 1, 1, 1, 1,
		0, 1, 0, 0, 1, 0, 0, 1, 1,
		0, 1, 0, 0, 1, 1, 1, 1, 1,
		0, 0, 0, 0, 1, 1, 1, 1, 0
	};
	
	static int const out[] = {
		//    0, 0, 0, 0,
		//    0, 0, 0, 0
		      0, 0, 1, 1,
		      0, 0, 1, 1,
		      1, 1, 1, 1,
		      0, 0, 1, 1
	};
	
	BinaryImage const img(makeBinaryImage(inp, 9, 9));
	BinaryImage const control(makeBinaryImage(out, 4, 4));
	QRect const dst_rect(img.rect().adjusted(2, 2, -3, -3));
	
	BOOST_CHECK(closeBrick(img, QSize(2, 2), dst_rect, WHITE) == control);
	BOOST_CHECK(closeBrick(img, QSize(2, 2), dst_rect, BLACK) == control);
}

BOOST_AUTO_TEST_CASE(test_hmm_1)
{
	static int const inp[] = {
		0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 1, 1, 1, 0,
		0, 0, 1, 0, 0, 1, 0, 1, 0,
		0, 1, 0, 1, 0, 1, 1, 1, 0,
		0, 0, 1, 0, 1, 0, 0, 0, 0,
		0, 0, 0, 1, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 1, 1, 1, 0,
		0, 0, 0, 0, 0, 1, 1, 1, 0,
		0, 0, 0, 0, 0, 1, 1, 1, 0
	};
	
	static int const out[] = {
		0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 1, 0, 0,
		0, 0, 1, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 1, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0
	};
	
	static char const pattern[] =
		"?X?"
		"X X"
		"?X?";
	QPoint const origin(1, 1);
	
	BinaryImage const img(makeBinaryImage(inp, 9, 9));
	BinaryImage const control(makeBinaryImage(out, 9, 9));
	
	BOOST_CHECK(hitMissMatch(img, WHITE, pattern, 3, 3, origin) == control);
	BOOST_CHECK(hitMissMatch(img, BLACK, pattern, 3, 3, origin) == control);
}

BOOST_AUTO_TEST_CASE(test_hmm_surroundings_1)
{
	static int const inp[] = {
		0, 0, 0, 0, 0, 1, 0, 1, 0,
		0, 0, 0, 0, 0, 1, 1, 1, 0,
		0, 0, 0, 0, 0, 0, 1, 0, 0,
		0, 0, 0, 0, 0, 1, 0, 0, 0,
		0, 0, 0, 0, 1, 0, 0, 0, 0,
		0, 0, 0, 1, 0, 0, 0, 0, 0,
		0, 0, 1, 0, 0, 0, 0, 0, 0,
		0, 1, 0, 0, 0, 0, 0, 0, 0,
		1, 0, 0, 0, 0, 0, 0, 0, 0
	};
	
	static int const out_white[] = {
		0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0
	};
	
	static int const out_black[] = {
		0, 0, 0, 0, 0, 0, 1, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0
	};
	
	static char const pattern[] =
		"?X?"
		"X X"
		"?X?";
	QPoint const origin(1, 1);
	
	BinaryImage const img(makeBinaryImage(inp, 9, 9));
	BinaryImage const control_w(makeBinaryImage(out_white, 9, 9));
	BinaryImage const control_b(makeBinaryImage(out_black, 9, 9));
	
	BOOST_CHECK(hitMissMatch(img, WHITE, pattern, 3, 3, origin) == control_w);
	BOOST_CHECK(hitMissMatch(img, BLACK, pattern, 3, 3, origin) == control_b);
}

BOOST_AUTO_TEST_CASE(test_hmm_surroundings_2)
{
	static int const inp[] = {
		0, 0, 0, 0, 0, 1, 0, 1, 0,
		0, 0, 0, 0, 0, 1, 1, 1, 0,
		0, 0, 0, 0, 0, 0, 1, 0, 0,
		0, 0, 0, 0, 0, 1, 0, 0, 0,
		0, 0, 0, 0, 1, 0, 0, 0, 0,
		0, 0, 0, 1, 0, 0, 0, 0, 0,
		0, 0, 1, 0, 0, 0, 0, 0, 0,
		0, 1, 0, 0, 0, 0, 0, 0, 0,
		1, 0, 0, 0, 0, 0, 0, 0, 0
	};
	
	static int const out[] = {
		0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0
	};
	
	static char const pattern[] =
		"?X?"
		"X X"
		"?X?";
	QPoint const origin(1, 0);
	
	BinaryImage const img(makeBinaryImage(inp, 9, 9));
	BinaryImage const control(makeBinaryImage(out, 9, 9));
	
	BOOST_CHECK(hitMissMatch(img, WHITE, pattern, 3, 3, origin) == control);
	BOOST_CHECK(hitMissMatch(img, BLACK, pattern, 3, 3, origin) == control);
}

BOOST_AUTO_TEST_CASE(test_hmm_cornercase_1)
{
	static int const inp[] = {
		0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 1, 1, 1, 0,
		0, 0, 1, 0, 0, 1, 0, 1, 0,
		0, 1, 0, 1, 0, 1, 1, 1, 0,
		0, 0, 1, 0, 1, 0, 0, 0, 0,
		0, 0, 0, 1, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 1, 1, 1, 0,
		1, 0, 0, 0, 0, 1, 1, 1, 0,
		0, 1, 0, 0, 0, 1, 1, 1, 0
	};
	
	static int const out[] = {
		0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0
	};
	
	static char const pattern[] =
		"?X?"
		"X X"
		"?X?";
	QPoint const origin(10, 0);
	
	BinaryImage const img(makeBinaryImage(inp, 9, 9));
	BinaryImage const control(makeBinaryImage(out, 9, 9));
	
	BOOST_CHECK(hitMissMatch(img, WHITE, pattern, 3, 3, origin) == control);
	BOOST_CHECK(hitMissMatch(img, BLACK, pattern, 3, 3, origin) == control);
}

BOOST_AUTO_TEST_CASE(test_hmm_cornercase_2)
{
	static int const inp[] = {
		0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 1, 1, 1, 0,
		0, 0, 1, 0, 0, 1, 0, 1, 0,
		0, 1, 0, 1, 0, 1, 1, 1, 0,
		0, 0, 1, 0, 1, 0, 0, 0, 0,
		0, 0, 0, 1, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 1, 1, 1, 0,
		1, 0, 0, 0, 0, 1, 1, 1, 0,
		0, 1, 0, 0, 0, 1, 1, 1, 0
	};
	
	static int const out[] = {
		0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0
	};
	
	static char const pattern[] =
		"?X?"
		"X X"
		"?X?";
	QPoint const origin(0, 9);
	
	BinaryImage const img(makeBinaryImage(inp, 9, 9));
	BinaryImage const control(makeBinaryImage(out, 9, 9));
	
	BOOST_CHECK(hitMissMatch(img, WHITE, pattern, 3, 3, origin) == control);
	BOOST_CHECK(hitMissMatch(img, BLACK, pattern, 3, 3, origin) == control);
}

BOOST_AUTO_TEST_CASE(test_hmr_1)
{
	static int const inp[] = {
		0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 1, 0,
		0, 0, 0, 0, 0, 1, 0, 0, 0,
		0, 0, 1, 1, 1, 0, 1, 1, 0,
		0, 0, 1, 0, 1, 1, 1, 1, 0,
		0, 0, 1, 1, 0, 1, 1, 1, 0,
		0, 0, 1, 0, 0, 1, 1, 1, 0,
		0, 0, 0, 0, 0, 1, 1, 1, 0
	};
	
	static int const out[] = {
		0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 1, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 1, 1, 1, 1, 1, 1, 0,
		0, 0, 1, 0, 1, 1, 1, 1, 0,
		0, 0, 1, 1, 0, 1, 1, 1, 0,
		0, 0, 1, 0, 0, 1, 1, 1, 0,
		0, 0, 0, 0, 0, 1, 1, 1, 0
	};
	
	static char const pattern[] =
		" - "
		"X+X"
		"XXX";
	
	BinaryImage const img(makeBinaryImage(inp, 9, 9));
	BinaryImage const control(makeBinaryImage(out, 9, 9));
	
	BOOST_CHECK(hitMissReplace(img, WHITE, pattern, 3, 3) == control);
	BOOST_CHECK(hitMissReplace(img, BLACK, pattern, 3, 3) == control);
}

BOOST_AUTO_TEST_SUITE_END();

} // namespace tests

} // namespace imageproc
