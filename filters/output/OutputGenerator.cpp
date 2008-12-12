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
#include "EstimateBackground.h"
#include "Dpi.h"
#include "Dpm.h"
#include "imageproc/BinaryImage.h"
#include "imageproc/BinaryThreshold.h"
#include "imageproc/Binarize.h"
#include "imageproc/BWColor.h"
#include "imageproc/Transform.h"
#include "imageproc/Scale.h"
#include "imageproc/Morphology.h"
#include "imageproc/Connectivity.h"
#include "imageproc/SeedFill.h"
#include "imageproc/Constants.h"
#include "imageproc/Grayscale.h"
#include "imageproc/RasterOp.h"
#include "imageproc/GrayRasterOp.h"
#include "imageproc/PolynomialSurface.h"
#include "imageproc/SavGolFilter.h"
#include "imageproc/DrawOver.h"
#include "imageproc/AdjustBrightness.h"
#include <QImage>
#include <QSize>
#include <QPoint>
#include <QRect>
#include <QRectF>
#include <QPointF>
#include <QPainter>
#include <QtGlobal>
#include <QDebug>
#include <vector>
#include <algorithm>
#include <assert.h>
#include <string.h>

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
	QTransform const post_scale(
		Utils::scaleFromToDpi(pre_xform.preScaledDpi(), m_dpi)
	);
	m_toUncropped *= post_scale;
	
	m_contentRect = m_toUncropped.map(
		content_rect_phys
	).boundingRect().toRect();
	
	m_cropRect = m_toUncropped.map(
		page_rect_phys
	).boundingRect().toRect();
	
	m_fullPageRect = post_scale.map(
		pre_xform.resultingCropArea()
	).boundingRect().toRect();
}

QImage
OutputGenerator::process(QImage const& input,
	TaskStatus const& status, DebugImages* const dbg) const
{
	QImage image;
	switch (m_colorParams.colorMode()) {
		case ColorParams::BLACK_AND_WHITE:
		case ColorParams::BITONAL:
		case ColorParams::MIXED:
			image = processMixedOrBitonal(input, status, dbg);
			break;
		case ColorParams::COLOR_GRAYSCALE:
			image = processColorOrGrayscale(input, status, dbg);
			break;
	}
	assert(!image.isNull());
	
	// Set the correct DPI.
	Dpm const output_dpm(m_dpi);
	image.setDotsPerMeterX(output_dpm.horizontal());
	image.setDotsPerMeterY(output_dpm.vertical());
	
	return image;
}

QImage
OutputGenerator::processColorOrGrayscale(QImage const& input,
	TaskStatus const& status, DebugImages* const dbg) const
{
	if (input.allGray()) {
		QImage const gray_input(toGrayscale(input));
		uint8_t const dominant_gray = calcDominantBackgroundGrayLevel(gray_input);
		
		status.throwIfCancelled();
		
		return transformToGray(
			gray_input, m_toUncropped, m_cropRect,
			qRgb(dominant_gray, dominant_gray, dominant_gray)
		);
	}
	
	uint8_t const dominant_gray = calcDominantBackgroundGrayLevel(input);
	
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
	
	return target;
}

namespace
{

struct RaiseAboveBackground
{
	static uint8_t transform(uint8_t src, uint8_t dst) {
		// src: orig
		// dst: background (dst >= src)
		if (dst - src < 1) {
			return 0xff;
		}
		unsigned const orig = src;
		unsigned const background = dst;
		return static_cast<uint8_t>((orig * 255 + background / 2) / background);
	}
};

struct CombineInverted
{
	static uint8_t transform(uint8_t src, uint8_t dst) {
		unsigned const dilated = dst;
		unsigned const eroded = src;
		unsigned const res = 255 - (255 - dilated) * eroded / 255;
		return static_cast<uint8_t>(res);
	}
};

} // anonymous namespace

