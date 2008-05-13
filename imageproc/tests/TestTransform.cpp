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

#include "Transform.h"
#include "Grayscale.h"
#include "Utils.h"
#include <QImage>
#include <QSize>
#include <boost/test/auto_unit_test.hpp>
#include <stdint.h>
#include <stdlib.h>
#include <math.h>

namespace imageproc
{

namespace tests
{

using namespace utils;

BOOST_AUTO_TEST_SUITE(TransformTestSuite);

BOOST_AUTO_TEST_CASE(test_null_image)
{
	QImage const null_img;
	QTransform const null_xform;
	QRect const unit_rect(0, 0, 1, 1);
	QColor const bgcolor(0xff, 0xff, 0xff);
	BOOST_CHECK(transformToGray(null_img, null_xform, unit_rect, bgcolor).isNull());
}

BOOST_AUTO_TEST_CASE(test_random_image)
{
	QImage img(100, 100, QImage::Format_Indexed8);
	img.setColorTable(createGrayscalePalette());
	uint8_t* line = img.bits();
	for (int y = 0; y < img.height(); ++y) {
		for (int x = 0; x < img.width(); ++x) {
			line[x] = rand() % 256;
		}
		line += img.bytesPerLine();
	}
	
	QColor const bgcolor(0xff, 0xff, 0xff);
	
	QTransform const null_xform;
	BOOST_CHECK(transformToGray(img, null_xform, img.rect(), bgcolor) == img);
}

BOOST_AUTO_TEST_SUITE_END();

} // namespace tests

} // namespace imageproc
