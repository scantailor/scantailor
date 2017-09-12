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

#include "ConnCompEraserExt.h"
#include "ConnComp.h"
#include "BinaryImage.h"
#include "BWColor.h"
#include "RasterOp.h"
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

BOOST_AUTO_TEST_SUITE(ConnCompEraserExtTestSuite);

BOOST_AUTO_TEST_CASE(test_null_image)
{
	ConnCompEraser eraser(BinaryImage(), CONN4);
	BOOST_CHECK(eraser.nextConnComp().isNull());
}

static bool checkAlignedImage(
	ConnCompEraserExt const& eraser, BinaryImage const& nonaligned)
{
	BinaryImage const aligned(eraser.computeConnCompImageAligned());
	int const pad = aligned.width() - nonaligned.width();
	if (pad < 0) {
		return false;
	}
	
	BinaryImage test1(nonaligned);
	BinaryImage empty1(test1.size());
	empty1.fill(WHITE);
	rasterOp<RopXor<RopSrc, RopDst> >(test1, test1.rect(), aligned, QPoint(pad, 0));
	if (test1 != empty1) {
		return false;
	}
	
	if (pad > 0) {
		// Check that padding is white.
		BinaryImage test2(pad, nonaligned.height());
		BinaryImage empty2(test2.size());
		empty2.fill(WHITE);
		rasterOp<RopSrc>(test2, test2.rect(), aligned, QPoint(0, 0));
		if (test2 != empty2) {
			return false;
		}
	}
	
	return true;
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
	
	std::list<BinaryImage> c4i;
	
	static int const out4_1[] = {
		1, 1, 0,
		0, 1, 0,
		0, 1, 0,
		0, 1, 1,
		1, 1, 0,
		0, 1, 0
	};
	c4i.push_back(makeBinaryImage(out4_1, 3, 6));
	
	static int const out4_2[] = {
		1, 1
	};
	c4i.push_back(makeBinaryImage(out4_2, 2, 1));
	
	static int const out4_3[] = {
		0, 0, 0, 0, 0, 1, 0,
		1, 1, 1, 1, 1, 1, 1
	};
	c4i.push_back(makeBinaryImage(out4_3, 7, 2));
	
	static int const out4_4[] = {
		1, 1, 1, 1,
		0, 1, 0, 0,
		0, 1, 1, 0,
	};
	c4i.push_back(makeBinaryImage(out4_4, 4, 3));
	
	static int const out4_5[] = {
		1
	};
	c4i.push_back(makeBinaryImage(out4_5, 1, 1));
	
	static int const out4_6[] = {
		1
	};
	c4i.push_back(makeBinaryImage(out4_6, 1, 1));
	
	std::list<BinaryImage> c8i;
	
	static int const out8_1[] = {
		0, 0, 1, 1, 0, 0, 0, 0, 0,
		0, 0, 0, 1, 0, 0, 0, 0, 0,
		0, 0, 0, 1, 0, 1, 1, 1, 1,
		1, 1, 0, 1, 1, 0, 1, 0, 0,
		0, 0, 1, 1, 0, 0, 1, 1, 0,
		0, 1, 0, 1, 0, 0, 0, 0, 0,
	};
	c8i.push_back(makeBinaryImage(out8_1, 9, 6));
	
	static int const out8_2[] = {
		0, 0, 0, 0, 0, 1, 0, 1,
		1, 1, 1, 1, 1, 1, 1, 0,
	};
	c8i.push_back(makeBinaryImage(out8_2, 8, 2));
	
	BinaryImage img(makeBinaryImage(inp, 9, 8));
	
	ConnComp cc;
	ConnCompEraserExt eraser4(img, CONN4);
	while (!(cc = eraser4.nextConnComp()).isNull()) {
		BinaryImage const cc_img(eraser4.computeConnCompImage());
		std::list<BinaryImage>::iterator const it(
			std::find(c4i.begin(), c4i.end(), cc_img)
		);
		if (it != c4i.end()) {
			BOOST_CHECK(checkAlignedImage(eraser4, cc_img));
			c4i.erase(it);
		} else {
			BOOST_ERROR("Incorrect 4-connected block found.");
		}
	}
	BOOST_CHECK_MESSAGE(c4i.empty(), "Not all 4-connected blocks were found.");
	
	ConnCompEraserExt eraser8(img, CONN8);
	while (!(cc = eraser8.nextConnComp()).isNull()) {
		BinaryImage const cc_img(eraser8.computeConnCompImage());
		std::list<BinaryImage>::iterator const it(
			std::find(c8i.begin(), c8i.end(), cc_img)
		);
		if (it != c8i.end()) {
			BOOST_CHECK(checkAlignedImage(eraser8, cc_img));
			c8i.erase(it);
		} else {
			BOOST_ERROR("Incorrect 8-connected block found.");
		}
	}
	BOOST_CHECK_MESSAGE(c8i.empty(), "Not all 8-connected blocks were found.");
}

BOOST_AUTO_TEST_SUITE_END();

} // namespace tests

} // namespace imageproc
