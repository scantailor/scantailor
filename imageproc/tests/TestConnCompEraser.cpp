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

#include "ConnCompEraser.h"
#include "ConnComp.h"
#include "BinaryImage.h"
#include "Utils.h"
#include <QImage>
#include <list>
#include <algorithm>
#ifndef Q_MOC_RUN
#include <boost/test/auto_unit_test.hpp>
#endif

namespace imageproc
{

namespace tests
{

using namespace utils;

BOOST_AUTO_TEST_SUITE(ConnCompEraserTestSuite);

BOOST_AUTO_TEST_CASE(test_null_image)
{
	ConnCompEraser eraser(BinaryImage(), CONN4);
	BOOST_CHECK(eraser.nextConnComp().isNull());
}

BOOST_AUTO_TEST_CASE(test_small_image)
{
	static int const inp[] = {
		0, 0, 1, 1, 0, 0, 0, 0, 0,
		0, 0, 0, 1, 0, 0, 0, 0, 0,
		0, 0, 0, 1, 0, 1, 1, 1, 1,
		1, 1, 0, 1, 1, 0, 1, 0, 0,
		0, 0, 1, 1, 0, 0, 1, 1, 0,
		0, 1, 0, 1, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 1, 0, 1, 0,
		1, 1, 1, 1, 1, 1, 1, 0, 0
	};
	
	std::list<QRect> c4r;
	c4r.push_back(QRect(2, 0, 3, 6));
	c4r.push_back(QRect(0, 3, 2, 1));
	c4r.push_back(QRect(1, 5, 1, 1));
	c4r.push_back(QRect(5, 2, 4, 3));
	c4r.push_back(QRect(0, 6, 7, 2));
	c4r.push_back(QRect(7, 6, 1, 1));
	
	std::list<QRect> c8r;
	c8r.push_back(QRect(0, 0, 9, 6));
	c8r.push_back(QRect(0, 6, 8, 2));
	
	BinaryImage img(makeBinaryImage(inp, 9, 8));
	
	ConnComp cc;
	ConnCompEraser eraser4(img, CONN4);
	while (!(cc = eraser4.nextConnComp()).isNull()) {
		std::list<QRect>::iterator const it(
			std::find(c4r.begin(), c4r.end(), cc.rect())
		);
		if (it != c4r.end()) {
			c4r.erase(it);
		} else {
			BOOST_ERROR("Incorrect 4-connected block found.");
		}
	}
	BOOST_CHECK_MESSAGE(c4r.empty(), "Not all 4-connected blocks were found.");
	
	ConnCompEraser eraser8(img, CONN8);
	while (!(cc = eraser8.nextConnComp()).isNull()) {
		std::list<QRect>::iterator const it(
			std::find(c8r.begin(), c8r.end(), cc.rect())
		);
		if (it != c8r.end()) {
			c8r.erase(it);
		} else {
			BOOST_ERROR("Incorrect 8-connected block found.");
		}
	}
	BOOST_CHECK_MESSAGE(c8r.empty(), "Not all 8-connected blocks were found.");
}

BOOST_AUTO_TEST_SUITE_END();

} // namespace tests

} // namespace imageproc
