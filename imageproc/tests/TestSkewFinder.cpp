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

#include "SkewFinder.h"
#include "BinaryImage.h"
#include <QApplication>
#include <QImage>
#include <QPainter>
#include <QTransform>
#include <QColor>
#include <QString>
#ifndef Q_MOC_RUN
#include <boost/test/auto_unit_test.hpp>
#endif
#include <math.h>
#include <stdlib.h>

namespace imageproc
{

namespace tests
{

BOOST_AUTO_TEST_SUITE(SkewFinderTestSuite);

BOOST_AUTO_TEST_CASE(test_positive_detection)
{
	int argc = 1;
	char argv0[] = "test";
	char* argv[1] = { argv0 };
	QApplication app(argc, argv);
	QImage image(1000, 800, QImage::Format_ARGB32_Premultiplied);
	image.fill(0xffffffff);
	{
		QPainter painter(&image);
		painter.setPen(QColor(0, 0, 0));
		QTransform xform1;
		xform1.translate(-0.5 * image.width(), -0.5 * image.height());
		QTransform xform2;
		xform2.rotate(4.5);
		QTransform xform3;
		xform3.translate(0.5 * image.width(), 0.5 * image.height());
		painter.setWorldTransform(xform1 * xform2 * xform3);
		
		QString text;
		for (int line = 0; line < 40; ++line) {
			for (int i = 0; i < 100; ++i) {
				text += '1';
			}
			text += '\n';
		}
		QTextOption opt;
		opt.setAlignment(Qt::AlignHCenter|Qt::AlignVCenter);
		painter.drawText(image.rect(), text, opt);
	}
	
	SkewFinder skew_finder;
	Skew const skew(skew_finder.findSkew(BinaryImage(image)));
	BOOST_REQUIRE(fabs(skew.angle() - 4.5) < 0.15);
	BOOST_CHECK(skew.confidence() >= Skew::GOOD_CONFIDENCE);
}

BOOST_AUTO_TEST_CASE(test_negative_detection)
{
	QImage image(1000, 800, QImage::Format_Mono);
	image.fill(1);
	
	int const num_dots = image.width() * image.height() / 5;
	for (int i = 0; i < num_dots; ++i) {
		int const x = rand() % image.width();
		int const y = rand() % image.height();
		image.setPixel(x, y, 0);
	}
	
	SkewFinder skew_finder;
	skew_finder.setCoarseReduction(0);
	skew_finder.setFineReduction(0);
	Skew const skew(skew_finder.findSkew(BinaryImage(image)));
	BOOST_CHECK(skew.confidence() < Skew::GOOD_CONFIDENCE);
}

BOOST_AUTO_TEST_SUITE_END();

} // namespace tests

} // namespace imageproc
