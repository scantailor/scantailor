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
#include "Utils.h"
#include "DebugImages.h"
#include "PerformanceTimer.h"
#include "Dpm.h"
#include "imageproc/BinaryImage.h"
#include "imageproc/BinaryThreshold.h"
#include "imageproc/Binarize.h"
#include "imageproc/BWColor.h"
#include "imageproc/Transform.h"
#include "imageproc/Morphology.h"
#include "imageproc/Connectivity.h"
#include "imageproc/SeedFill.h"
#include "imageproc/Constants.h"
#include "imageproc/Grayscale.h"
#include "imageproc/GaussBlur.h"
#include <QImage>
#include <QSize>
#include <QRectF>
#include <QPointF>
#include <QPainter>
#include <QtGlobal>
#include <QDebug>
#include <vector>
#include <algorithm>
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
	m_pageRectPhys(page_rect_phys),
	m_toUncropped(pre_xform.transform())
{
	m_toUncropped *= Utils::scaleFromToDpi(pre_xform.preScaledDpi(), m_dpi);
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
			//return processAutoHalftone(input, status, dbg);
			return processColorOrGrayscale(input, status, dbg);
	}
	
	assert(!"Unreachable");
	return QImage();
}

QImage
OutputGenerator::processBitonalOrBW(QImage const& input,
	TaskStatus const& status, DebugImages* const dbg) const
{
	QImage const transformed(
		transformToGray(input, m_toUncropped, m_cropRect, Qt::white)
	);
	
	if (dbg) {
		dbg->add(transformed, "transformed");
	}
	
	status.throwIfCancelled();
	
	BinaryImage bin_img;
	switch (m_colorParams.thresholdMode()) {
		case ColorParams::OTSU:
			bin_img = binarizeOtsu(transformed);
			break;
		case ColorParams::SAUVOLA:
			bin_img = binarizeSauvola(
				transformed, calcLocalWindowSize(m_dpi)
			);
			break;
		case ColorParams::WOLF:
			bin_img = binarizeWolf(
				transformed, calcLocalWindowSize(m_dpi)
			);
			break;
	}
	assert(!bin_img.isNull());
	
	BinaryImage seed(openBrick(bin_img, from300dpi(QSize(3, 3), m_dpi)));
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
	
	status.throwIfCancelled();
	
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
	
	status.throwIfCancelled();
	
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
	
	status.throwIfCancelled();
	
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
	
	status.throwIfCancelled();
	
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
	
	status.throwIfCancelled();
	
	{
		char const pattern[] =
			"   "
			"X+X"
			"XXX";
		hitMissReplaceAllDirections(bin_img, pattern, 3, 3);
	}
	
	//hmt_timer.print("hit-miss operations: ");
#endif
	
	status.throwIfCancelled();
	
	bin_img.fillExcept(
		m_contentRect.translated(-m_cropRect.topLeft()), WHITE
	);
	
	status.throwIfCancelled();
	
	QImage q_img(bin_img.toQImage());
	if (m_colorParams.colorMode() == ColorParams::BITONAL) {
		colorizeBitonal(
			q_img, m_colorParams.lightColor(),
			m_colorParams.darkColor()
		);
	}
	
	status.throwIfCancelled();
	
	Dpm const output_dpm(m_dpi);
	q_img.setDotsPerMeterX(output_dpm.horizontal());
	q_img.setDotsPerMeterY(output_dpm.vertical());
	
	return q_img;
}

QImage
OutputGenerator::processColorOrGrayscale(QImage const& input,
	TaskStatus const& status, DebugImages* const dbg) const
{
	unsigned char const dominant_gray = calcDominantBackgroundGrayLevel(input);
	
	QImage target(m_cropRect.size(), QImage::Format_RGB32);
	target.fill(qRgb(dominant_gray, dominant_gray, dominant_gray));
	
	status.throwIfCancelled();
	
	{
		QPainter painter(&target);
		painter.setRenderHint(QPainter::SmoothPixmapTransform);
		painter.setCompositionMode(QPainter::CompositionMode_SourceOver);
		
		QTransform target_xform(m_toUncropped);
		target_xform *= QTransform().translate(-m_cropRect.left(), -m_cropRect.top());
		painter.setTransform(target_xform);
		
		painter.drawImage(QPointF(0.0, 0.0), input);
	}
	
	status.throwIfCancelled();
	
	if (target.allGray()) {
		return toGrayscale(target);
	}
	
	return target;
}

