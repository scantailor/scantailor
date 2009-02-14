/*
    Scan Tailor - Interactive post-processing tool for scanned pages.
    Copyright (C) 2007-2009  Joseph Artsimovich <joseph_a@mail.ru>

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
#include "FilterData.h"
#include "TaskStatus.h"
#include "Utils.h"
#include "DebugImages.h"
#include "EstimateBackground.h"
#include "Despeckle.h"
#include "RenderParams.h"
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
#include "imageproc/ConnCompEraser.h"
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
#include <QtGlobal>
#include <QDebug>
#include <vector>
#include <algorithm>
#include <assert.h>
#include <string.h>
#include <stdint.h>

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
	
	// Make sure both m_cropRect and m_fullPageRect completely
	// cover m_contentRect.
	m_cropRect |= m_contentRect;
	m_fullPageRect |= m_contentRect;
}

QImage
OutputGenerator::process(FilterData const& input,
	TaskStatus const& status, DebugImages* const dbg) const
{
	QImage image(processImpl(input, status, dbg));
	assert(!image.isNull());
	
	// Set the correct DPI.
	Dpm const output_dpm(m_dpi);
	image.setDotsPerMeterX(output_dpm.horizontal());
	image.setDotsPerMeterY(output_dpm.vertical());
	
	return image;
}

QImage
OutputGenerator::processAsIs(FilterData const& input,
	TaskStatus const& status, DebugImages* const dbg) const
{
	uint8_t const dominant_gray = calcDominantBackgroundGrayLevel(input.grayImage());
	
	status.throwIfCancelled();
	
	QColor const bg_color(dominant_gray, dominant_gray, dominant_gray);
	
	if (input.origImage().allGray()) {
		return transformToGray(
			input.grayImage(), m_toUncropped, m_cropRect, bg_color
		);
	} else {
		return transform(
			input.origImage(), m_toUncropped, m_cropRect, bg_color
		);
	}
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

/**
 * Fills areas of \p mixed with pixels from \p bw_content in
 * areas where \p bw_mask is black.  Supported \p mixed image formats
 * are Indexed8 grayscale, RGB32 and ARGB32.
 * The \p MixedPixel type is uint8_t for Indexed8 grayscale and uint32_t
 * for RGB32 and ARGB32.
 */
template<typename MixedPixel>
void combineMixed(
	QImage& mixed, BinaryImage const& bw_content,
	BinaryImage const& bw_mask)
{
	MixedPixel* mixed_line = reinterpret_cast<MixedPixel*>(mixed.bits());
	int const mixed_stride = mixed.bytesPerLine() / sizeof(MixedPixel);
	uint32_t const* bw_content_line = bw_content.data();
	int const bw_content_stride = bw_content.wordsPerLine();
	uint32_t const* bw_mask_line = bw_mask.data();
	int const bw_mask_stride = bw_mask.wordsPerLine();
	int const width = mixed.width();
	int const height = mixed.height();
	uint32_t const msb = uint32_t(1) << 31;
	
	for (int y = 0; y < height; ++y) {
		for (int x = 0; x < width; ++x) {
			if (bw_mask_line[x >> 5] & (msb >> (x & 31))) {
				uint32_t tmp = bw_content_line[x >> 5];
				tmp >>= (31 - (x & 31));
				tmp &= uint32_t(1);
				// Now it's 0 for white and 1 for black.
				
				--tmp; // 0 becomes 0xffffffff and 1 becomes 0.
				
				tmp |= 0xff000000; // Force opacity.
				
				mixed_line[x] = static_cast<MixedPixel>(tmp);
			}
		}
		mixed_line += mixed_stride;
		bw_content_line += bw_content_stride;
		bw_mask_line += bw_mask_stride;
	}
}

} // anonymous namespace

