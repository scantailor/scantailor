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

#include "BinaryImage.h"
#include "BWColor.h"
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

BOOST_AUTO_TEST_SUITE(BinaryImageTestSuite);

BOOST_AUTO_TEST_CASE(test_null_image)
{
	BOOST_CHECK(BinaryImage().toQImage() == QImage());
	BOOST_CHECK(BinaryImage(QImage()).toQImage() == QImage());
}

BOOST_AUTO_TEST_CASE(test_from_to_qimage)
{
	int const w = 50;
	int const h = 64;
	QImage qimg_argb32(w, h, QImage::Format_ARGB32);
	QImage qimg_mono(w, h, QImage::Format_Mono);
	qimg_mono.setNumColors(2);
	qimg_mono.setColor(0, 0xffffffff);
	qimg_mono.setColor(1, 0xff000000);
	for (int y = 0; y < h; ++y) {
		for (int x = 0; x < w; ++x) {
			int const rnd = rand() & 1;
			qimg_argb32.setPixel(x, y, rnd ? 0x66888888 : 0x66777777);
			qimg_mono.setPixel(x, y, rnd ? 0 : 1);
		}
	}
	
	QImage qimg_mono_lsb(qimg_mono.convertToFormat(QImage::Format_MonoLSB));
	QImage qimg_rgb32(qimg_argb32.convertToFormat(QImage::Format_RGB32));
	QImage qimg_argb32_pm(qimg_argb32.convertToFormat(QImage::Format_ARGB32_Premultiplied));
	QImage qimg_rgb16(qimg_rgb32.convertToFormat(QImage::Format_RGB16));
	QImage qimg_indexed8(qimg_rgb32.convertToFormat(QImage::Format_Indexed8));
	
	BOOST_REQUIRE(BinaryImage(qimg_mono).toQImage() == qimg_mono);
	BOOST_CHECK(BinaryImage(qimg_mono_lsb).toQImage() == qimg_mono);
	BOOST_CHECK(BinaryImage(qimg_argb32).toQImage() == qimg_mono);
	BOOST_CHECK(BinaryImage(qimg_rgb32).toQImage() == qimg_mono);
	BOOST_CHECK(BinaryImage(qimg_argb32_pm).toQImage() == qimg_mono);
	BOOST_CHECK(BinaryImage(qimg_indexed8).toQImage() == qimg_mono);
	
	// A bug in Qt prevents this from working.
	//BOOST_CHECK(BinaryImage(qimg_rgb16, 0x80).toQImage() == qimg_mono);
}

BOOST_AUTO_TEST_CASE(test_full_fill)
{
	BinaryImage white(100, 100);
	white.fill(WHITE);
	
	QImage q_white(100, 100, QImage::Format_Mono);
	q_white.fill(1);
	
	BOOST_REQUIRE(BinaryImage(q_white) == white);
	
	BinaryImage black(30, 30);
	black.fill(BLACK);
	
	QImage q_black(30, 30, QImage::Format_Mono);
	q_black.fill(0);
	
	BOOST_REQUIRE(BinaryImage(q_black) == black);
}

BOOST_AUTO_TEST_CASE(test_partial_fill_small)
{
	QImage q_image(randomMonoQImage(100, 100));
	
	QRect const rect(80, 80, 20, 20);
	BinaryImage image(q_image);
	image.fill(rect, WHITE);
	QImage white_rect(rect.width(), rect.height(), QImage::Format_Mono);
	white_rect.setNumColors(2);
	white_rect.setColor(0, 0xffffffff);
	white_rect.setColor(1, 0xff000000);
	white_rect.fill(0);
	BOOST_REQUIRE(image.toQImage().copy(rect) == white_rect);
	BOOST_CHECK(surroundingsIntact(image.toQImage(), q_image, rect));
}

BOOST_AUTO_TEST_CASE(test_partial_fill_large)
{
	QImage q_image(randomMonoQImage(100, 100));
	
	QRect const rect(20, 20, 79, 79);
	BinaryImage image(q_image);
	image.fill(rect, WHITE);
	QImage white_rect(rect.width(), rect.height(), QImage::Format_Mono);
	white_rect.setNumColors(2);
	white_rect.setColor(0, 0xffffffff);
	white_rect.setColor(1, 0xff000000);
	white_rect.fill(0);
	BOOST_REQUIRE(image.toQImage().copy(rect) == white_rect);
	BOOST_CHECK(surroundingsIntact(image.toQImage(), q_image, rect));
}

BOOST_AUTO_TEST_CASE(test_fill_except)
{
	QImage q_image(randomMonoQImage(100, 100));
	
	QRect const rect(20, 20, 79, 79);
	BinaryImage image(q_image);
	image.fillExcept(rect, BLACK);
	
	QImage black_image(q_image.width(), q_image.height(), QImage::Format_Mono);
	black_image.setNumColors(2);
	black_image.setColor(0, 0xffffffff);
	black_image.setColor(1, 0xff000000);
	black_image.fill(1);
	
	BOOST_REQUIRE(image.toQImage().copy(rect) == q_image.copy(rect));
	BOOST_CHECK(surroundingsIntact(image.toQImage(), black_image, rect));
}

BOOST_AUTO_TEST_CASE(test_content_bounding_box4)
{
	static int const inp[] = {
		0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 1, 0, 0,
		0, 0, 0, 0, 0, 1, 0, 0, 0,
		0, 0, 0, 0, 1, 0, 0, 0, 0,
		0, 0, 0, 1, 0, 0, 0, 0, 0,
		0, 0, 1, 0, 0, 0, 0, 0, 0,
		0, 1, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0
	};
	
	BinaryImage const img(makeBinaryImage(inp, 9, 8));
	BOOST_CHECK(img.contentBoundingBox() == QRect(1, 1, 6, 6));
}

BOOST_AUTO_TEST_SUITE_END();

} // namespace tests

} // namespace imageproc
