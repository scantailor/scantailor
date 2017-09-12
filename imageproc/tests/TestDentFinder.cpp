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

#include "DentFinder.h"
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

BOOST_AUTO_TEST_SUITE(DentFinderTestSuite);

BOOST_AUTO_TEST_CASE(test_null_image)
{
	BinaryImage const null_img;
	BOOST_CHECK(DentFinder::findDentsAndHoles(null_img).isNull());
}

BOOST_AUTO_TEST_CASE(test1)
{
	static int const inp[] = {
		0, 1, 0, 0, 0, 0, 0, 1, 0,
		0, 1, 1, 1, 1, 1, 1, 1, 0,
		0, 0, 0, 0, 1, 0, 0, 0, 0,
		0, 0, 0, 0, 1, 0, 0, 0, 0,
		0, 0, 0, 0, 1, 0, 0, 0, 0,
		0, 0, 0, 0, 1, 0, 0, 0, 0,
		0, 0, 0, 0, 1, 0, 0, 0, 0,
		0, 0, 0, 0, 1, 0, 0, 0, 0,
		0, 0, 0, 1, 1, 1, 0, 0, 0
	};
	
	static int const out[] = {
		0, 0, 1, 1, 1, 1, 1, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 1, 1, 0, 1, 1, 0, 0,
		0, 0, 0, 1, 0, 1, 0, 0, 0,
		0, 0, 0, 1, 0, 1, 0, 0, 0,
		0, 0, 0, 1, 0, 1, 0, 0, 0,
		0, 0, 0, 1, 0, 1, 0, 0, 0,
		0, 0, 0, 1, 0, 1, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0
	};
	
	BinaryImage const img(makeBinaryImage(inp, 9, 9));
	BinaryImage const control(makeBinaryImage(out, 9, 9));
	
	BOOST_CHECK(DentFinder::findDentsAndHoles(img) == control);
}

BOOST_AUTO_TEST_SUITE_END();

} // namespace tests

} // namespace imageproc