QImage
OutputGenerator::processAutoHalftone(QImage const& input,
	TaskStatus const& status, DebugImages* const dbg) const
{
#if 0
	h = fspecial('gaussian', [5, 5], 1.7);
	blurred = imfilter(A, h,'replicate');
	level = graythresh(A);
	im= im2bw(A, level);
	se = strel('disk',2);
	w_d=imdilate(A, se);
	b=imcomplement(A);
	b_d=imdilate(b, se);
	g=uint8(double(b_d).*double(w_d)/255); % OR
	se2 = strel('disk',15);
	marker=imerode(g, se2);
	p=imreconstruct(marker, g);
	g_a = imfill(p,'holes');
	level = graythresh(g_a);% replace to other thresholding algoritm, error if not pictures in image
	g_ab= im2bw(g_a,level);
	final_image=uint8((double(blurred).*double(g_ab)+255*double(im).*double(1-g_ab)));
	imwrite(final_image, 'final_image.tif')
#endif
	QImage const gray_input(toGrayscale(input));
	
	QImage const blurred(gaussBlurGray(gray_input, 4.5));
	if (dbg) {
		dbg->add(blurred, "blurred");
	}
	BinaryImage const im(binarizeOtsu(blurred));
	if (dbg) {
		dbg->add(im, "blurred_binarized");
	}
	QImage const dilated(dilateGray(gray_input, QSize(4, 4)));
	if (dbg) {
		dbg->add(dilated, "input_dilated");
	}
	QImage const eroded(erodeGray(gray_input, QSize(4, 4)));
	if (dbg) {
		dbg->add(eroded, "input_eroded");
	}
	
	QImage gray(gray_input.size(), QImage::Format_Indexed8);
	gray.setColorTable(createGrayscalePalette());
	for (int y = 0; y < gray.height(); ++y) {
		for (int x = 0; x < gray.width(); ++x) {
			unsigned const e = eroded.pixelIndex(x, y);
			unsigned const d = dilated.pixelIndex(x, y);
			gray.setPixel(x, y, (255 - e) * d / 255);
		}
	}
	if (dbg) {
		dbg->add(gray, "gray");
	}
	
	QImage const marker(dilateGray(gray, QSize(30, 30)));
	if (dbg) {
		dbg->add(marker, "marker");
	}
	
	QImage const p(seedFillGray(marker, gray, CONN4));
	if (dbg) {
		dbg->add(p, "p");
	}
	
	BinaryImage const gray_binarized(binarizeOtsu(p));
	if (dbg) {
		dbg->add(gray_binarized, "gray_binarized");
	}
	
	return input;
}

QSize
OutputGenerator::from300dpi(QSize const& size, Dpi const& target_dpi)
{
	double const hscale = target_dpi.horizontal() / 300.0;
	double const vscale = target_dpi.vertical() / 300.0;
	int const width = qRound(size.width() * hscale);
	int const height = qRound(size.height() * vscale);
	return QSize(std::max(1, width), std::max(1, height));
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

unsigned char
OutputGenerator::calcDominantBackgroundGrayLevel(QImage const& img)
{
	QImage const gray(toGrayscale(img));
	
	BinaryImage mask(binarizeOtsu(gray));
	mask.invert();
	
	GrayscaleHistogram const hist(gray, mask);
	
	int integral_hist[256];
	integral_hist[0] = hist[0];
	for (int i = 1; i < 256; ++i) {
		integral_hist[i] = hist[i] + integral_hist[i - 1];
	}
	
	int const num_colors = 256;
	int const window_size = 10;
	
	int best_pos = 0;
	int best_sum = integral_hist[window_size - 1];
	for (int i = 1; i < num_colors - window_size; ++i) {
		int const sum = integral_hist[i + window_size - 1] - integral_hist[i - 1];
		if (sum > best_sum) {
			best_sum = sum;
			best_pos = i;
		}
	}
	
	int half_sum = 0;
	for (int i = best_pos; i < best_pos + window_size; ++i) {
		half_sum += hist[i];
		if (half_sum >= best_sum / 2) {
			return i;
		}
	}
	
	assert(!"Unreachable");
	return 0;
}

} // namespace output

