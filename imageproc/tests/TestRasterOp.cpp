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

#include "RasterOp.h"
#include "BinaryImage.h"
#include "Utils.h"
#include <QImage>
#ifndef Q_MOC_RUN
#include <boost/test/auto_unit_test.hpp>
#endif
#include <vector>
#include <stddef.h>
#include <stdlib.h>
#include <assert.h>

namespace imageproc
{

namespace tests
{

using namespace utils;

BOOST_AUTO_TEST_SUITE(RasterOpTestSuite);

template<typename Rop>
static bool check_subimage_rop(
	QImage const& dst, QRect const& dst_rect,
	QImage const& src, QPoint const& src_pt)
{
	BinaryImage dst_bi(dst);
	BinaryImage const src_bi(src);
	rasterOp<Rop>(dst_bi, dst_rect, src_bi, src_pt);
	
	// Here we assume that full-image raster operations work correctly.
	BinaryImage dst_subimage(dst.copy(dst_rect));
	QRect const src_rect(src_pt, dst_rect.size());
	BinaryImage const src_subimage(src.copy(src_rect));
	rasterOp<Rop>(dst_subimage, dst_subimage.rect(), src_subimage, QPoint(0, 0));
	
	return dst_subimage.toQImage() == dst_bi.toQImage().copy(dst_rect);
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
	
	static int const mask[] = {
		0, 0, 0, 1, 1, 1, 0, 0, 0,
		0, 0, 0, 1, 1, 1, 0, 0, 0,
		0, 0, 0, 1, 1, 1, 0, 0, 0,
		0, 0, 0, 1, 1, 1, 0, 0, 0,
		0, 0, 0, 1, 1, 1, 0, 0, 0,
		0, 0, 0, 1, 1, 1, 0, 0, 0,
		0, 0, 0, 1, 1, 1, 0, 0, 0,
		0, 0, 0, 1, 1, 1, 0, 0, 0
	};
	
	static int const out[] = {
		0, 0, 0, 1, 0, 0, 0, 0, 0,
		0, 0, 0, 1, 0, 0, 0, 0, 0,
		0, 0, 0, 1, 0, 1, 0, 0, 0,
		0, 0, 0, 1, 1, 0, 0, 0, 0,
		0, 0, 0, 1, 0, 0, 0, 0, 0,
		0, 0, 0, 1, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 1, 0, 0, 0,
		0, 0, 0, 1, 1, 1, 0, 0, 0
	};
	
	BinaryImage img(makeBinaryImage(inp, 9, 8));
	BinaryImage const mask_img(makeBinaryImage(mask, 9, 8));
	
	typedef RopAnd<RopDst, RopSrc> Rop;
	
	rasterOp<Rop>(img, img.rect(), mask_img, QPoint(0, 0));
	BOOST_CHECK(img == makeBinaryImage(out, 9, 8));
}

namespace
{

class Tester1
{
public:
	Tester1();
	
	bool testFullImage() const;
	
	bool testSubImage(QRect const& dst_rect, QPoint const& src_pt) const;
private:
	typedef RopXor<RopDst, RopSrc> Rop;
	
	BinaryImage m_src;
	BinaryImage m_dstBefore;
	BinaryImage m_dstAfter;
};


Tester1::Tester1()
{
	int const w = 400;
	int const h = 300;
	
	std::vector<int> src(w * h);
	for (size_t i = 0; i < src.size(); ++i) {
		src[i] = rand() & 1;
	}
	
	std::vector<int> dst(w * h);
	for (size_t i = 0; i < dst.size(); ++i) {
		dst[i] = rand() & 1;
	}
	
	std::vector<int> res(w * h);
	for (size_t i = 0; i < res.size(); ++i) {
		res[i] = Rop::transform(src[i], dst[i]) & 1;
	}
	
	m_src = makeBinaryImage(&src[0], w, h);
	m_dstBefore = makeBinaryImage(&dst[0], w, h);
	m_dstAfter = makeBinaryImage(&res[0], w, h);
}

bool
Tester1::testFullImage() const
{
	BinaryImage dst(m_dstBefore);
	rasterOp<Rop>(dst, dst.rect(), m_src, QPoint(0, 0));
	return dst == m_dstAfter;
}

bool
Tester1::testSubImage(QRect const& dst_rect, QPoint const& src_pt) const
{
	QImage const dst_before(m_dstBefore.toQImage());
	QImage dst(dst_before);
	QImage const src(m_src.toQImage());
	
	if (!check_subimage_rop<Rop>(dst, dst_rect, src, src_pt)) {
		return false;
	}
	
	return surroundingsIntact(dst, dst_before, dst_rect);
}

} // anonymous namespace

BOOST_AUTO_TEST_CASE(test_large_image)
{
	Tester1 tester;
	BOOST_REQUIRE(tester.testFullImage());
	BOOST_REQUIRE(tester.testSubImage(QRect(101, 32, 211, 151), QPoint(101, 41)));
	BOOST_REQUIRE(tester.testSubImage(QRect(101, 32, 211, 151), QPoint(99, 99)));
	BOOST_REQUIRE(tester.testSubImage(QRect(101, 32, 211, 151), QPoint(104, 64)));
}

namespace
{

class Tester2
{
public:
	Tester2();
	
	bool testBlockMove(QRect const& rect, int dx, int dy);
private:
	BinaryImage m_image;
};


Tester2::Tester2()
{
	m_image = randomBinaryImage(400, 300);
}

bool
Tester2::testBlockMove(QRect const& rect, int const dx, int const dy)
{
	BinaryImage dst(m_image);
	QRect const dst_rect(rect.translated(dx, dy));
	rasterOp<RopSrc>(dst, dst_rect, dst, rect.topLeft());
	QImage q_src(m_image.toQImage());
	QImage q_dst(dst.toQImage());
	if (q_src.copy(rect) != q_dst.copy(dst_rect)) {
		return false;
	}
	return surroundingsIntact(q_dst, q_src, dst_rect);
}

} // anonymous namespace

BOOST_AUTO_TEST_CASE(test_move_blocks)
{
	Tester2 tester;
	BOOST_REQUIRE(tester.testBlockMove(QRect(0, 0, 97, 150), 1, 0));
	BOOST_REQUIRE(tester.testBlockMove(QRect(100, 50, 15, 100), -1, 0));
	BOOST_REQUIRE(tester.testBlockMove(QRect(200, 200, 200, 100), -1, -1));
	BOOST_REQUIRE(tester.testBlockMove(QRect(51, 35, 199, 200), 0, 1));
	BOOST_REQUIRE(tester.testBlockMove(QRect(51, 35, 199, 200), 1, 1));
}

BOOST_AUTO_TEST_SUITE_END();

} // namespace tests

} // namespace imageproc
