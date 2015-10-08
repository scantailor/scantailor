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

#include "Scale.h"
#include "GrayImage.h"
#include "Utils.h"
#include <QImage>
#include <QSize>
#ifndef Q_MOC_RUN
#include <boost/test/auto_unit_test.hpp>
#endif
#include <stdint.h>
#include <stdlib.h>
#include <math.h>

namespace imageproc
{

namespace tests
{

using namespace utils;

BOOST_AUTO_TEST_SUITE(ScaleTestSuite);

BOOST_AUTO_TEST_CASE(test_null_image)
{
	GrayImage const null_img;
	BOOST_CHECK(scaleToGray(null_img, QSize(1, 1)).isNull());
}

static bool fuzzyCompare(QImage const& img1, QImage const& img2)
{
	BOOST_REQUIRE(img1.size() == img2.size());
	
	int const width = img1.width();
	int const height = img1.height();
	uint8_t const* line1 = img1.bits();
	uint8_t const* line2 = img2.bits();
	int const line1_bpl = img1.bytesPerLine();
	int const line2_bpl = img2.bytesPerLine();
	
	for (int y = 0; y < height; ++y) {
		for (int x = 0; x < width; ++x) {
			if (abs(int(line1[x]) - int(line2[x])) > 1) {
				return false;
			}
		}
		line1 += line1_bpl;
		line2 += line2_bpl;
	}
	
	return true;
}

static bool checkScale(GrayImage const& img, QSize const& new_size)
{
	GrayImage const scaled1(scaleToGray(img, new_size));
	GrayImage const scaled2(img.toQImage().scaled(
		new_size, Qt::IgnoreAspectRatio, Qt::SmoothTransformation
	));
	
	return fuzzyCompare(scaled1, scaled2);
}

BOOST_AUTO_TEST_CASE(test_random_image)
{
	GrayImage img(QSize(100, 100));
	uint8_t* line = img.data();
	for (int y = 0; y < img.height(); ++y) {
		for (int x = 0; x < img.width(); ++x) {
			line[x] = rand() % 256;
		}
		line += img.stride();
	}
	
	// Unfortunately scaleToGray() and QImage::scaled()
	// produce too different results when upscaling.
	
	BOOST_CHECK(checkScale(img, QSize(50, 50)));
	//BOOST_CHECK(checkScale(img, QSize(200, 200)));
	BOOST_CHECK(checkScale(img, QSize(80, 80)));
	//BOOST_CHECK(checkScale(img, QSize(140, 140)));
	//BOOST_CHECK(checkScale(img, QSize(55, 145)));
	//BOOST_CHECK(checkScale(img, QSize(145, 55)));
}

BOOST_AUTO_TEST_SUITE_END();

} // namespace tests

} // namespace imageproc