QImage
OutputGenerator::processMixedOrBitonal(QImage const& input,
	TaskStatus const& status, DebugImages* const dbg) const
{
	// The whole image minus the part cut off by the split line.
	QRect const big_margins_rect(m_fullPageRect);
	
	// For various reasons, we need some whitespace around the content
	// area.  This is the number of pixels of such whitespace.
	int const content_margin = 20;
	
	// The content area (in m_toUncropped coordinates) extended
	// with content_margin.  Note that we prevent that extension
	// from reaching the neighboring page.
	QRect const small_margins_rect(
		m_contentRect.adjusted(
			-content_margin, -content_margin,
			content_margin, content_margin
		).intersected(big_margins_rect)
	);
	
	// This is the area we are going to pass to estimateBackground().
	// estimateBackground() needs some margins around content, and
	// generally smaller margins are better, except when there is
	// some garbage that connects the content to the edge of the
	// image area.
	QRect const normalize_illumination_rect(
#if 1
		small_margins_rect
#else
		big_margins_rect
#endif
	);
	
	QImage to_be_normalized(
		transformToGray(
			input, m_toUncropped,
			normalize_illumination_rect,
			Qt::black // <-- Important!
		)
	);
	if (dbg) {
		dbg->add(to_be_normalized, "to_be_normalized");
	}
	
	status.throwIfCancelled();
	
	PolynomialSurface const bg_ps(
		estimateBackground(
			to_be_normalized, status, dbg
		)
	);
	
	status.throwIfCancelled();
	
	QImage normalized_illumination(
		bg_ps.render(to_be_normalized.size())
	);
	if (dbg) {
		dbg->add(normalized_illumination, "background");
	}
	
	status.throwIfCancelled();
	
	grayRasterOp<RaiseAboveBackground>(
		normalized_illumination, to_be_normalized
	);
	if (dbg) {
		dbg->add(normalized_illumination, "normalized_illumination");
	}
	
	to_be_normalized = QImage(); // Save memory.
	
	status.throwIfCancelled();
	
	QSize const savgol_window(
		from300dpi(QSize(7, 7), m_dpi).expandedTo(QSize(7, 7))
	);
	QImage normalized_and_smoothed(
		savGolFilter(normalized_illumination, savgol_window, 4, 4)
	);
	if (dbg) {
		dbg->add(normalized_and_smoothed, "smoothed");
	}
	
	status.throwIfCancelled();
	
	if (m_colorParams.colorMode() != ColorParams::MIXED) {
		// This means it's BLACK_AND_WHITE or BITONAL.
		
		BinaryImage bw_content(
			binarize(normalized_and_smoothed, m_dpi)
		);
		morphologicalSmoothInPlace(bw_content, status);
		
		BinaryImage dst(m_cropRect.size(), WHITE);
		
		QRect dst_rect(m_contentRect);
		dst_rect.moveTopLeft(
			normalize_illumination_rect.topLeft()
			- m_cropRect.topLeft()
		);
		
		QPoint const src_pos(
			m_contentRect.topLeft() -
			normalize_illumination_rect.topLeft()
		);
		rasterOp<RopSrc>(dst, dst_rect, bw_content, src_pos);
		
		bw_content.release(); // Save memory.
		
		QImage bitonal(dst.toQImage());
		
		dst.release(); // Save memory.
		
		if (m_colorParams.colorMode() == ColorParams::BITONAL) {
			colorizeBitonal(
				bitonal, m_colorParams.lightColor(),
				m_colorParams.darkColor()
			);
		}
		
		return bitonal;
	}
	
	// For detecting pictures on a scan, we need to get rid of
	// big margins.  The more extra space we have, the more chances
	// there are for false positives.  Still, because we need
	// to detect gradients, we don't want content touching the edge,
	// which means zero margins won't be OK.
	
	// If we need to strip some of the margins from a grayscale
	// image, we may actually do it without copying anything.
	// We are going to construct a QImage from existing data,
	// that image won't hold that data, so we need someone
	// else to hold it ...
	QImage holder(normalized_illumination);
	
	if (normalize_illumination_rect != small_margins_rect) {
		// The area of normalized_illumination we actually need.
		QRect subrect(small_margins_rect);
		subrect.moveTopLeft(
			small_margins_rect.topLeft() - big_margins_rect.topLeft()
		);
		
		// This prevents a copy-on-write when we access holder.bits().
		normalized_illumination = QImage();
		
		int const bpl = holder.bytesPerLine();
		normalized_illumination = QImage(
			holder.bits() + subrect.left() + subrect.top() * bpl,
			subrect.width(), subrect.height(),
			bpl, holder.format()
		);
		normalized_illumination.setColorTable(holder.colorTable());
	}
	
	status.throwIfCancelled();
	
	// A 300dpi version of normalized_illumination.
	QImage normalized_illumination_300(
		scaleToGray(
			normalized_illumination,
			to300dpi(normalized_illumination.size(), m_dpi)
		)
	);
	
	status.throwIfCancelled();
	
	// Light areas indicate pictures.
	QImage picture_areas(
		detectPictures(normalized_illumination_300, status, dbg)
	);
	normalized_illumination_300 = QImage();
	
	status.throwIfCancelled();
	
	BinaryThreshold const image_mask_threshold(
		BinaryThreshold::mokjiThreshold(picture_areas, 5, 26)
	);
	
	picture_areas = scaleToGray(
		picture_areas, normalized_illumination.size()
	);
	
	// Black areas in bw_mask indicate areas to be binarized.
	BinaryImage bw_mask(picture_areas, image_mask_threshold);
	picture_areas = QImage();
	if (dbg) {
		dbg->add(bw_mask, "bw_mask");
	}
	
	status.throwIfCancelled();
	
	GrayscaleHistogram const hist(normalized_and_smoothed, bw_mask);
	normalized_and_smoothed = QImage(); // Save memory.
	
	status.throwIfCancelled();
	
	BinaryThreshold const bw_thresh(BinaryThreshold::otsuThreshold(hist));
	BinaryImage bw_content(normalized_illumination, bw_thresh);
	morphologicalSmoothInPlace(bw_content, status);
	if (dbg) {
		dbg->add(bw_content, "bw_content");
	}
	
	status.throwIfCancelled();
	
	if (!input.allGray()) {
		bw_mask.invert();
		QImage bw_content_argb(bw_content.toQImage());
		bw_content.release();
		
		status.throwIfCancelled();
		
		bw_content_argb.setAlphaChannel(toGrayscale(bw_mask.toQImage()));
		bw_mask.release();
		
		status.throwIfCancelled();
		
		// First draw a color version of the area covered by
		// small_margins_rect here, then we are going to
		// adjust its brightness.
		QImage norm_illum_color(
			normalized_illumination.size(), QImage::Format_RGB32
		);
		norm_illum_color.fill(0xffffffff); // opaque white
		
		status.throwIfCancelled();
		
		{
			QPainter painter(&norm_illum_color);
			painter.setRenderHint(QPainter::SmoothPixmapTransform);
			
			QTransform target_xform(m_toUncropped);
			target_xform *= QTransform().translate(
				-small_margins_rect.left(),
				-small_margins_rect.top()
			);
			
			painter.setTransform(target_xform);
			painter.drawImage(QPointF(0.0, 0.0), input);
		}
		
		status.throwIfCancelled();
		
		adjustBrightnessGrayscale(
			norm_illum_color, normalized_illumination
		);
		if (dbg) {
			dbg->add(norm_illum_color, "norm_illum_color");
		}
		
		status.throwIfCancelled();
		
		QImage dst(m_cropRect.size(), QImage::Format_ARGB32_Premultiplied);
		dst.fill(0xffffffff); // opaque white
		
		status.throwIfCancelled();
		
		QPainter painter;
		painter.begin(&dst);
		painter.setRenderHint(QPainter::SmoothPixmapTransform);
		
		QRectF const clip_rect(
			m_contentRect.translated(-m_cropRect.topLeft())
		);
		painter.setClipRect(clip_rect);
		
		QPoint const draw_position(
			small_margins_rect.topLeft() - m_cropRect.topLeft()
		);
		
		// Draw the color part.
		painter.drawImage(draw_position, norm_illum_color);
		
		status.throwIfCancelled();
		
		// Draw the B/W part.
		painter.drawImage(draw_position, bw_content_argb);
		
		painter.end();
		
		return dst;
	}
	
	QImage mixed(normalized_illumination);
	
	// Save memory.
	normalized_illumination = QImage();
	holder = QImage();
	
	uint8_t* mixed_line = mixed.bits();
	int const mixed_bpl = mixed.bytesPerLine();
	uint32_t const* bw_content_line = bw_content.data();
	int const bw_content_wpl = bw_content.wordsPerLine();
	uint32_t const* bw_mask_line = bw_mask.data();
	int const bw_mask_wpl = bw_mask.wordsPerLine();
	int const width = mixed.width();
	int const height = mixed.height();
	uint32_t const msb = uint32_t(1) << 31;
	
	for (int y = 0; y < height; ++y) {
		for (int x = 0; x < width; ++x) {
			if (bw_mask_line[x >> 5] & (msb >> (x & 31))) {
				uint32_t const bit = (
					bw_content_line[x >> 5] >> (31 - (x & 31))
				) & uint32_t(1);
				
				// 1 becomes 0 (black), 0 becomes 0xff (white)
				mixed_line[x] = static_cast<uint8_t>(bit - 1);
			}
		}
		mixed_line += mixed_bpl;
		bw_content_line += bw_content_wpl;
		bw_mask_line += bw_mask_wpl;
	}
	
	// Save memory.
	bw_content.release();
	bw_mask.release();
	
	status.throwIfCancelled();
	
	QImage dst(m_cropRect.size(), QImage::Format_Indexed8);
	dst.setColorTable(createGrayscalePalette());
	dst.fill(0xff); // white.
	
	QRect src_rect(m_contentRect);
	src_rect.moveTopLeft(
		m_contentRect.topLeft() - small_margins_rect.topLeft()
	);
	
	QRect const dst_rect(m_contentRect.translated(-m_cropRect.topLeft()));
	
	drawOver(dst, dst_rect, mixed, src_rect);
	
	return dst;
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

QSize
OutputGenerator::to300dpi(QSize const& size, Dpi const& source_dpi)
{
	double const hscale = 300.0 / source_dpi.horizontal();
	double const vscale = 300.0 / source_dpi.vertical();
	int const width = qRound(size.width() * hscale);
	int const height = qRound(size.height() * vscale);
	return QSize(std::max(1, width), std::max(1, height));
}

QImage
OutputGenerator::detectPictures(
	QImage const& input_300dpi, TaskStatus const& status,
	DebugImages* const dbg)
{
	// We stretch the range of gray levels to cover the whole
	// range of [0, 255].  We do it because we want text
	// and background to be equally far from the center
	// of the whole range.  Otherwise text printed with a big
	// font will be considered a picture.
	QImage stretched(stretchGrayRange(input_300dpi, 0.01, 0.01));
	if (dbg) {
		dbg->add(stretched, "stretched");
	}
	
	status.throwIfCancelled();
	
	QImage eroded(erodeGray(stretched, QSize(3, 3), 0x00));
	if (dbg) {
		dbg->add(eroded, "eroded");
	}
	
	status.throwIfCancelled();
	
	QImage dilated(dilateGray(stretched, QSize(3, 3), 0xff));
	if (dbg) {
		dbg->add(dilated, "dilated");
	}
	
	stretched = QImage(); // Save memory.
	
	status.throwIfCancelled();
	
	grayRasterOp<CombineInverted>(dilated, eroded);
	QImage gray_gradient(dilated);
	dilated = QImage();
	eroded = QImage();
	if (dbg) {
		dbg->add(gray_gradient, "gray_gradient");
	}
	
	status.throwIfCancelled();
	
	QImage marker(erodeGray(gray_gradient, QSize(35, 35), 0x00));
	if (dbg) {
		dbg->add(marker, "marker");
	}
	
	status.throwIfCancelled();
	
	seedFillGrayInPlace(marker, gray_gradient, CONN8);
	QImage reconstructed(marker);
	marker = QImage();
	if (dbg) {
		dbg->add(reconstructed, "reconstructed");
	}
	
	status.throwIfCancelled();
	
	grayRasterOp<GRopInvert<GRopSrc> >(reconstructed, reconstructed);
	if (dbg) {
		dbg->add(reconstructed, "reconstructed_inverted");
	}
	
	status.throwIfCancelled();
	
	QImage holes_filled(createFramedImage(reconstructed.size()));
	seedFillGrayInPlace(holes_filled, reconstructed, CONN8);
	reconstructed = QImage();
	if (dbg) {
		dbg->add(holes_filled, "holes_filled");
	}
	
	return holes_filled;
}

BinaryImage
OutputGenerator::binarize(QImage const& image, Dpi const& image_dpi) const
{
	BinaryImage bin_img;
	switch (m_colorParams.thresholdMode()) {
		case ColorParams::OTSU:
			bin_img = binarizeOtsu(image);
			break;
		case ColorParams::MOKJI:
			// TODO: max_edge_width must depend on the transformation.
			bin_img = binarizeMokji(image, 5, 40);
			break;
		case ColorParams::SAUVOLA:
			bin_img = binarizeSauvola(
				image, calcLocalWindowSize(image_dpi)
			);
			break;
		case ColorParams::WOLF:
			bin_img = binarizeWolf(
				image, calcLocalWindowSize(image_dpi)
			);
			break;
	}
	assert(!bin_img.isNull());
	
	return bin_img;
}

void
OutputGenerator::morphologicalSmoothInPlace(
	BinaryImage& bin_img, TaskStatus const& status)
{
	// When removing black noise, remove small ones first.
	
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
	QSizeF const size_mm(3, 30);
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

