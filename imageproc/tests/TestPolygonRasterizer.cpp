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

#include "PolygonRasterizer.h"
#include "BinaryImage.h"
#include "RasterOp.h"
#include "Morphology.h"
#include "BWColor.h"
#include "Utils.h"
#include <QPolygonF>
#include <QSize>
#include <QRectF>
#include <QPointF>
#include <QImage>
#include <QPainter>
#include <QBrush>
#include <QColor>
#include <Qt>
#include <math.h>
#include <boost/test/auto_unit_test.hpp>

namespace imageproc
{

namespace tests
{

using namespace utils;

BOOST_AUTO_TEST_SUITE(PolygonRasterizerTestSuite);

static QPolygonF createShape(QSize const& image_size, double radius)
{
	QPointF const center(0.5 * image_size.width(), 0.5 * image_size.height());
	double const PI = 3.14159265;
	double angle = PI / 2.0;
	int const num_steps = 5;
	double const step = PI * 2.0 / num_steps;
	
	QPolygonF poly;
	
	poly.push_back(center + QPointF(cos(angle), sin(angle)) * radius);
	for (int i = 1; i < num_steps; ++i) {
		angle += step * 2;
		poly.push_back(center + QPointF(cos(angle), sin(angle)) * radius);
	}
	
	return poly;
}

static bool fuzzyCompare(BinaryImage const& img1, BinaryImage const& img2)
{
	// Get pixels that are different in img1 and img2.
	BinaryImage diff(img1);
	rasterOp<RopXor<RopSrc, RopDst> >(diff, img2);
	
	// If there are at least 2 adjacent pixels that are different
	// (either horizontally or vertically adjacent), then we consider
	// the images to be significantly different.
	
	BinaryImage const hor_eroded(erodeBrick(diff, Brick(QSize(2, 1))));
	if (hor_eroded.countBlackPixels() != 0) {
		return false;
	}
	
	BinaryImage const ver_eroded(erodeBrick(diff, Brick(QSize(1, 2))));
	if (ver_eroded.countBlackPixels() != 0) {
		return false;
	}
	
	return true;
}

static bool testFillShape(
	QSize const& image_size, double radius, Qt::FillRule fill_rule)
{
	QPolygonF const shape(createShape(image_size, radius));
	
	BinaryImage b_image(image_size, WHITE);
	PolygonRasterizer::fill(b_image, BLACK, shape, fill_rule);
	
	QImage q_image(image_size, QImage::Format_RGB32);
	q_image.fill(0xffffffff);
	
	{
		QPainter painter(&q_image);
		painter.setRenderHint(QPainter::Antialiasing, true);
		painter.setBrush(QColor(0x00, 0x00, 0x00));
		painter.setPen(Qt::NoPen);
		painter.drawPolygon(shape, fill_rule);
	}
	
	return fuzzyCompare(b_image, BinaryImage(q_image));
}

static bool testFillExceptShape(
	QSize const& image_size, double radius, Qt::FillRule fill_rule)
{
	QPolygonF const shape(createShape(image_size, radius));
	
	BinaryImage b_image(image_size, WHITE);
	PolygonRasterizer::fillExcept(b_image, BLACK, shape, fill_rule);
	
	QImage q_image(image_size, QImage::Format_RGB32);
	q_image.fill(0x00000000);
	
	{
		QPainter painter(&q_image);
		painter.setRenderHint(QPainter::Antialiasing, true);
		painter.setBrush(QColor(0xff, 0xff, 0xff));
		painter.setPen(Qt::NoPen);
		painter.drawPolygon(shape, fill_rule);
	}
	
	return fuzzyCompare(b_image, BinaryImage(q_image));
}

BOOST_AUTO_TEST_CASE(test)
{
	QSize image_size(500, 500);
	
	// Shapes of radius 230 fit inside the image, while those
	// with radius 300 are clipped.
	
	BOOST_CHECK(testFillShape(image_size, 230, Qt::OddEvenFill));
	BOOST_CHECK(testFillShape(image_size, 230, Qt::WindingFill));
	BOOST_CHECK(testFillShape(image_size, 300, Qt::OddEvenFill));
	BOOST_CHECK(testFillShape(image_size, 300, Qt::WindingFill));
	BOOST_CHECK(testFillExceptShape(image_size, 230, Qt::OddEvenFill));
	BOOST_CHECK(testFillExceptShape(image_size, 230, Qt::WindingFill));
	BOOST_CHECK(testFillExceptShape(image_size, 300, Qt::OddEvenFill));
	BOOST_CHECK(testFillExceptShape(image_size, 300, Qt::WindingFill));
}

BOOST_AUTO_TEST_SUITE_END();

} // namespace tests

} // namespace imageproc
