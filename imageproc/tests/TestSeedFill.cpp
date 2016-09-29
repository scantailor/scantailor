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

#include "SeedFill.h"
#include "Connectivity.h"
#include "BinaryImage.h"
#include "BWColor.h"
#include "Grayscale.h"
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

BOOST_AUTO_TEST_SUITE(SeedFillTestSuite);

BOOST_AUTO_TEST_CASE(test_regression_1)
{
	int seed_data[70*2] = { 0 };
	int mask_data[70*2] = { 0 };
	
	seed_data[32] = 1;
	seed_data[64] = 1;
	
	mask_data[32] = 1;
	mask_data[64] = 1;
	mask_data[70 + 31] = 1;
	mask_data[70 + 63] = 1;
	
	BinaryImage const seed(makeBinaryImage(seed_data, 70, 2));
	BinaryImage const mask(makeBinaryImage(mask_data, 70, 2));
	BOOST_CHECK(seedFill(seed, mask, CONN8) == mask);
}

BOOST_AUTO_TEST_CASE(test_regression_2)
{
	int seed_data[70*2] = { 0 };
	int mask_data[70*2] = { 0 };
	
	seed_data[32] = 1;
	seed_data[64] = 1;
	
	mask_data[31] = 1;
	mask_data[63] = 1;
	mask_data[70 + 31] = 1;
	mask_data[70 + 63] = 1;
	
	BinaryImage const seed(makeBinaryImage(seed_data, 70, 2));
	BinaryImage const mask(makeBinaryImage(mask_data, 70, 2));
	BOOST_CHECK(seedFill(seed, mask, CONN8) == BinaryImage(70, 2, WHITE));
}

BOOST_AUTO_TEST_CASE(test_regression_3)
{
	static int const seed_data[] = {
		1, 1, 0, 1, 0,
		0, 1, 0, 0, 0,
		0, 1, 1, 0, 1,
		1, 0, 0, 0, 0,
		1, 1, 1, 0, 0
	};
	
	static int const mask_data[] = {
		0, 1, 0, 0, 1,
		0, 1, 0, 0, 0,
		1, 0, 0, 1, 1,
		1, 0, 0, 0, 0,
		0, 1, 0, 0, 1
	};
	
	static int const fill_data[] = {
		0, 1, 0, 0, 0,
		0, 1, 0, 0, 0,
		1, 0, 0, 1, 1,
		1, 0, 0, 0, 0,
		0, 1, 0, 0, 0
	};
	
	BinaryImage const seed(makeBinaryImage(seed_data, 5, 5));
	BinaryImage const mask(makeBinaryImage(mask_data, 5, 5));
	BinaryImage const fill(makeBinaryImage(fill_data, 5, 5));
	
	BOOST_REQUIRE(seedFill(seed, mask, CONN8) == fill);
}

BOOST_AUTO_TEST_CASE(test_regression_4)
{
	static int const seed_data[] = {
		0, 1, 1, 0, 0,
		1, 0, 0, 1, 0,
		1, 1, 0, 0, 0,
		1, 0, 1, 1, 0,
		0, 0, 1, 1, 1
	};
	
	static int const mask_data[] = {
		1, 1, 1, 0, 1,
		1, 1, 0, 1, 0,
		0, 1, 0, 1, 1,
		1, 1, 1, 1, 0,
		1, 0, 1, 0, 1
	};
	
	static int const fill_data[] = {
		1, 1, 1, 0, 0,
		1, 1, 0, 1, 0,
		0, 1, 0, 1, 1,
		1, 1, 1, 1, 0,
		1, 0, 1, 0, 1
	};
	
	BinaryImage const seed(makeBinaryImage(seed_data, 5, 5));
	BinaryImage const mask(makeBinaryImage(mask_data, 5, 5));
	BinaryImage const fill(makeBinaryImage(fill_data, 5, 5));
	BOOST_REQUIRE(seedFill(seed, mask, CONN4) == fill);
}

BOOST_AUTO_TEST_CASE(test_gray4_random)
{
	for (int i = 0; i < 200; ++i) {
		GrayImage const seed(randomGrayImage(5, 5));
		GrayImage const mask(randomGrayImage(5, 5));
		GrayImage const fill_new(seedFillGray(seed, mask, CONN4));
		GrayImage const fill_old(seedFillGraySlow(seed, mask, CONN4));
		if (fill_new != fill_old) {
			BOOST_ERROR("fill_new != fill_old at iteration " << i);
			dumpGrayImage(seed, "seed");
			dumpGrayImage(mask, "mask");
			dumpGrayImage(fill_old, "fill_old");
			dumpGrayImage(fill_new, "fill_new");
			break;
		}
	}
}

BOOST_AUTO_TEST_CASE(test_gray8_random)
{
	for (int i = 0; i < 200; ++i) {
		GrayImage const seed(randomGrayImage(5, 5));
		GrayImage const mask(randomGrayImage(5, 5));
		GrayImage const fill_new(seedFillGray(seed, mask, CONN8));
		GrayImage const fill_old(seedFillGraySlow(seed, mask, CONN8));
		if (fill_new != fill_old) {
			BOOST_ERROR("fill_new != fill_old at iteration " << i);
			dumpGrayImage(seed, "seed");
			dumpGrayImage(mask, "mask");
			dumpGrayImage(fill_old, "fill_old");
			dumpGrayImage(fill_new, "fill_new");
			break;
		}
	}
}

BOOST_AUTO_TEST_CASE(test_gray_vs_binary)
{
	for (int i = 0; i < 200; ++i) {
		BinaryImage const bin_seed(randomBinaryImage(5, 5));
		BinaryImage const bin_mask(randomBinaryImage(5, 5));
		GrayImage const gray_seed(toGrayscale(bin_seed.toQImage()));
		GrayImage const gray_mask(toGrayscale(bin_mask.toQImage()));
		BinaryImage const fill_bin4(seedFill(bin_seed, bin_mask, CONN4));
		BinaryImage const fill_bin8(seedFill(bin_seed, bin_mask, CONN8));
		GrayImage const fill_gray4(seedFillGray(gray_seed, gray_mask, CONN4));
		GrayImage const fill_gray8(seedFillGray(gray_seed, gray_mask, CONN8));
		
		if (fill_gray4 != GrayImage(fill_bin4.toQImage())) {
			BOOST_ERROR("grayscale 4-fill != binary 4-fill at index " << i);
			dumpBinaryImage(bin_seed, "seed");
			dumpBinaryImage(bin_mask, "mask");
			dumpBinaryImage(fill_bin4, "bin_fill");
			dumpBinaryImage(BinaryImage(fill_gray4), "gray_fill");
			break;
		}
		
		if (fill_gray8 != GrayImage(fill_bin8.toQImage())) {
			BOOST_ERROR("grayscale 8-fill != binary 8-fill at index " << i);
			dumpBinaryImage(bin_seed, "seed");
			dumpBinaryImage(bin_mask, "mask");
			dumpBinaryImage(fill_bin8, "bin_fill");
			dumpBinaryImage(BinaryImage(fill_gray8), "gray_fill");
			break;
		}
	}
}

BOOST_AUTO_TEST_SUITE_END();

} // namespace tests

} // namespace imageproc