QImage
OutputGenerator::normalizeIlluminationGray(
	TaskStatus const& status,
	QImage const& input, QTransform const& xform,
	QRect const& target_rect, DebugImages* const dbg)
{
	QImage to_be_normalized(
		transformToGray(
			input, xform, target_rect,
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
	
	QImage background(bg_ps.render(to_be_normalized.size()));
	if (dbg) {
		dbg->add(background, "background");
	}
	
	status.throwIfCancelled();
	
	grayRasterOp<RaiseAboveBackground>(background, to_be_normalized);
	if (dbg) {
		dbg->add(background, "normalized_illumination");
	}
	
	return background;
}

imageproc::BinaryImage
OutputGenerator::estimateBinarizationMask(
	TaskStatus const& status, QImage const& gray_source,
	QRect const& source_rect, QRect const& source_sub_rect,
	DebugImages* const dbg) const
{
	assert(gray_source.format() == QImage::Format_Indexed8);
	assert(source_rect.contains(source_sub_rect));
	
	// If we need to strip some of the margins from a grayscale
	// image, we may actually do it without copying anything.
	// We are going to construct a QImage from existing data.
	// That image won't own that data, but gray_source is not
	// going anywhere, so it's fine.
	
	QImage trimmed_image;
	
	if (source_rect == source_sub_rect) {
		trimmed_image = gray_source; // Shallow copy.
	} else {
		// Sub-rectangle in input image coordinates.
		QRect relative_subrect(source_sub_rect);
		relative_subrect.moveTopLeft(
			source_sub_rect.topLeft() - source_rect.topLeft()
		);
		
		int const bpl = gray_source.bytesPerLine();
		int const offset = relative_subrect.top() * bpl
				+ relative_subrect.left();
		
		trimmed_image = QImage(
			gray_source.bits() + offset,
			relative_subrect.width(), relative_subrect.height(),
			bpl, gray_source.format()
		);
		trimmed_image.setColorTable(gray_source.colorTable());
	}
	
	status.throwIfCancelled();
	
	QSize const downscaled_size(to300dpi(trimmed_image.size(), m_dpi));
	
	// A 300dpi version of trimmed_image.
	QImage downscaled_input(
		scaleToGray(trimmed_image, downscaled_size)
	);
	trimmed_image = QImage(); // Save memory.
	
	status.throwIfCancelled();
	
	// Light areas indicate pictures.
	QImage picture_areas(
		detectPictures(downscaled_input, status, dbg)
	);
	downscaled_input = QImage(); // Save memory.
	
	status.throwIfCancelled();
	
	BinaryThreshold const threshold(
		BinaryThreshold::mokjiThreshold(picture_areas, 5, 26)
	);
	
	// Scale back to original size.
	picture_areas = scaleToGray(
		picture_areas, source_sub_rect.size()
	);
	
	return BinaryImage(picture_areas, threshold);
}

QImage
OutputGenerator::processImpl(FilterData const& input,
	TaskStatus const& status, DebugImages* const dbg) const
{
	RenderParams const render_params(m_colorParams);
	
	if (!render_params.whiteMargins()) {
		return processAsIs(input, status, dbg);
	}
	
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
	
	QImage maybe_normalized;
	
	if (render_params.normalizeIllumination()) {
		maybe_normalized = normalizeIlluminationGray(
			status, input.grayImage(), m_toUncropped,
			normalize_illumination_rect, dbg
		);
	} else {
		maybe_normalized = transform(
			input.origImage(), m_toUncropped,
			normalize_illumination_rect, Qt::white
		);
	}
	
	status.throwIfCancelled();
	
	QImage maybe_smoothed;
	
	// We only do smoothing if we are going to do binarization later.
	if (!render_params.needBinarization()) {
		maybe_smoothed = maybe_normalized;
	} else {
		maybe_smoothed = smoothToGrayscale(maybe_normalized, m_dpi);
		if (dbg) {
			dbg->add(maybe_smoothed, "smoothed");
		}
	}
	
	status.throwIfCancelled();
	
	if (render_params.binaryOutput()) {
		BinaryImage dst(m_cropRect.size(), WHITE);
		
		if (!m_contentRect.isEmpty()) {
			BinaryImage bw_content(binarize(maybe_smoothed, m_dpi));
			if (dbg) {
				dbg->add(bw_content, "bw_content");
			}
			status.throwIfCancelled();
			
			if (render_params.despeckle()) {
				despeckleInPlace(bw_content, m_dpi, status, dbg);
				if (dbg) {
					dbg->add(bw_content, "despeckled");
				}
			}
			
			status.throwIfCancelled();
			
			morphologicalSmoothInPlace(bw_content, status);
			if (dbg) {
				dbg->add(bw_content, "edges_smoothed");
			}
			
			status.throwIfCancelled();
			
			QRect dst_rect(m_contentRect);
			dst_rect.moveTopLeft(m_contentRect.topLeft() - m_cropRect.topLeft());
			
			QPoint const src_pos(
				m_contentRect.topLeft() -
				normalize_illumination_rect.topLeft()
			);
			
			rasterOp<RopSrc>(dst, dst_rect, bw_content, src_pos);
		}
		
		QImage bitonal(dst.toQImage());
		
		dst.release(); // Save memory.
#if 0
		if (m_colorParams.colorMode() == ColorParams::BITONAL) {
			colorizeBitonal(
				bitonal, m_colorParams.lightColor(),
				m_colorParams.darkColor()
			);
		}
#endif
		return bitonal;
	}
	
	BinaryImage bw_mask;
	if (render_params.mixedOutput()) {
		// This should go before the block with
		// adjustBrightnessGrayscale(), it may convert
		// maybe_normalized from grayscale to color mode.
		bw_mask = estimateBinarizationMask(
			status, toGrayscale(maybe_normalized),
			normalize_illumination_rect,
			small_margins_rect, dbg
		);
		if (dbg) {
			dbg->add(bw_mask, "bw_mask");
		}
		
		status.throwIfCancelled();
	}
	
	if (render_params.normalizeIllumination()
			&& !input.origImage().allGray()) {
		assert(maybe_normalized.format() == QImage::Format_Indexed8);
		QImage tmp(
			transform(
				input.origImage(), m_toUncropped,
				normalize_illumination_rect,
				Qt::white
			)
		);
		
		status.throwIfCancelled();
		
		adjustBrightnessGrayscale(tmp, maybe_normalized);
		maybe_normalized = tmp;
		if (dbg) {
			dbg->add(maybe_normalized, "norm_illum_color");
		}
	}
	
	if (render_params.mixedOutput()) {
		GrayscaleHistogram const hist(maybe_smoothed, bw_mask);
		maybe_smoothed = QImage(); // Save memory.
		
		status.throwIfCancelled();
		
		BinaryThreshold bw_thresh(
			BinaryThreshold::otsuThreshold(hist)
		);
		int const adjusted_thresh = bw_thresh +
			m_colorParams.blackWhiteOptions().thresholdAdjustment();
		bw_thresh = BinaryThreshold(qBound(0, adjusted_thresh, 255));
		
		status.throwIfCancelled();
		
		BinaryImage bw_content(maybe_normalized, bw_thresh);
		if (dbg) {
			dbg->add(bw_content, "bw_content");
		}
		
		status.throwIfCancelled();
		
		if (render_params.despeckle()) {
			despeckleInPlace(bw_content, m_dpi, status, dbg);
			if (dbg) {
				dbg->add(bw_content, "despeckled");
			}
		}
		
		status.throwIfCancelled();
		
		morphologicalSmoothInPlace(bw_content, status);
		if (dbg) {
			dbg->add(bw_content, "edges_smoothed");
		}
		
		status.throwIfCancelled();
		
		if (maybe_normalized.format() == QImage::Format_Indexed8) {
			combineMixed<uint8_t>(
				maybe_normalized, bw_content, bw_mask
			);
		} else {
			assert(maybe_normalized.format() == QImage::Format_RGB32
				|| maybe_normalized.format() == QImage::Format_ARGB32);
			
			combineMixed<uint32_t>(
				maybe_normalized, bw_content, bw_mask
			);
		}
	}
	
	status.throwIfCancelled();
	
	QImage dst(m_cropRect.size(), maybe_normalized.format());
	if (maybe_normalized.format() == QImage::Format_Indexed8) {
		dst.setColorTable(createGrayscalePalette());
		dst.fill(0xff); // White.
	} else {
		dst.fill(0xffffffff); // Opaque white.
	}
	
	if (!m_contentRect.isEmpty()) {
		QRect src_rect(m_contentRect);
		src_rect.moveTopLeft(
			m_contentRect.topLeft() - small_margins_rect.topLeft()
		);
		
		QRect const dst_rect(m_contentRect.translated(-m_cropRect.topLeft()));
		drawOver(dst, dst_rect, maybe_normalized, src_rect);
	}
	
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

QImage
OutputGenerator::smoothToGrayscale(QImage const& src, Dpi const& dpi)
{
	int const min_dpi = std::min(dpi.horizontal(), dpi.vertical());
	int window;
	int degree;
	if (min_dpi <= 200) {
		window = 5;
		degree = 3;
	} else if (min_dpi <= 400) {
		window = 7;
		degree = 4;
	} else if (min_dpi <= 800) {
		window = 11;
		degree = 4;
	} else {
		window = 11;
		degree = 2;
	}
	return savGolFilter(src, QSize(window, window), degree, degree);
}

BinaryImage
OutputGenerator::binarize(QImage const& image, Dpi const& image_dpi) const
{
	BinaryThreshold bw_thresh(
		BinaryThreshold::otsuThreshold(image)
	);
	int const adjusted_thresh = bw_thresh +
		m_colorParams.blackWhiteOptions().thresholdAdjustment();
	bw_thresh = BinaryThreshold(qBound(0, adjusted_thresh, 255));
	return BinaryImage(image, bw_thresh);
}

/**
 * Remove small connected components that are considered to be garbage.
 * Both the size and the distance to other components are taken into account.
 * \note This function only works effectively when the DPI is symmetric,
 * that is, its horizontal and vertical components are equal.
 */
void
OutputGenerator::despeckleInPlace(
	imageproc::BinaryImage& image, Dpi const& dpi,
	TaskStatus const& status, DebugImages* dbg)
{
	int const min_dpi = std::min(dpi.horizontal(), dpi.vertical());
	double const factor = min_dpi / 300.0;
	double const squared_factor = factor * factor;
	
	int const big_object_threshold = qRound(100 * squared_factor);
	::despeckleInPlace(image, big_object_threshold, dbg);
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
