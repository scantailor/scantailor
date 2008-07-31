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

#include "OutputGenerator.h"
#include "ImageTransformation.h"
#include "TaskStatus.h"
#include "DebugImages.h"
#include "PerformanceTimer.h"
#include "Dpm.h"
#include "imageproc/BinaryImage.h"
#include "imageproc/Binarize.h"
#include "imageproc/BWColor.h"
#include "imageproc/Transform.h"
#include "imageproc/Morphology.h"
#include "imageproc/Connectivity.h"
#include "imageproc/SeedFill.h"
#include "imageproc/Constants.h"
#include "imageproc/Grayscale.h"
#include <QImage>
#include <QSize>
#include <QRectF>
#include <QPointF>
#include <QPainter>
#include <QDebug>
#include <vector>
#include <assert.h>

using namespace imageproc;

namespace output
{

OutputGenerator::OutputGenerator(
	Dpi const& dpi, ColorParams const& color_params,
	ImageTransformation const& pre_xform,
	QPolygonF const& content_rect_phys,
	QPolygonF const& page_rect_phys)
:	m_dpi(dpi),
	m_colorParams(color_params),
	m_pageRectPhys(page_rect_phys)
{
	ImageTransformation xform(pre_xform);
	xform.preScaleToDpi(dpi);
	
	m_toUncropped = xform.transform();
	m_cropRect = m_toUncropped.map(page_rect_phys).boundingRect().toRect();
	m_contentRect = m_toUncropped.map(content_rect_phys).boundingRect().toRect();
}

QImage
OutputGenerator::process(QImage const& input,
	TaskStatus const& status, DebugImages* const dbg) const
{
	switch (m_colorParams.colorMode()) {
		case ColorParams::BLACK_AND_WHITE:
		case ColorParams::BITONAL:
			return processBitonalOrBW(input, status, dbg);
		case ColorParams::COLOR_GRAYSCALE:
			return processColorOrGrayscale(input, status, dbg);
	}
	
	assert(!"Unreachable");
	return QImage();
}

QImage
OutputGenerator::processBitonalOrBW(QImage const& input,
	TaskStatus const& status, DebugImages* const dbg) const
{
	// TODO: calculate the dominant background color and use that.
	QColor const white(200, 200, 200); // FIXME: this may hurt binarizeWolf()
	QImage const gray_img(
		transformToGray(input, m_toUncropped, m_cropRect, white)
	);
	
	if (dbg) {
		dbg->add(gray_img, "gray_img");
	}
	
	status.throwIfCancelled();
	
	BinaryImage bin_img;
	switch (m_colorParams.thresholdMode()) {
		case ColorParams::OTSU:
			bin_img = binarizeOtsu(gray_img);
			break;
		case ColorParams::SAUVOLA:
			bin_img = binarizeSauvola(
				gray_img, calcLocalWindowSize(m_dpi)
			);
			break;
		case ColorParams::WOLF:
			bin_img = binarizeWolf(
				gray_img, calcLocalWindowSize(m_dpi)
			);
			break;
	}
	assert(!bin_img.isNull());
	
	BinaryImage seed(openBrick(bin_img, QSize(3, 3)));
	bin_img = seedFill(seed, bin_img, CONN4);
	seed.release();
	
#if 1
	//PerformanceTimer hmt_timer;
	
	// When removing black noice, remove small ones first.
	
	{
		char const pattern[] =
			"XXX"
			" - "
			"   ";
		hitMissReplaceAllDirections(bin_img, pattern, 3, 3);
	}
	
	{
		char const pattern[] =
			"X ?"
			"X  "
			"X- "
			"X- "
			"X  "
			"X ?";
		hitMissReplaceAllDirections(bin_img, pattern, 3, 6);
	}
	
	{
		char const pattern[] =
			"X ?"
			"X ?"
			"X  "
			"X- "
			"X- "
			"X- "
			"X  "
			"X ?"
			"X ?";
		hitMissReplaceAllDirections(bin_img, pattern, 3, 9);
	}
	
	{
		char const pattern[] =
			"XX?"
			"XX?"
			"XX "
			"X+ "
			"X+ "
			"X+ "
			"XX "
			"XX?"
			"XX?";
		hitMissReplaceAllDirections(bin_img, pattern, 3, 9);
	}
	
	{
		char const pattern[] =
			"XX?"
			"XX "
			"X+ "
			"X+ "
			"XX "
			"XX?";
		hitMissReplaceAllDirections(bin_img, pattern, 3, 6);
	}
	
	{
		char const pattern[] =
			"   "
			"X+X"
			"XXX";
		hitMissReplaceAllDirections(bin_img, pattern, 3, 3);
	}
	
	//hmt_timer.print("hit-miss operations: ");
#endif
	
	bin_img.fillExcept(
		m_contentRect.translated(-m_cropRect.topLeft()), WHITE
	);
	
	QImage q_img(bin_img.toQImage());
	if (m_colorParams.colorMode() == ColorParams::BITONAL) {
		colorizeBitonal(
			q_img, m_colorParams.lightColor(),
			m_colorParams.darkColor()
		);
	}
	
	Dpm const output_dpm(m_dpi);
	q_img.setDotsPerMeterX(output_dpm.horizontal());
	q_img.setDotsPerMeterY(output_dpm.vertical());
	
	return q_img;
}

QImage
OutputGenerator::processColorOrGrayscale(QImage const& input,
	TaskStatus const& status, DebugImages* const dbg) const
{
	QImage target(m_cropRect.size(), QImage::Format_RGB32);
	target.fill(0xFFFFFFFF); // FIXME: calculate the dominant color instead.
	
	{
		QPainter painter(&target);
		painter.setRenderHint(QPainter::SmoothPixmapTransform);
		painter.setCompositionMode(QPainter::CompositionMode_SourceOver);
		
		QTransform target_xform(m_toUncropped);
		target_xform *= QTransform().translate(-m_cropRect.left(), -m_cropRect.top());
		painter.setTransform(target_xform);
		
		painter.drawImage(QPointF(0.0, 0.0), input);
	}
	
	if (target.allGray()) {
		return toGrayscale(target);
	}
	
	return target;
}

void
OutputGenerator::hitMissReplaceAllDirections(
	imageproc::BinaryImage& img, char const* const pattern,
	int const pattern_width, int const pattern_height)
{
	hitMissReplaceInPlace(img, WHITE, pattern, pattern_width, pattern_height);
	
	std::vector<char> pattern_data(pattern_width * pattern_height, ' ');
	char* const new_pattern = &pattern_data[0];
	
	// Rotate 90 degrees clockwise.
	char const* p = pattern;
	int new_width = pattern_height;
	int new_height = pattern_width;
	for (int y = 0; y < pattern_height; ++y) {
		for (int x = 0; x < pattern_width; ++x, ++p) {
			int const new_x = pattern_height - 1 - y;
			int const new_y = x;
			new_pattern[new_y * new_width + new_x] = *p;
		}
	}
	hitMissReplaceInPlace(img, WHITE, new_pattern, new_width, new_height);
	
	// Rotate upside down.
	p = pattern;
	new_width = pattern_width;
	new_height = pattern_height;
	for (int y = 0; y < pattern_height; ++y) {
		for (int x = 0; x < pattern_width; ++x, ++p) {
			int const new_x = pattern_width - 1 - x;
			int const new_y = pattern_height - 1 - y;
			new_pattern[new_y * new_width + new_x] = *p;
		}
	}
	hitMissReplaceInPlace(img, WHITE, new_pattern, new_width, new_height);
	
	// Rotate 90 degrees counter-clockwise.
	p = pattern;
	new_width = pattern_height;
	new_height = pattern_width;
	for (int y = 0; y < pattern_height; ++y) {
		for (int x = 0; x < pattern_width; ++x, ++p) {
			int const new_x = y;
			int const new_y = pattern_width - 1 - x;
			new_pattern[new_y * new_width + new_x] = *p;
		}
	}
	hitMissReplaceInPlace(img, WHITE, new_pattern, new_width, new_height);
}

QSize
OutputGenerator::calcLocalWindowSize(Dpi const& dpi)
{
	QSizeF const size_mm(15, 15);
	QSizeF const size_inch(size_mm * constants::MM2INCH);
	QSizeF const size_pixels_f(
		dpi.horizontal() * size_inch.width(),
		dpi.vertical() * size_inch.height()
	);
	QSize size_pixels(size_pixels_f.toSize());
	
	if (size_pixels.width() < 3) {
		size_pixels.setWidth(3);
	}
	if (size_pixels.height() < 3) {
		size_pixels.setHeight(3);
	}
	
	return size_pixels;
}

void
OutputGenerator::colorizeBitonal(
	QImage& img, QRgb const light_color, QRgb const dark_color)
{
	if (qGray(img.color(0)) < qGray(img.color(1))) {
		img.setColor(0, dark_color);
		img.setColor(1, light_color);
	} else {
		img.setColor(0, light_color);
		img.setColor(1, dark_color);
	}
}

} // namespace output

