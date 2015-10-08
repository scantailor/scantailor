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

#include "Grayscale.h"
#include "Utils.h"
#include <QImage>
#ifndef Q_MOC_RUN
#include <boost/test/auto_unit_test.hpp>
#endif
#include <stdlib.h>

namespace imageproc
{

namespace tests
{

using namespace utils;

BOOST_AUTO_TEST_SUITE(GrayscaleTestSuite);

BOOST_AUTO_TEST_CASE(test_null_image)
{
	BOOST_CHECK(toGrayscale(QImage()).isNull());
}

BOOST_AUTO_TEST_CASE(test_mono_to_grayscale)
{
	int const w = 50;
	int const h = 64;
	
	QImage mono(w, h, QImage::Format_Mono);
	QImage gray(w, h, QImage::Format_Indexed8);
	gray.setColorTable(createGrayscalePalette());
	
	for (int y = 0; y < h; ++y) {
		for (int x = 0; x < w; ++x) {
			int const rnd = rand() & 1;
			mono.setPixel(x, y, rnd ? 0 : 1);
			gray.setPixel(x, y, rnd ? 0 : 255);
		}
	}
	
	QImage const mono_lsb(mono.convertToFormat(QImage::Format_MonoLSB));
	
	BOOST_REQUIRE(toGrayscale(mono) == gray);
	BOOST_CHECK(toGrayscale(mono_lsb) == gray);
}

BOOST_AUTO_TEST_CASE(test_argb32_to_grayscale)
{
	int const w = 50;
	int const h = 64;
	QImage argb32(w, h, QImage::Format_ARGB32);
	QImage gray(w, h, QImage::Format_Indexed8);
	gray.setColorTable(createGrayscalePalette());
	
	for (int y = 0; y < h; ++y) {
		for (int x = 0; x < w; ++x) {
			int const rnd = rand() & 1;
			argb32.setPixel(x, y, rnd ? 0x80303030 : 0x80606060);
			gray.setPixel(x, y, rnd ? 0x30 : 0x60);
		}
	}
	
	BOOST_CHECK(toGrayscale(argb32) == gray);
}

BOOST_AUTO_TEST_SUITE_END();

} // namespace tests

} // namespace imageproc
