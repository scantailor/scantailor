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
#include "BinaryThreshold.h"
#include "RasterOp.h"
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
#ifndef Q_MOC_RUN
#include <boost/test/auto_unit_test.hpp>
#endif

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

static bool fuzzyCompare(BinaryImage const& img, QImage const& control)
{
	// Make two binary images from the QImage with slightly different thresholds.
	BinaryImage control1(control, BinaryThreshold(128 - 30));
	BinaryImage control2(control, BinaryThreshold(128 + 30));
	
	// Take the difference with each control image.
	rasterOp<RopXor<RopSrc, RopDst> >(control1, img);
	rasterOp<RopXor<RopSrc, RopDst> >(control2, img);
	
	// Are there pixels different in both cases?
	rasterOp<RopAnd<RopSrc, RopDst> >(control1, control2);
	
	return control1.countBlackPixels() == 0;
}

static bool testFillShape(
	QSize const& image_size, QPolygonF const& shape, Qt::FillRule fill_rule)
{
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
	
	return fuzzyCompare(b_image, q_image);
}

static bool testFillExceptShape(
	QSize const& image_size, QPolygonF const& shape, Qt::FillRule fill_rule)
{
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
	
	return fuzzyCompare(b_image, q_image);
}

BOOST_AUTO_TEST_CASE(test_complex_shape)
{
	QSize const image_size(500, 500);
	
	// This one fits the image.
	QPolygonF const smaller_shape(createShape(image_size, 230));
	
	// This one doesn't fit the image and will be clipped.
	QPolygonF const bigger_shape(createShape(image_size, 300));
	
	BOOST_CHECK(testFillShape(image_size, smaller_shape, Qt::OddEvenFill));
	BOOST_CHECK(testFillShape(image_size, smaller_shape, Qt::WindingFill));
	BOOST_CHECK(testFillShape(image_size, bigger_shape, Qt::OddEvenFill));
	BOOST_CHECK(testFillShape(image_size, bigger_shape, Qt::WindingFill));
	BOOST_CHECK(testFillExceptShape(image_size, smaller_shape, Qt::OddEvenFill));
	BOOST_CHECK(testFillExceptShape(image_size, smaller_shape, Qt::WindingFill));
	BOOST_CHECK(testFillExceptShape(image_size, bigger_shape, Qt::OddEvenFill));
	BOOST_CHECK(testFillExceptShape(image_size, bigger_shape, Qt::WindingFill));
}

BOOST_AUTO_TEST_CASE(test_corner_cases)
{
	QSize const image_size(500, 500);
	QPolygonF const shape(QRectF(QPointF(0, 0), image_size));
	QPolygonF const shape2(QRectF(QPointF(-1, -1), image_size));
	
	// This one touches clip rectangle's corners.
	QPolygonF shape3;
	shape3.push_back(QPointF(-250.0, 250.0));
	shape3.push_back(QPointF(250.0, -250.0));
	shape3.push_back(QPointF(750.0, -250.0));
	shape3.push_back(QPointF(-250.0, 750.0));
	
	BOOST_CHECK(testFillShape(image_size, shape, Qt::OddEvenFill));
	BOOST_CHECK(testFillShape(image_size, shape, Qt::WindingFill));
	BOOST_CHECK(testFillShape(image_size, shape2, Qt::OddEvenFill));
	BOOST_CHECK(testFillShape(image_size, shape2, Qt::WindingFill));
	BOOST_CHECK(testFillShape(image_size, shape3, Qt::OddEvenFill));
	BOOST_CHECK(testFillShape(image_size, shape3, Qt::WindingFill));
	BOOST_CHECK(testFillExceptShape(image_size, shape, Qt::OddEvenFill));
	BOOST_CHECK(testFillExceptShape(image_size, shape, Qt::WindingFill));
	BOOST_CHECK(testFillExceptShape(image_size, shape2, Qt::OddEvenFill));
	BOOST_CHECK(testFillExceptShape(image_size, shape2, Qt::WindingFill));
	BOOST_CHECK(testFillExceptShape(image_size, shape3, Qt::OddEvenFill));
	BOOST_CHECK(testFillExceptShape(image_size, shape3, Qt::WindingFill));
}

BOOST_AUTO_TEST_CASE(regression_test_1)
{
	QPolygonF shape;
	shape.push_back(QPointF(937.872, 24.559));
	shape.push_back(QPointF(-1.23235e-14, -1.70697e-15));
	shape.push_back(QPointF(2.73578e-11, 1275.44));
	shape.push_back(QPointF(904.496, 1299.12));
	shape.push_back(QPointF(937.872, 24.559));
	BOOST_CHECK(testFillExceptShape(QSize(938, 1299), shape, Qt::WindingFill));
}

BOOST_AUTO_TEST_SUITE_END();

} // namespace tests

} // namespace imageproc
