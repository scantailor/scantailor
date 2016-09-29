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

#include "SlicedHistogram.h"
#include "BinaryImage.h"
#include "Utils.h"
#include <QImage>
#include <stdexcept>
#include <stddef.h>
#ifndef Q_MOC_RUN
#include <boost/test/auto_unit_test.hpp>
#endif

namespace imageproc
{

namespace tests
{

using namespace utils;

static bool checkHistogram(
	SlicedHistogram const& hist, int const* data_begin, int const* data_end)
{
	if (hist.size() != size_t(data_end - data_begin)) {
		return false;
	}
	for (unsigned i = 0; i < hist.size(); ++i) {
		if (hist[i] != data_begin[i]) {
			return false;
		}
	}
	
	return true;
}

BOOST_AUTO_TEST_SUITE(SlicedHistogramTestSuite);

BOOST_AUTO_TEST_CASE(test_null_image)
{
	BinaryImage const null_img;
	
	SlicedHistogram hor_hist(null_img, SlicedHistogram::ROWS);
	BOOST_CHECK(hor_hist.size() == 0);
	
	SlicedHistogram ver_hist(null_img, SlicedHistogram::COLS);
	BOOST_CHECK(ver_hist.size() == 0);
}

BOOST_AUTO_TEST_CASE(test_exceeding_area)
{
	BinaryImage const img(1, 1);
	QRect const area(0, 0, 1, 2);
	
	BOOST_CHECK_THROW(
		SlicedHistogram(img, area, SlicedHistogram::ROWS),
		std::invalid_argument
	);
}

BOOST_AUTO_TEST_CASE(test_small_image)
{
	static int const inp[] = {
		0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 1, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 1, 0, 0, 0, 1, 0,
		1, 0, 0, 1, 1, 0, 0, 0, 0,
		1, 1, 1, 1, 1, 1, 1, 1, 1,
		0, 0, 0, 0, 1, 0, 0, 1, 0,
		0, 1, 1, 1, 1, 1, 1, 0, 0,
		0, 0, 1, 0, 1, 1, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 1
	};
	
	static int const hor_counts[] = {
		0, 1, 2, 3, 9, 2, 6, 3, 1
	};
	
	static int const ver_counts[] = {
		2, 2, 4, 4, 5, 3, 2, 3, 2
	};
	
	BinaryImage const img(makeBinaryImage(inp, 9, 9));
	
	SlicedHistogram hor_hist(img, SlicedHistogram::ROWS);
	BOOST_CHECK(checkHistogram(hor_hist, hor_counts, hor_counts + 9));
	
	SlicedHistogram ver_hist(img, SlicedHistogram::COLS);
	BOOST_CHECK(checkHistogram(ver_hist, ver_counts, ver_counts + 9));
	
	hor_hist = SlicedHistogram(
		img, img.rect().adjusted(0, 1, 0, 0), SlicedHistogram::ROWS
	);
	BOOST_CHECK(checkHistogram(hor_hist, hor_counts + 1, hor_counts + 9));
	
	ver_hist = SlicedHistogram(
		img, img.rect().adjusted(1, 0, 0, 0), SlicedHistogram::COLS
	);
	BOOST_CHECK(checkHistogram(ver_hist, ver_counts + 1, ver_counts + 9));
}

BOOST_AUTO_TEST_SUITE_END();

} // namespace tests

} // namespace imageproc
