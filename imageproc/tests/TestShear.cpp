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

#include "Shear.h"
#include "BinaryImage.h"
#include "BWColor.h"
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

BOOST_AUTO_TEST_SUITE(ShearTestSuite);

BOOST_AUTO_TEST_CASE(test_small_image)
{
	static int const inp[] = {
		0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 1, 1, 1, 1, 1, 1, 1, 0,
		0, 1, 1, 1, 1, 1, 1, 1, 0,
		0, 1, 1, 1, 1, 1, 1, 1, 0,
		0, 1, 1, 1, 1, 1, 1, 1, 0,
		0, 1, 1, 1, 1, 1, 1, 1, 0,
		0, 1, 1, 1, 1, 1, 1, 1, 0,
		0, 1, 1, 1, 1, 1, 1, 1, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0
	};
	
	static int const h_out[] = {
		0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 1, 1, 1, 1, 1,
		0, 0, 0, 1, 1, 1, 1, 1, 1,
		0, 0, 1, 1, 1, 1, 1, 1, 1,
		0, 1, 1, 1, 1, 1, 1, 1, 0,
		1, 1, 1, 1, 1, 1, 1, 0, 0,
		1, 1, 1, 1, 1, 1, 0, 0, 0,
		1, 1, 1, 1, 1, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0
	};
	
	static int const v_out[] = {
		0, 1, 1, 1, 0, 0, 0, 0, 0,
		0, 1, 1, 1, 1, 0, 0, 0, 0,
		0, 1, 1, 1, 1, 1, 0, 0, 0,
		0, 1, 1, 1, 1, 1, 1, 0, 0,
		0, 1, 1, 1, 1, 1, 1, 1, 0,
		0, 0, 1, 1, 1, 1, 1, 1, 0,
		0, 0, 0, 1, 1, 1, 1, 1, 0,
		0, 0, 0, 0, 1, 1, 1, 1, 0,
		0, 0, 0, 0, 0, 1, 1, 1, 0
	};
	
	BinaryImage const img(makeBinaryImage(inp, 9, 9));
	BinaryImage const h_out_img(makeBinaryImage(h_out, 9, 9));
	BinaryImage const v_out_img(makeBinaryImage(v_out, 9, 9));
	
	BinaryImage const h_shear = hShear(img, -1.0, 0.5 * img.height(), WHITE);
	BOOST_REQUIRE(h_shear == h_out_img);
	
	BinaryImage const v_shear = vShear(img, 1.0, 0.5 * img.width(), WHITE);
	BOOST_REQUIRE(v_shear == v_out_img);
	
	BinaryImage h_shear_inplace(img);
	hShearInPlace(h_shear_inplace, -1.0, 0.5 * img.height(), WHITE);
	BOOST_REQUIRE(h_shear_inplace == h_out_img);
	
	BinaryImage v_shear_inplace(img);
	vShearInPlace(v_shear_inplace, 1.0, 0.5 * img.width(), WHITE);
	BOOST_REQUIRE(v_shear_inplace == v_out_img);
}

BOOST_AUTO_TEST_SUITE_END();

} // namespace tests

} // namespace imageproc
