/*
    Scan Tailor - Interactive post-processing tool for scanned pages.
    Copyright (C)  Joseph Artsimovich <joseph.artsimovich@gmail.com>

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
#include "dewarping/DistortionModel.h"
#include "Dpi.h"
#include "Dpm.h"
#include "Zone.h"
#include "ZoneSet.h"
#include "PictureLayerProperty.h"
#include "FillColorProperty.h"
#include "dewarping/CylindricalSurfaceDewarper.h"
#include "dewarping/TextLineTracer.h"
#include "dewarping/TopBottomEdgeTracer.h"
#include "dewarping/DistortionModelBuilder.h"
#include "dewarping/DewarpingPointMapper.h"
#include "dewarping/RasterDewarper.h"
#include "imageproc/GrayImage.h"
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
#include "imageproc/PolygonRasterizer.h"
#include "imageproc/ConnectivityMap.h"
#include "imageproc/InfluenceMap.h"
#include "config.h"
#ifndef Q_MOC_RUN
#include <boost/foreach.hpp>
#include <boost/bind.hpp>
#include <boost/shared_ptr.hpp>
#endif
#include <QImage>
#include <QSize>
#include <QPoint>
#include <QRect>
#include <QRectF>
#include <QPointF>
#include <QPolygonF>
#include <QPainter>
#include <QColor>
#include <QPen>
#include <QBrush>
#include <QtGlobal>
#include <QDebug>
#include <Qt>
#include <vector>
#include <memory>
#include <new>
#include <algorithm>
#include <assert.h>
#include <string.h>
#include <stdint.h>
//begin of modified by monday2000
//Marginal_Dewarping
#include "imageproc/OrthogonalRotation.h"
//end of modified by monday2000

using namespace imageproc;
using namespace dewarping;

namespace output
{

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
 * In picture areas we make sure we don't use pure black and pure white colors.
 * These are reserved for text areas.  This behaviour makes it possible to
 * detect those picture areas later and treat them differently, for example
 * encoding them as a background layer in DjVu format.
 */
template<typename PixelType>
PixelType reserveBlackAndWhite(PixelType color);

template<>
uint32_t reserveBlackAndWhite(uint32_t color)
{
	// We handle both RGB32 and ARGB32 here.
	switch (color & 0x00FFFFFF) {
		case 0x00000000:
			return 0xFF010101;
		case 0x00FFFFFF:
			return 0xFFFEFEFE;
		default:
			return color;
	}
}

template<>
uint8_t reserveBlackAndWhite(uint8_t color)
{
	switch (color) {
		case 0x00:
			return 0x01;
		case 0xFF:
			return 0xFE;
		default:
			return color;
	}
}

template<typename PixelType>
void reserveBlackAndWhite(QSize size, int stride, PixelType* data)
{
	int const width = size.width();
	int const height = size.height();

	PixelType* line = data;
	for (int y = 0; y < height; ++y, line += stride) {
		for (int x = 0; x < width; ++x) {
			line[x] = reserveBlackAndWhite<PixelType>(line[x]);
		}
	}
}

void reserveBlackAndWhite(QImage& img)
{
	assert(img.depth() == 8 || img.depth() == 24 || img.depth() == 32);
	switch (img.format()) {
		case QImage::Format_Indexed8:
			reserveBlackAndWhite(img.size(), img.bytesPerLine(), img.bits());
			break;
		case QImage::Format_RGB32:
		case QImage::Format_ARGB32:
			reserveBlackAndWhite(img.size(), img.bytesPerLine()/4, (uint32_t*)img.bits());
			break;
		default:; // Should not happen.
	}
}

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
//begin of modified by monday2000
//Dont_Equalize_Illumination_Pic_Zones
	//BinaryImage const& bw_mask)
	BinaryImage const& bw_mask,
//added:
	QImage& original_image,
	bool dont_equalize_illumination_pic_zones
	)
//end of modified by monday2000
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
//begin of modified by monday2000
//Dont_Equalize_Illumination_Pic_Zones
//added:	
	MixedPixel* original_image_line = reinterpret_cast<MixedPixel*>(original_image.bits());	
	int const original_image_stride = original_image.bytesPerLine() / sizeof(MixedPixel);
//end of modified by monday2000

	for (int y = 0; y < height; ++y) {
		for (int x = 0; x < width; ++x) {
			if (bw_mask_line[x >> 5] & (msb >> (x & 31))) {
				// B/W content.

				uint32_t tmp = bw_content_line[x >> 5];
				tmp >>= (31 - (x & 31));
				tmp &= uint32_t(1);
				// Now it's 0 for white and 1 for black.
				
				--tmp; // 0 becomes 0xffffffff and 1 becomes 0.
				
				tmp |= 0xff000000; // Force opacity.
				
				mixed_line[x] = static_cast<MixedPixel>(tmp);
			} else {
				// Non-B/W content.
//begin of modified by monday2000
//Dont_Equalize_Illumination_Pic_Zones
				//mixed_line[x] = reserveBlackAndWhite<MixedPixel>(mixed_line[x]);
				if (dont_equalize_illumination_pic_zones)
					mixed_line[x] = reserveBlackAndWhite<MixedPixel>(original_image_line[x]);
				else
					mixed_line[x] = reserveBlackAndWhite<MixedPixel>(mixed_line[x]);
//end of modified by monday2000
			}
		}
		mixed_line += mixed_stride;
		bw_content_line += bw_content_stride;
		bw_mask_line += bw_mask_stride;
//begin of modified by monday2000
//Dont_Equalize_Illumination_Pic_Zones
//added:
		original_image_line += original_image_stride;
//end of modified by monday2000
	}
}

} // anonymous namespace


OutputGenerator::OutputGenerator(
	Dpi const& dpi, ColorParams const& color_params,
	DespeckleLevel const despeckle_level,
	ImageTransformation const& xform,
	QPolygonF const& content_rect_phys)
:	m_dpi(dpi),
	m_colorParams(color_params),
	m_xform(xform),
	m_outRect(xform.resultingRect().toRect()),
	m_contentRect(xform.transform().map(content_rect_phys).boundingRect().toRect()),
	m_despeckleLevel(despeckle_level)
{	
	assert(m_outRect.topLeft() == QPoint(0, 0));

	// Note that QRect::contains(<empty rect>) always returns false, so we don't use it here.
	assert(m_outRect.contains(m_contentRect.topLeft()) && m_outRect.contains(m_contentRect.bottomRight()));
}

QImage
OutputGenerator::process(
	TaskStatus const& status, FilterData const& input,
//Quadro_Zoner
	//ZoneSet const& picture_zones, ZoneSet const& fill_zones,
	ZoneSet& picture_zones, ZoneSet const& fill_zones,
	DewarpingMode dewarping_mode,
	DistortionModel& distortion_model,
	DepthPerception const& depth_perception,
//begin of modified by monday2000
//Dont_Equalize_Illumination_Pic_Zones
//Original_Foreground_Mixed
//added:	
	bool dont_equalize_illumination_pic_zones,
	bool keep_orig_fore_subscan,
//end of modified by monday2000
	imageproc::BinaryImage* auto_picture_mask,
	imageproc::BinaryImage* speckles_image,
//begin of modified by monday2000
//Picture_Shape
	//DebugImages* const dbg) const
	DebugImages* const dbg,
	PictureShape picture_shape
//Quadro_Zoner
	, PageId* p_pageId,
	IntrusivePtr<Settings>* p_settings
	) const
//end of modified by monday2000
{
	QImage image(
		processImpl(
			status, input, picture_zones, fill_zones,
			dewarping_mode, distortion_model, depth_perception,
//begin of modified by monday2000
//Dont_Equalize_Illumination_Pic_Zones
//Original_Foreground_Mixed
//added:		
			dont_equalize_illumination_pic_zones,
			keep_orig_fore_subscan,
//end of modified by monday2000
			auto_picture_mask, speckles_image, dbg
//begin of modified by monday2000
//Picture_Shape
			, picture_shape
//Quadro_Zoner
			, p_pageId,
			p_settings
//end of modified by monday2000
		)
	);
	assert(!image.isNull());
	
	// Set the correct DPI.
	Dpm const output_dpm(m_dpi);
	image.setDotsPerMeterX(output_dpm.horizontal());
	image.setDotsPerMeterY(output_dpm.vertical());
	
	return image;
}

QSize
OutputGenerator::outputImageSize() const
{
	return m_outRect.size();
}

QRect
OutputGenerator::outputContentRect() const
{
	return m_contentRect;
}

GrayImage
OutputGenerator::normalizeIlluminationGray(
	TaskStatus const& status,
	QImage const& input, QPolygonF const& area_to_consider,
	QTransform const& xform, QRect const& target_rect,
	GrayImage* background, DebugImages* const dbg)
{
	GrayImage to_be_normalized(
		transformToGray(
			input, xform, target_rect, OutsidePixels::assumeWeakNearest()
		)
	);
	if (dbg) {
		dbg->add(to_be_normalized, "to_be_normalized");
	}

	status.throwIfCancelled();
	
	QPolygonF transformed_consideration_area(xform.map(area_to_consider));
	transformed_consideration_area.translate(-target_rect.topLeft());
	
	PolynomialSurface const bg_ps(
		estimateBackground(
			to_be_normalized, transformed_consideration_area,
			status, dbg
		)
	);
	
	status.throwIfCancelled();
	
	GrayImage bg_img(bg_ps.render(to_be_normalized.size()));
	if (dbg) {
		dbg->add(bg_img, "background");
	}
	if (background) {
		*background = bg_img;
	}
	
	status.throwIfCancelled();
	
	grayRasterOp<RaiseAboveBackground>(bg_img, to_be_normalized);
	if (dbg) {
		dbg->add(bg_img, "normalized_illumination");
	}
	
	return bg_img;
}

imageproc::BinaryImage
OutputGenerator::estimateBinarizationMask(
	TaskStatus const& status, GrayImage const& gray_source,
	QRect const& source_rect, QRect const& source_sub_rect,
	DebugImages* const dbg) const
{
	assert(source_rect.contains(source_sub_rect));
	
	// If we need to strip some of the margins from a grayscale
	// image, we may actually do it without copying anything.
	// We are going to construct a QImage from existing data.
	// That image won't own that data, but gray_source is not
	// going anywhere, so it's fine.
	
	GrayImage trimmed_image;
	
	if (source_rect == source_sub_rect) {
		trimmed_image = gray_source; // Shallow copy.
	} else {
		// Sub-rectangle in input image coordinates.
		QRect relative_subrect(source_sub_rect);
		relative_subrect.moveTopLeft(
			source_sub_rect.topLeft() - source_rect.topLeft()
		);
		
		int const stride = gray_source.stride();
		int const offset = relative_subrect.top() * stride
				+ relative_subrect.left();
		
		trimmed_image = GrayImage(QImage(
			gray_source.data() + offset,
			relative_subrect.width(), relative_subrect.height(),
			stride, QImage::Format_Indexed8
		));
	}
	
	status.throwIfCancelled();
	
	QSize const downscaled_size(to300dpi(trimmed_image.size(), m_dpi));
	
	// A 300dpi version of trimmed_image.
	GrayImage downscaled_input(
		scaleToGray(trimmed_image, downscaled_size)
	);
	trimmed_image = GrayImage(); // Save memory.
	
	status.throwIfCancelled();
	
	// Light areas indicate pictures.
	GrayImage picture_areas(detectPictures(downscaled_input, status, dbg));
	downscaled_input = GrayImage(); // Save memory.
	
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

void
OutputGenerator::modifyBinarizationMask(
	imageproc::BinaryImage& bw_mask,
	QRect const& mask_rect, ZoneSet const& zones) const
{
	QTransform xform(m_xform.transform());
	xform *= QTransform().translate(-mask_rect.x(), -mask_rect.y());

	typedef PictureLayerProperty PLP;

	// Pass 1: ERASER1
	BOOST_FOREACH(Zone const& zone, zones) {
		if (zone.properties().locateOrDefault<PLP>()->layer() == PLP::ERASER1) {
			QPolygonF const poly(zone.spline().toPolygon());
			PolygonRasterizer::fill(bw_mask, BLACK, xform.map(poly), Qt::WindingFill);
		}
	}

	// Pass 2: PAINTER2
	BOOST_FOREACH(Zone const& zone, zones) {
		if (zone.properties().locateOrDefault<PLP>()->layer() == PLP::PAINTER2) {
			QPolygonF const poly(zone.spline().toPolygon());
			PolygonRasterizer::fill(bw_mask, WHITE, xform.map(poly), Qt::WindingFill);
		}
	}

	// Pass 1: ERASER3
	BOOST_FOREACH(Zone const& zone, zones) {
		if (zone.properties().locateOrDefault<PLP>()->layer() == PLP::ERASER3) {
			QPolygonF const poly(zone.spline().toPolygon());
			PolygonRasterizer::fill(bw_mask, BLACK, xform.map(poly), Qt::WindingFill);
		}
	}
}

QImage
OutputGenerator::processImpl(
	TaskStatus const& status, FilterData const& input,
//Quadro_Zoner
	//ZoneSet const& picture_zones, ZoneSet const& fill_zones,
	ZoneSet& picture_zones, ZoneSet const& fill_zones,
	DewarpingMode dewarping_mode,
	DistortionModel& distortion_model,
	DepthPerception const& depth_perception,
//begin of modified by monday2000
//Dont_Equalize_Illumination_Pic_Zones
//Original_Foreground_Mixed
//added:	
	bool dont_equalize_illumination_pic_zones,
	bool keep_orig_fore_subscan,
//end of modified by monday2000
	imageproc::BinaryImage* auto_picture_mask,
	imageproc::BinaryImage* speckles_image,
//begin of modified by monday2000
//Picture_Shape
	//DebugImages* const dbg) const
	DebugImages* const dbg,
	PictureShape picture_shape
//Quadro_Zoner
	, PageId* p_pageId,
	IntrusivePtr<Settings>* p_settings
	) const
//end of modified by monday2000
{
	RenderParams const render_params(m_colorParams);

//begin of modified by monday2000
//Original_Foreground_Mixed
//added:

	if (keep_orig_fore_subscan)
	{
		if (dewarping_mode == DewarpingMode::AUTO ||
//begin of modified by monday2000
//Marginal_Dewarping
			dewarping_mode == DewarpingMode::MARGINAL ||
//end of modified by monday2000
			(dewarping_mode == DewarpingMode::MANUAL && distortion_model.isValid())) {
				return processWithDewarping(
					status, input, picture_zones, fill_zones,
					dewarping_mode, distortion_model, depth_perception,
					dont_equalize_illumination_pic_zones,
					keep_orig_fore_subscan,
					auto_picture_mask, speckles_image, dbg
//begin of modified by monday2000
//Picture_Shape
					, picture_shape
//Quadro_Zoner
					, p_pageId,
					p_settings
//end of modified by monday2000
					);
		} else return processAsIs(
			input, status, fill_zones, depth_perception, dbg
			);
	}
//end of modified by monday2000

	if (dewarping_mode == DewarpingMode::AUTO ||
//begin of modified by monday2000
//Marginal_Dewarping
		dewarping_mode == DewarpingMode::MARGINAL ||
//end of modified by monday2000
		(dewarping_mode == DewarpingMode::MANUAL && distortion_model.isValid())) {
		return processWithDewarping(
			status, input, picture_zones, fill_zones,
			dewarping_mode, distortion_model, depth_perception,
//begin of modified by monday2000
//Dont_Equalize_Illumination_Pic_Zones
//added:			
			dont_equalize_illumination_pic_zones,
			false,
//end of modified by monday2000
			auto_picture_mask, speckles_image, dbg
//begin of modified by monday2000
//Picture_Shape
			, picture_shape
//Quadro_Zoner
			, p_pageId,
			p_settings
//end of modified by monday2000
		);
	} else if (!render_params.whiteMargins()) {
		return processAsIs(
			input, status, fill_zones, depth_perception, dbg
		);
	} else {
		return processWithoutDewarping(
			status, input, picture_zones, fill_zones,
//begin of modified by monday2000
//Dont_Equalize_Illumination_Pic_Zones
//added:			
			dont_equalize_illumination_pic_zones,
//end of modified by monday2000
			auto_picture_mask, speckles_image, dbg
//begin of modified by monday2000
//Picture_Shape
			, picture_shape
//Quadro_Zoner
			, p_pageId,
			p_settings
//end of modified by monday2000
		);
	}
}

QImage
OutputGenerator::processAsIs(
	FilterData const& input, TaskStatus const& status,
	ZoneSet const& fill_zones,
	DepthPerception const& depth_perception,
	DebugImages* const dbg) const
{
	uint8_t const dominant_gray = reserveBlackAndWhite<uint8_t>(
		calcDominantBackgroundGrayLevel(input.grayImage())
	);
	
	status.throwIfCancelled();
	
	QColor const bg_color(dominant_gray, dominant_gray, dominant_gray);
	
	QImage out;

	if (input.origImage().allGray()) {
		if (m_outRect.isEmpty()) {
			QImage image(1, 1, QImage::Format_Indexed8);
			image.setColorTable(createGrayscalePalette());
			if (image.isNull()) {
				throw std::bad_alloc();
			}
			image.fill(dominant_gray);
			return image;
		}

		out = transformToGray(
			input.grayImage(), m_xform.transform(), m_outRect,
			OutsidePixels::assumeColor(bg_color)
		);
	} else {
		if (m_outRect.isEmpty()) {
			QImage image(1, 1, QImage::Format_RGB32);
			image.fill(bg_color.rgb());
			return image;
		}
		
		out = transform(
			input.origImage(), m_xform.transform(), m_outRect,
			OutsidePixels::assumeColor(bg_color)
		);
	}

	applyFillZonesInPlace(out, fill_zones);
	reserveBlackAndWhite(out);

	return out;
}

QImage
OutputGenerator::processWithoutDewarping(
	TaskStatus const& status, FilterData const& input,
//Quadro_Zoner
	//ZoneSet const& picture_zones, ZoneSet const& fill_zones,
	ZoneSet& picture_zones, ZoneSet const& fill_zones,
//begin of modified by monday2000
//Dont_Equalize_Illumination_Pic_Zones
//added:	
	bool dont_equalize_illumination_pic_zones,
//end of modified by monday2000
	imageproc::BinaryImage* auto_picture_mask,
	imageproc::BinaryImage* speckles_image,
//begin of modified by monday2000
//Picture_Shape
	//DebugImages* dbg) const
	DebugImages* dbg,
	PictureShape picture_shape
//Quadro_Zoner
	, PageId* p_pageId,
	IntrusivePtr<Settings>* p_settings
	) const
//end of modified by monday2000
{
	RenderParams const render_params(m_colorParams);
	
	// The whole image minus the part cut off by the split line.
	QRect const big_margins_rect(
		m_xform.resultingPreCropArea().boundingRect().toRect() | m_contentRect
	);
	
	// For various reasons, we need some whitespace around the content
	// area.  This is the number of pixels of such whitespace.	
//begin of modified by monday2000
//Marginal_Dewarping
	//int const content_margin = m_dpi.vertical() * 20 / 300;
	int const content_margin = 40;
//end of modified by monday2000
	
	// The content area (in output image coordinates) extended
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
	
	// Crop area in original image coordinates.
	QPolygonF const orig_image_crop_area(
		m_xform.transformBack().map(
			m_xform.resultingPreCropArea()
		)
	);
	
	// Crop area in maybe_normalized image coordinates.	
	QPolygonF normalize_illumination_crop_area(m_xform.resultingPreCropArea());
	normalize_illumination_crop_area.translate(-normalize_illumination_rect.topLeft());

	if (render_params.normalizeIllumination()) {
		maybe_normalized = normalizeIlluminationGray(
			status, input.grayImage(), orig_image_crop_area,
			m_xform.transform(), normalize_illumination_rect, 0, dbg
		);
	} else {
		maybe_normalized = transform(
			input.origImage(), m_xform.transform(),
			normalize_illumination_rect, OutsidePixels::assumeColor(Qt::white)
		);
	}

//begin of modified by monday2000
//Dont_Equalize_Illumination_Pic_Zones
//added:
        QImage maybe_normalized_Dont_Equalize_Illumination_Pic_Zones;

		bool const color_original = !input.origImage().allGray();

        if (!color_original)

                maybe_normalized_Dont_Equalize_Illumination_Pic_Zones = transformToGray(
                input.grayImage(), m_xform.transform(),
                normalize_illumination_rect, OutsidePixels::assumeColor(Qt::white)
                );
        else
                maybe_normalized_Dont_Equalize_Illumination_Pic_Zones = transform(
                input.origImage(), m_xform.transform(),
                normalize_illumination_rect, OutsidePixels::assumeColor(Qt::white)
                );
//end of modified by monday2000

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
	
	if (render_params.binaryOutput() || m_outRect.isEmpty()) {
		BinaryImage dst(m_outRect.size().expandedTo(QSize(1, 1)), WHITE);
		
		if (!m_contentRect.isEmpty()) {
			BinaryImage bw_content(
				binarize(maybe_smoothed, normalize_illumination_crop_area)
			);
			if (dbg) {
				dbg->add(bw_content, "binarized_and_cropped");
			}
			
			status.throwIfCancelled();
			
			morphologicalSmoothInPlace(bw_content, status);
			if (dbg) {
				dbg->add(bw_content, "edges_smoothed");
			}

			status.throwIfCancelled();
			
			QRect const src_rect(m_contentRect.translated(-normalize_illumination_rect.topLeft()));
			QRect const dst_rect(m_contentRect);
			rasterOp<RopSrc>(dst, dst_rect, bw_content, src_rect.topLeft());
			bw_content.release(); // Save memory.
			
			// It's important to keep despeckling the very last operation
			// affecting the binary part of the output. That's because
			// we will be reconstructing the input to this despeckling
			// operation from the final output file.
			maybeDespeckleInPlace(
				dst, m_outRect, m_outRect, m_despeckleLevel,
				speckles_image, m_dpi, status, dbg
			);
		}
		
		applyFillZonesInPlace(dst, fill_zones);
		return dst.toQImage();
	}
	
	QSize const target_size(m_outRect.size().expandedTo(QSize(1, 1)));

	BinaryImage bw_mask;
	if (render_params.mixedOutput()) {
		// This block should go before the block with
		// adjustBrightnessGrayscale(), which may convert
		// maybe_normalized from grayscale to color mode.
		
		bw_mask = estimateBinarizationMask(
			status, GrayImage(maybe_normalized),
			normalize_illumination_rect,
			small_margins_rect, dbg
		);
//begin of modified by monday2000
//Picture_Shape
//Quadro_Zoner
		if (picture_shape == RECTANGULAR_SHAPE)
		{
			bw_mask.rectangularizeAreas(WHITE);

			picture_zones.remove_auto_zones();

			(*p_settings)->setPictureZones(*p_pageId, picture_zones);
		}		
		else if (picture_shape == QUADRO_SHAPE)
		{
			if (picture_zones.auto_zones_found())
				bw_mask.fill(BLACK);
			else
			{
				std::vector<QRect> areas;
				bw_mask.rectangularizeAreasQuadro(WHITE, areas);				

				QTransform xform1(m_xform.transform());            
				xform1 *= QTransform().translate(-small_margins_rect.x(), -small_margins_rect.y());

				QTransform inv_xform(xform1.inverted()); 

				for (int i=0; i<(int)areas.size(); i++)
				{				
					QRectF area0(areas[i]);
					QPolygonF area1(area0);
					QPolygonF area(inv_xform.map(area1));

					Zone zone1(area);

					picture_zones.add(zone1);
				}

				(*p_settings)->setPictureZones(*p_pageId, picture_zones);
			}
		}
		else
		{
			picture_zones.remove_auto_zones();

			(*p_settings)->setPictureZones(*p_pageId, picture_zones);
		}
//end of modified by monday2000

		if (dbg) {
			dbg->add(bw_mask, "bw_mask");
		}
		
		if (auto_picture_mask) {
			if (auto_picture_mask->size() != target_size) {
				BinaryImage(target_size).swap(*auto_picture_mask);
			}
			auto_picture_mask->fill(BLACK);

			if (!m_contentRect.isEmpty()) {
				QRect const src_rect(m_contentRect.translated(-small_margins_rect.topLeft()));
				QRect const dst_rect(m_contentRect);
				rasterOp<RopSrc>(*auto_picture_mask, dst_rect, bw_mask, src_rect.topLeft());
			}
		}

		status.throwIfCancelled();

		modifyBinarizationMask(bw_mask, small_margins_rect, picture_zones);
		if (dbg) {
			dbg->add(bw_mask, "bw_mask with zones");
		}
	}
	
	if (render_params.normalizeIllumination()
			&& !input.origImage().allGray()) {
		assert(maybe_normalized.format() == QImage::Format_Indexed8);
		QImage tmp(
			transform(
				input.origImage(), m_xform.transform(),
				normalize_illumination_rect,
				OutsidePixels::assumeColor(Qt::white)
			)
		);
		
		status.throwIfCancelled();
		
		adjustBrightnessGrayscale(tmp, maybe_normalized);
		maybe_normalized = tmp;
		if (dbg) {
			dbg->add(maybe_normalized, "norm_illum_color");
		}
	}
	
	if (!render_params.mixedOutput()) {
		// It's "Color / Grayscale" mode, as we handle B/W above.
		reserveBlackAndWhite(maybe_normalized);
	} else {
		BinaryImage bw_content(
			binarize(maybe_smoothed, normalize_illumination_crop_area, &bw_mask)
		);
		maybe_smoothed = QImage(); // Save memory.
		if (dbg) {
			dbg->add(bw_content, "binarized_and_cropped");
		}
		
		status.throwIfCancelled();
		
		morphologicalSmoothInPlace(bw_content, status);
		if (dbg) {
			dbg->add(bw_content, "edges_smoothed");
		}

		status.throwIfCancelled();
		
		// We don't want speckles in non-B/W areas, as they would
		// then get visualized on the Despeckling tab.
		rasterOp<RopAnd<RopSrc, RopDst> >(bw_content, bw_mask);

		status.throwIfCancelled();

		// It's important to keep despeckling the very last operation
		// affecting the binary part of the output. That's because
		// we will be reconstructing the input to this despeckling
		// operation from the final output file.
		maybeDespeckleInPlace(
			bw_content, small_margins_rect, m_contentRect,
			m_despeckleLevel, speckles_image, m_dpi, status, dbg
		);
		
		status.throwIfCancelled();
		
		if (maybe_normalized.format() == QImage::Format_Indexed8) {
			combineMixed<uint8_t>(
				maybe_normalized, bw_content, bw_mask,
//begin of modified by monday2000
//Dont_Equalize_Illumination_Pic_Zones
//added:
				maybe_normalized_Dont_Equalize_Illumination_Pic_Zones,
				dont_equalize_illumination_pic_zones
//end of modified by monday2000
			);
		} else {
			assert(maybe_normalized.format() == QImage::Format_RGB32
				|| maybe_normalized.format() == QImage::Format_ARGB32);

			combineMixed<uint32_t>(
				maybe_normalized, bw_content, bw_mask,
//begin of modified by monday2000
//Dont_Equalize_Illumination_Pic_Zones
//added:
				maybe_normalized_Dont_Equalize_Illumination_Pic_Zones,				
				dont_equalize_illumination_pic_zones
//end of modified by monday2000
			);
		}
	}
	
	status.throwIfCancelled();
	
	assert(!target_size.isEmpty());
	QImage dst(target_size, maybe_normalized.format());

	if (maybe_normalized.format() == QImage::Format_Indexed8) {
		dst.setColorTable(createGrayscalePalette());
		// White.  0xff is reserved if in "Color / Grayscale" mode.
		uint8_t const color = render_params.mixedOutput() ? 0xff : 0xfe;
		dst.fill(color);
	} else {
		// White.  0x[ff]ffffff is reserved if in "Color / Grayscale" mode.
		uint32_t const color = render_params.mixedOutput() ? 0xffffffff : 0xfffefefe;
		dst.fill(color);
	}

	if (dst.isNull()) {
		// Both the constructor and setColorTable() above can leave the image null.
		throw std::bad_alloc();
	}

	if (!m_contentRect.isEmpty()) {
		QRect const src_rect(m_contentRect.translated(-small_margins_rect.topLeft()));
		QRect const dst_rect(m_contentRect);
		drawOver(dst, dst_rect, maybe_normalized, src_rect);
	}
	
	applyFillZonesInPlace(dst, fill_zones);
	return dst;
}

QImage
OutputGenerator::processWithDewarping(
	TaskStatus const& status, FilterData const& input,
//Quadro_Zoner
	//ZoneSet const& picture_zones, ZoneSet const& fill_zones,
	ZoneSet& picture_zones, ZoneSet const& fill_zones,
	DewarpingMode dewarping_mode,
	DistortionModel& distortion_model,
	DepthPerception const& depth_perception,
//begin of modified by monday2000
//Dont_Equalize_Illumination_Pic_Zones
//Original_Foreground_Mixed
//added:	
	bool dont_equalize_illumination_pic_zones,
	bool keep_orig_fore_subscan,
//end of modified by monday2000
	imageproc::BinaryImage* auto_picture_mask,
	imageproc::BinaryImage* speckles_image,
//begin of modified by monday2000
//Picture_Shape
	//DebugImages* dbg) const
	DebugImages* dbg,
	PictureShape picture_shape
//Quadro_Zoner
	, PageId* p_pageId,
	IntrusivePtr<Settings>* p_settings
	) const
//end of modified by monday2000
{
	QSize const target_size(m_outRect.size().expandedTo(QSize(1, 1)));
	if (m_outRect.isEmpty()) {
		return BinaryImage(target_size, WHITE).toQImage();
	}

	RenderParams const render_params(m_colorParams);
	
	// The whole image minus the part cut off by the split line.
	QRect const big_margins_rect(
		m_xform.resultingPreCropArea().boundingRect().toRect() | m_contentRect
	);
	
	// For various reasons, we need some whitespace around the content
	// area.  This is the number of pixels of such whitespace.	
//begin of modified by monday2000
//Marginal_Dewarping
	//int const content_margin = m_dpi.vertical() * 20 / 300;
	int const content_margin = 40;
//end of modified by monday2000
	
	// The content area (in output image coordinates) extended
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
	
	// Crop area in original image coordinates.
	QPolygonF const orig_image_crop_area(
		m_xform.transformBack().map(m_xform.resultingPreCropArea())
	);
	
	// Crop area in maybe_normalized image coordinates.
	QPolygonF normalize_illumination_crop_area(m_xform.resultingPreCropArea());
	normalize_illumination_crop_area.translate(-normalize_illumination_rect.topLeft());
	
	bool const color_original = !input.origImage().allGray();

	// Original image, but:
	// 1. In a format we can handle, that is grayscale, RGB32, ARGB32
	// 2. With illumination normalized over the content area, if required.
	// 3. With margins filled with white, if required.
	QImage normalized_original;

	// The output we would get if dewarping was turned off, except always grayscale.
	// Used for automatic picture detection and binarization threshold calculation.
	// This image corresponds to the area of normalize_illumination_rect above.
	GrayImage warped_gray_output;

	// Picture mask (white indicate a picture) in the same coordinates as
	// warped_gray_output.  Only built for Mixed mode.
	BinaryImage warped_bw_mask;

	BinaryThreshold bw_threshold(128);

	QTransform const norm_illum_to_original(
		QTransform().translate(
			normalize_illumination_rect.left(),
			normalize_illumination_rect.top()
		) * m_xform.transformBack()
	);

//begin of modified by monday2000
//Dont_Equalize_Illumination_Pic_Zones
//added:	
	QImage normalized_original_Dont_Equalize_Illumination_Pic_Zones;

		if (color_original) {
			normalized_original_Dont_Equalize_Illumination_Pic_Zones
				= convertToRGBorRGBA(input.origImage());
		} else {
			normalized_original_Dont_Equalize_Illumination_Pic_Zones = input.grayImage();
		}
//end of modified by monday2000

	if (!render_params.normalizeIllumination()) {
		if (color_original) {
			normalized_original = convertToRGBorRGBA(input.origImage());
		} else {
			normalized_original = input.grayImage();
		}
		if (dewarping_mode == DewarpingMode::AUTO
//begin of modified by monday2000
//Marginal_Dewarping
			|| dewarping_mode == DewarpingMode::MARGINAL
//end of modified by monday2000			
			) {
			warped_gray_output = transformToGray(
				input.grayImage(), m_xform.transform(), normalize_illumination_rect,
				OutsidePixels::assumeWeakColor(Qt::white)
			);
		} // Otherwise we just don't need it.
	} else {
		GrayImage warped_gray_background;
		warped_gray_output = normalizeIlluminationGray(
			status, input.grayImage(), orig_image_crop_area,
			m_xform.transform(), normalize_illumination_rect,
			&warped_gray_background, dbg
		);

		status.throwIfCancelled();
		
		// Transform warped_gray_background to original image coordinates.
		warped_gray_background = transformToGray(
			warped_gray_background.toQImage(), norm_illum_to_original,
			input.origImage().rect(), OutsidePixels::assumeWeakColor(Qt::black)
		);
		if (dbg) {
			dbg->add(warped_gray_background, "orig_background");
		}

		status.throwIfCancelled();

		// Turn background into a grayscale, illumination-normalized image.
		grayRasterOp<RaiseAboveBackground>(warped_gray_background, input.grayImage());
		if (dbg) {
			dbg->add(warped_gray_background, "norm_illum_gray");
		}

		status.throwIfCancelled();

		if (!color_original || render_params.binaryOutput()) {
			normalized_original = warped_gray_background;
		} else {
			normalized_original = convertToRGBorRGBA(input.origImage());
			adjustBrightnessGrayscale(normalized_original, warped_gray_background);
			if (dbg) {
				dbg->add(normalized_original, "norm_illum_color");
			}
		}
	}

	status.throwIfCancelled();

	if (render_params.binaryOutput()) {
		bw_threshold = calcBinarizationThreshold(
			warped_gray_output, normalize_illumination_crop_area
		);

		status.throwIfCancelled();

	} else if (render_params.mixedOutput()) {

		estimateBinarizationMask(
			status, GrayImage(warped_gray_output),
			normalize_illumination_rect,
			small_margins_rect, dbg
		).swap(warped_bw_mask);
		if (dbg) {
			dbg->add(warped_bw_mask, "warped_bw_mask");
		}

//begin of modified by monday2000
//Picture_Shape
//Quadro_Zoner
		if (picture_shape == RECTANGULAR_SHAPE)
		{
			warped_bw_mask.rectangularizeAreas(WHITE);

			picture_zones.remove_auto_zones();

			(*p_settings)->setPictureZones(*p_pageId, picture_zones);
		}		
		else if (picture_shape == QUADRO_SHAPE)
		{
			if (picture_zones.auto_zones_found())
				warped_bw_mask.fill(BLACK);
			else
			{
				std::vector<QRect> areas;
				warped_bw_mask.rectangularizeAreasQuadro(WHITE, areas);				

				QTransform xform1(m_xform.transform());            
				xform1 *= QTransform().translate(-small_margins_rect.x(), -small_margins_rect.y());

				QTransform inv_xform(xform1.inverted()); 

				for (int i=0; i<(int)areas.size(); i++)
				{				
					QRectF area0(areas[i]);
					QPolygonF area1(area0);
					QPolygonF area(inv_xform.map(area1));

					Zone zone1(area);

					picture_zones.add(zone1);
				}			

				(*p_settings)->setPictureZones(*p_pageId, picture_zones);
			}
		}
		else
		{
			picture_zones.remove_auto_zones();

			(*p_settings)->setPictureZones(*p_pageId, picture_zones);
		}
//end of modified by monday2000

		status.throwIfCancelled();

		if (auto_picture_mask) {
			if (auto_picture_mask->size() != target_size) {
				BinaryImage(target_size).swap(*auto_picture_mask);
			}
			auto_picture_mask->fill(BLACK);

			if (!m_contentRect.isEmpty()) {
				QRect const src_rect(m_contentRect.translated(-small_margins_rect.topLeft()));
				QRect const dst_rect(m_contentRect);
				rasterOp<RopSrc>(*auto_picture_mask, dst_rect, warped_bw_mask, src_rect.topLeft());
			}
		}

		status.throwIfCancelled();

		modifyBinarizationMask(warped_bw_mask, small_margins_rect, picture_zones);
		if (dbg) {
			dbg->add(warped_bw_mask, "warped_bw_mask with zones");
		}

		status.throwIfCancelled();

		// For Mixed output, we mask out pictures when calculating binarization threshold.
		bw_threshold = calcBinarizationThreshold(
			warped_gray_output, normalize_illumination_crop_area, &warped_bw_mask
		);
		
		status.throwIfCancelled();
	}

	if (dewarping_mode == DewarpingMode::AUTO) {
		DistortionModelBuilder model_builder(Vec2d(0, 1));

		QRect const content_rect(
			m_contentRect.translated(-normalize_illumination_rect.topLeft())
		);
		TextLineTracer::trace(
			warped_gray_output, m_dpi, content_rect, model_builder, status, dbg
		);
		model_builder.transform(norm_illum_to_original);

		TopBottomEdgeTracer::trace(
			input.grayImage(), model_builder.verticalBounds(),
			model_builder, status, dbg
		);
		
		distortion_model = model_builder.tryBuildModel(dbg, &input.grayImage().toQImage());

//begin of modified by monday2000
//Auto_Dewarping_Vert_Half_Correction

		BinaryThreshold bw_threshold(64);	
		BinaryImage bw_image(input.grayImage(), bw_threshold);

		QTransform transform = m_xform.preRotation().transform(bw_image.size());
		QTransform inv_transform = transform.inverted();

		int degrees = m_xform.preRotation().toDegrees();
		bw_image = orthogonalRotation(bw_image, degrees);

		std::vector<QPointF> const& top_polyline0 = distortion_model.topCurve().polyline();
		std::vector<QPointF> const& bottom_polyline0 = distortion_model.bottomCurve().polyline();

		std::vector<QPointF> top_polyline;
		std::vector<QPointF> bottom_polyline;

		for (int i=0; i<(int)top_polyline0.size(); i++)
			top_polyline.push_back(transform.map(top_polyline0[i]));

		for (int i=0; i<(int)bottom_polyline0.size(); i++)
			bottom_polyline.push_back(transform.map(bottom_polyline0[i]));

		//QImage out_image(bw_image.toQImage().convertToFormat(QImage::Format_RGB32));
		//for (int i=0; i<(int)top_polyline.size(); i++) drawPoint(out_image, top_polyline[i]);
		//for (int i=0; i<(int)bottom_polyline.size(); i++) drawPoint(out_image, bottom_polyline[i]);
		//TiffWriter::writeImage("C:\\bw_dewarp.tif", out_image);

		PageId const& pageId = *p_pageId;

		QString stAngle;

		float max_angle = 2.75; // chosen empirically

		//QFile file("C:\\st_angle.txt");
		//file.open(QIODevice::WriteOnly | QIODevice::Text);
		//QTextStream out(&file);
		//out << "degrees = " << QString::number(degrees) << endl;

		if (pageId.subPage() == PageId::SINGLE_PAGE || pageId.subPage() == PageId::LEFT_PAGE)
		{
			float vert_skew_angle_left = vert_border_skew_angle(top_polyline.front(), bottom_polyline.front());

			stAngle.setNum(vert_skew_angle_left);

			//out << "vert_skew_angle_left = " << stAngle << endl;

			if (vert_skew_angle_left > max_angle)
			{
				//out << "vert_skew_angle_left correction" << endl;

				float top_x = top_polyline.front().x();
				float bottom_x = bottom_polyline.front().x();

				if (top_x < bottom_x)
				{
					std::vector<QPointF> new_bottom_polyline;

					QPointF pt(top_x, bottom_polyline.front().y());

					new_bottom_polyline.push_back(pt);

					for (int i=0; i<(int)bottom_polyline.size(); i++)
						new_bottom_polyline.push_back(inv_transform.map(bottom_polyline[i]));

					distortion_model.setBottomCurve(dewarping::Curve(new_bottom_polyline));
				}
				else
				{
					std::vector<QPointF> new_top_polyline;

					QPointF pt(bottom_x, top_polyline.front().y());

					new_top_polyline.push_back(pt);

					for (int i=0; i<(int)top_polyline.size(); i++)
						new_top_polyline.push_back(inv_transform.map(top_polyline[i]));

					distortion_model.setBottomCurve(dewarping::Curve(new_top_polyline));
				}
			}
		}
		else
		{
			float vert_skew_angle_right = vert_border_skew_angle(top_polyline.back(), bottom_polyline.back());

			stAngle.setNum(vert_skew_angle_right);
			
			//out << "vert_skew_angle_right = " << stAngle << endl;

			if (vert_skew_angle_right > max_angle)
			{				
				//out << "vert_skew_angle_right correction" << endl;

				float top_x = top_polyline.back().x();
				float bottom_x = bottom_polyline.back().x();

				if (top_x > bottom_x)
				{
					std::vector<QPointF> new_bottom_polyline;

					QPointF pt(top_x, bottom_polyline.back().y());

					for (int i=0; i<(int)bottom_polyline.size(); i++)
						new_bottom_polyline.push_back(inv_transform.map(bottom_polyline[i]));

					new_bottom_polyline.push_back(pt);

					distortion_model.setBottomCurve(dewarping::Curve(new_bottom_polyline));
				}
				else
				{
					std::vector<QPointF> new_top_polyline;

					QPointF pt(bottom_x, top_polyline.back().y());					

					for (int i=0; i<(int)top_polyline.size(); i++)
						new_top_polyline.push_back(inv_transform.map(top_polyline[i]));

					new_top_polyline.push_back(pt);

					distortion_model.setBottomCurve(dewarping::Curve(new_top_polyline));
				}			
			}	
		}

		//file.close();

//end of modified by monday2000

		if (!distortion_model.isValid()) {
			setupTrivialDistortionModel(distortion_model);
		}
//begin of modified by monday2000
//Marginal_Dewarping
	}
	else if (dewarping_mode == DewarpingMode::MARGINAL)
	{
		BinaryThreshold bw_threshold(64);	
		BinaryImage bw_image(input.grayImage(), bw_threshold);

		QTransform transform = m_xform.preRotation().transform(bw_image.size());
		QTransform inv_transform = transform.inverted();

		int degrees = m_xform.preRotation().toDegrees();
		bw_image = orthogonalRotation(bw_image, degrees);

		setupTrivialDistortionModel(distortion_model);

		PageId const& pageId = *p_pageId;		

		int max_red_points = 5; //the more the curling the more this value
		
		XSpline top_spline;

		std::vector<QPointF> const& top_polyline = distortion_model.topCurve().polyline();

		QLineF const top_line(transform.map(top_polyline.front()), transform.map(top_polyline.back()));

		top_spline.appendControlPoint(top_line.p1(), 0);

		if (pageId.subPage() == PageId::SINGLE_PAGE || pageId.subPage() == PageId::LEFT_PAGE)
		{
			for (int i=29-max_red_points; i<29; i++)
				top_spline.appendControlPoint(top_line.pointAt((float)i/29.0), 1);
		}
		else
		{
			for (int i=1; i<=max_red_points; i++)
				top_spline.appendControlPoint(top_line.pointAt((float)i/29.0), 1);
		}		

		top_spline.appendControlPoint(top_line.p2(), 0);

		for (int i=0; i<=top_spline.numSegments(); i++) movePointToTopMargin(bw_image, top_spline, i);

		for (int i=0; i<=top_spline.numSegments(); i++)
			top_spline.moveControlPoint(i,inv_transform.map(top_spline.controlPointPosition(i)));

		distortion_model.setTopCurve(dewarping::Curve(top_spline));

//bottom:

		XSpline bottom_spline;

		std::vector<QPointF> const& bottom_polyline = distortion_model.bottomCurve().polyline();

		QLineF const bottom_line(transform.map(bottom_polyline.front()), transform.map(bottom_polyline.back()));

		bottom_spline.appendControlPoint(bottom_line.p1(), 0);

		if (pageId.subPage() == PageId::SINGLE_PAGE || pageId.subPage() == PageId::LEFT_PAGE)
		{
			for (int i=29-max_red_points; i<29; i++)
				bottom_spline.appendControlPoint(top_line.pointAt((float)i/29.0), 1);
		}
		else
		{
			for (int i=1; i<=max_red_points; i++)
				bottom_spline.appendControlPoint(top_line.pointAt((float)i/29.0), 1);
		}

		bottom_spline.appendControlPoint(bottom_line.p2(), 0);

		for (int i=0; i<=bottom_spline.numSegments(); i++) movePointToBottomMargin(bw_image, bottom_spline, i);

		for (int i=0; i<=bottom_spline.numSegments(); i++)
			bottom_spline.moveControlPoint(i,inv_transform.map(bottom_spline.controlPointPosition(i)));

		distortion_model.setBottomCurve(dewarping::Curve(bottom_spline));

		if (!distortion_model.isValid()) {		
			setupTrivialDistortionModel(distortion_model);
		}

		if (dbg) {
			QImage out_image(bw_image.toQImage().convertToFormat(QImage::Format_RGB32));
			for (int i=0; i<=top_spline.numSegments(); i++) drawPoint(out_image, top_spline.controlPointPosition(i));
			for (int i=0; i<=bottom_spline.numSegments(); i++) drawPoint(out_image, bottom_spline.controlPointPosition(i));
			dbg->add(out_image, "marginal dewarping");
		}
//end of modified by monday2000
	}

	warped_gray_output = GrayImage(); // Save memory.

	if (render_params.whiteMargins()) {
		// Fill everything except the content area in normalized_original to white.
		QPolygonF const orig_content_poly(m_xform.transformBack().map(QRectF(m_contentRect)));
		fillMarginsInPlace(normalized_original, orig_content_poly, Qt::white);
		if (dbg) {
			dbg->add(normalized_original, "white margins");
		}
	}
	
	status.throwIfCancelled();

	QColor bg_color(Qt::white);
	if (!render_params.whiteMargins()) {
		uint8_t const dominant_gray = reserveBlackAndWhite<uint8_t>(
			calcDominantBackgroundGrayLevel(input.grayImage())
		);
		bg_color = QColor(dominant_gray, dominant_gray, dominant_gray);
	}

//begin of modified by monday2000
//Dont_Equalize_Illumination_Pic_Zones
//added:

	QImage dewarped_Dont_Equalize_Illumination_Pic_Zones;
	try {
		dewarped_Dont_Equalize_Illumination_Pic_Zones = dewarp(
			QTransform(), normalized_original_Dont_Equalize_Illumination_Pic_Zones,
			m_xform.transform(),
			distortion_model, depth_perception, bg_color
		);
	} catch (std::runtime_error const&) {
		// Probably an impossible distortion model.  Let's fall back to a trivial one.
		setupTrivialDistortionModel(distortion_model);
		dewarped_Dont_Equalize_Illumination_Pic_Zones = dewarp(
			QTransform(), normalized_original_Dont_Equalize_Illumination_Pic_Zones,
			m_xform.transform(),
			distortion_model, depth_perception, bg_color
		);
	}
	normalized_original_Dont_Equalize_Illumination_Pic_Zones = QImage(); // Save memory.

//Original_Foreground_Mixed
	if (keep_orig_fore_subscan)
	{

	boost::shared_ptr<DewarpingPointMapper> mapper(
		new DewarpingPointMapper(
			distortion_model, depth_perception.value(),
			m_xform.transform(), m_contentRect
		)
	);
	boost::function<QPointF(QPointF const&)> const orig_to_output(
		boost::bind(&DewarpingPointMapper::mapToDewarpedSpace, mapper, _1)
	);

		applyFillZonesInPlace(dewarped_Dont_Equalize_Illumination_Pic_Zones, fill_zones, orig_to_output);
//Marginal_Dewarping
		maybe_deskew(&dewarped_Dont_Equalize_Illumination_Pic_Zones, dewarping_mode);
		return dewarped_Dont_Equalize_Illumination_Pic_Zones;
	}
//end of modified by monday2000

	QImage dewarped;
	try {
		dewarped = dewarp(
			QTransform(), normalized_original, m_xform.transform(),
			distortion_model, depth_perception, bg_color
		);
	} catch (std::runtime_error const&) {
		// Probably an impossible distortion model.  Let's fall back to a trivial one.
		setupTrivialDistortionModel(distortion_model);
		dewarped = dewarp(
			QTransform(), normalized_original, m_xform.transform(),
			distortion_model, depth_perception, bg_color
		);
	}
	normalized_original = QImage(); // Save memory.
	if (dbg) {
		dbg->add(dewarped, "dewarped");
	}

	status.throwIfCancelled();

	QImage dewarped_and_maybe_smoothed;
	// We only do smoothing if we are going to do binarization later.
	if (!render_params.needBinarization()) {
		dewarped_and_maybe_smoothed = dewarped;
	} else {
		dewarped_and_maybe_smoothed = smoothToGrayscale(dewarped, m_dpi);
		if (dbg) {
			dbg->add(dewarped_and_maybe_smoothed, "smoothed");
		}
	}

	boost::shared_ptr<DewarpingPointMapper> mapper(
		new DewarpingPointMapper(
			distortion_model, depth_perception.value(),
			m_xform.transform(), m_contentRect
		)
	);
	boost::function<QPointF(QPointF const&)> const orig_to_output(
		boost::bind(&DewarpingPointMapper::mapToDewarpedSpace, mapper, _1)
	);

	if (render_params.binaryOutput()) {	
		BinaryImage dewarped_bw_content(dewarped_and_maybe_smoothed, bw_threshold);
		dewarped_and_maybe_smoothed = QImage(); // Save memory.
		if (dbg) {
			dbg->add(dewarped_bw_content, "dewarped_bw_content");
		}
		
		status.throwIfCancelled();

		morphologicalSmoothInPlace(dewarped_bw_content, status);
		if (dbg) {
			dbg->add(dewarped_bw_content, "edges_smoothed");
		}

		status.throwIfCancelled();
		
		// It's important to keep despeckling the very last operation
		// affecting the binary part of the output. That's because
		// we will be reconstructing the input to this despeckling
		// operation from the final output file.
		maybeDespeckleInPlace(
			dewarped_bw_content, m_outRect, m_outRect, m_despeckleLevel,
			speckles_image, m_dpi, status, dbg
		);

		applyFillZonesInPlace(dewarped_bw_content, fill_zones, orig_to_output);
//begin of modified by monday2000
		QImage tmp_image(dewarped_bw_content.toQImage()); 
		maybe_deskew(&tmp_image, dewarping_mode);		
		return tmp_image;		
//end of modified by monday2000
	}

	if (!render_params.mixedOutput()) {
		// It's "Color / Grayscale" mode, as we handle B/W above.
		reserveBlackAndWhite(dewarped);
	} else {
		status.throwIfCancelled();

		// Dewarp the B/W mask.
		QTransform const orig_to_small_margins(
			m_xform.transform() * QTransform().translate(
				-small_margins_rect.left(),
				-small_margins_rect.top()
			)
		);
		QTransform small_margins_to_output;
		small_margins_to_output.translate(
			small_margins_rect.left(), small_margins_rect.top()
		);
		BinaryImage const dewarped_bw_mask(
			dewarp(
				orig_to_small_margins, warped_bw_mask.toQImage(),
				small_margins_to_output, distortion_model,
				depth_perception, Qt::black
			)
		);
		if (dbg) {
			dbg->add(dewarped_bw_mask, "dewarped_bw_mask");
		}

		status.throwIfCancelled();

		BinaryImage dewarped_bw_content(dewarped_and_maybe_smoothed, bw_threshold);
		dewarped_and_maybe_smoothed = QImage(); // Save memory.
		if (dbg) {
			dbg->add(dewarped_bw_content, "dewarped_bw_content");
		}

		status.throwIfCancelled();
		
		morphologicalSmoothInPlace(dewarped_bw_content, status);
		if (dbg) {
			dbg->add(dewarped_bw_content, "edges_smoothed");
		}

		status.throwIfCancelled();
		
		// We don't want speckles in non-B/W areas, as they would
		// then get visualized on the Despeckling tab.
		rasterOp<RopAnd<RopSrc, RopDst> >(dewarped_bw_content, dewarped_bw_mask);

		status.throwIfCancelled();

		// It's important to keep despeckling the very last operation
		// affecting the binary part of the output. That's because
		// we will be reconstructing the input to this despeckling
		// operation from the final output file.
		maybeDespeckleInPlace(
			dewarped_bw_content, m_outRect, m_contentRect,
			m_despeckleLevel, speckles_image, m_dpi, status, dbg
		);
		
		status.throwIfCancelled();
		
		if (dewarped.format() == QImage::Format_Indexed8) {
			combineMixed<uint8_t>(
//begin of modified by monday2000
//Dont_Equalize_Illumination_Pic_Zones
				//dewarped, dewarped_bw_content, dewarped_bw_mask
				dewarped, dewarped_bw_content, dewarped_bw_mask,
				dewarped_Dont_Equalize_Illumination_Pic_Zones,
				dont_equalize_illumination_pic_zones
//end of modified by monday2000
			);
		} else {
			assert(dewarped.format() == QImage::Format_RGB32
				|| dewarped.format() == QImage::Format_ARGB32);
			
			combineMixed<uint32_t>(
//begin of modified by monday2000
//Dont_Equalize_Illumination_Pic_Zones
				//dewarped, dewarped_bw_content, dewarped_bw_mask
				dewarped, dewarped_bw_content, dewarped_bw_mask,
				dewarped_Dont_Equalize_Illumination_Pic_Zones,
				dont_equalize_illumination_pic_zones
//end of modified by monday2000
			);
		}
	}

//begin of modified by monday2000
//Dont_Equalize_Illumination_Pic_Zones
//added:
	dewarped_Dont_Equalize_Illumination_Pic_Zones = QImage(); // Save memory.
//end of modified by monday2000

	applyFillZonesInPlace(dewarped, fill_zones, orig_to_output);
//begin of modified by monday2000
//Marginal_Dewarping	
	maybe_deskew(&dewarped, dewarping_mode);
	return dewarped;
//end of modified by monday2000
}

/**
 * Set up a distortion model corresponding to the content rect,
 * which will result in no distortion correction.
 */
void
OutputGenerator::setupTrivialDistortionModel(DistortionModel& distortion_model) const
{
	QPolygonF poly;
	if (!m_contentRect.isEmpty()) {
		poly = QRectF(m_contentRect);
	} else {
		poly << m_contentRect.topLeft() + QPointF(-0.5, -0.5);
		poly << m_contentRect.topLeft() + QPointF(0.5, -0.5);
		poly << m_contentRect.topLeft() + QPointF(0.5, 0.5);
		poly << m_contentRect.topLeft() + QPointF(-0.5, 0.5);
	}
	poly = m_xform.transformBack().map(poly);
	
	std::vector<QPointF> top_polyline, bottom_polyline;
	top_polyline.push_back(poly[0]); // top-left
	top_polyline.push_back(poly[1]); // top-right
	bottom_polyline.push_back(poly[3]); // bottom-left
	bottom_polyline.push_back(poly[2]); // bottom-right
	distortion_model.setTopCurve(Curve(top_polyline));
	distortion_model.setBottomCurve(Curve(bottom_polyline));
}

CylindricalSurfaceDewarper
OutputGenerator::createDewarper(
	DistortionModel const& distortion_model,
	QTransform const& distortion_model_to_target, double depth_perception)
{
	if (distortion_model_to_target.isIdentity()) {
		return CylindricalSurfaceDewarper(
			distortion_model.topCurve().polyline(),
			distortion_model.bottomCurve().polyline(), depth_perception
		);
	}

	std::vector<QPointF> top_polyline(distortion_model.topCurve().polyline());
	std::vector<QPointF> bottom_polyline(distortion_model.bottomCurve().polyline());
	BOOST_FOREACH(QPointF& pt, top_polyline) {
		pt = distortion_model_to_target.map(pt);
	}
	BOOST_FOREACH(QPointF& pt, bottom_polyline) {
		pt = distortion_model_to_target.map(pt);
	}
	return CylindricalSurfaceDewarper(
		top_polyline, bottom_polyline, depth_perception
	);
}

/**
 * \param orig_to_src Transformation from the original image coordinates
 *                    to the coordinate system of \p src image.
 * \param src_to_output Transformation from the \p src image coordinates
 *                      to output image coordinates.
 * \param distortion_model Distortion model.
 * \param depth_perception Depth perception.
 * \param bg_color The color to use for areas outsize of \p src.
 * \param modified_content_rect A vertically shrunk version of outputContentRect().
 *                              See function definition for more details.
 */
QImage
OutputGenerator::dewarp(
	QTransform const& orig_to_src, QImage const& src,
	QTransform const& src_to_output, DistortionModel const& distortion_model,
	DepthPerception const& depth_perception, QColor const& bg_color) const
{
	CylindricalSurfaceDewarper const dewarper(
		createDewarper(distortion_model, orig_to_src, depth_perception.value())
	);

	// Model domain is a rectangle in output image coordinates that
	// will be mapped to our curved quadrilateral.
	QRect const model_domain(
		distortion_model.modelDomain(
			dewarper, orig_to_src * src_to_output, outputContentRect()
		).toRect()
	);
	if (model_domain.isEmpty()) {
		GrayImage out(src.size());
		out.fill(0xff); // white
		return out;
	}

	return RasterDewarper::dewarp(
		src, m_outRect.size(), dewarper, model_domain, bg_color
	);
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
OutputGenerator::convertToRGBorRGBA(QImage const& src)
{
	QImage::Format const fmt = src.hasAlphaChannel()
		? QImage::Format_ARGB32 : QImage::Format_RGB32;

	return src.convertToFormat(fmt);
}

void
OutputGenerator::fillMarginsInPlace(
	QImage& image, QPolygonF const& content_poly, QColor const& color)
{
	if (image.format() == QImage::Format_Indexed8 && image.isGrayscale()) {
		PolygonRasterizer::grayFillExcept(
			image, qGray(color.rgb()), content_poly, Qt::WindingFill
		);
		return;
	}

	assert(image.format() == QImage::Format_RGB32 || image.format() == QImage::Format_ARGB32);

	if (image.format() == QImage::Format_ARGB32) {
		image = image.convertToFormat(QImage::Format_ARGB32_Premultiplied);
	}

	{
		QPainter painter(&image);
		painter.setRenderHint(QPainter::Antialiasing);
		painter.setBrush(color);
		painter.setPen(Qt::NoPen);

		QPainterPath outer_path;
		outer_path.addRect(image.rect());
		QPainterPath inner_path;
		inner_path.addPolygon(content_poly);

		painter.drawPath(outer_path.subtracted(inner_path));
	}

	if (image.format() == QImage::Format_ARGB32_Premultiplied) {
		image = image.convertToFormat(QImage::Format_ARGB32);
	}
}

GrayImage
OutputGenerator::detectPictures(
	GrayImage const& input_300dpi, TaskStatus const& status,
	DebugImages* const dbg)
{
	// We stretch the range of gray levels to cover the whole
	// range of [0, 255].  We do it because we want text
	// and background to be equally far from the center
	// of the whole range.  Otherwise text printed with a big
	// font will be considered a picture.
	GrayImage stretched(stretchGrayRange(input_300dpi, 0.01, 0.01));
	if (dbg) {
		dbg->add(stretched, "stretched");
	}
	
	status.throwIfCancelled();
	
	GrayImage eroded(erodeGray(stretched, QSize(3, 3), 0x00));
	if (dbg) {
		dbg->add(eroded, "eroded");
	}
	
	status.throwIfCancelled();
	
	GrayImage dilated(dilateGray(stretched, QSize(3, 3), 0xff));
	if (dbg) {
		dbg->add(dilated, "dilated");
	}
	
	stretched = GrayImage(); // Save memory.
	
	status.throwIfCancelled();
	
	grayRasterOp<CombineInverted>(dilated, eroded);
	GrayImage gray_gradient(dilated);
	dilated = GrayImage();
	eroded = GrayImage();
	if (dbg) {
		dbg->add(gray_gradient, "gray_gradient");
	}
	
	status.throwIfCancelled();
	
	GrayImage marker(erodeGray(gray_gradient, QSize(35, 35), 0x00));
	if (dbg) {
		dbg->add(marker, "marker");
	}
	
	status.throwIfCancelled();
	
	seedFillGrayInPlace(marker, gray_gradient, CONN8);
	GrayImage reconstructed(marker);
	marker = GrayImage();
	if (dbg) {
		dbg->add(reconstructed, "reconstructed");
	}
	
	status.throwIfCancelled();
	
	grayRasterOp<GRopInvert<GRopSrc> >(reconstructed, reconstructed);
	if (dbg) {
		dbg->add(reconstructed, "reconstructed_inverted");
	}
	
	status.throwIfCancelled();
	
	GrayImage holes_filled(createFramedImage(reconstructed.size()));
	seedFillGrayInPlace(holes_filled, reconstructed, CONN8);
	reconstructed = GrayImage();
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

BinaryThreshold
OutputGenerator::adjustThreshold(BinaryThreshold threshold) const
{
	int const adjusted = threshold +
		m_colorParams.blackWhiteOptions().thresholdAdjustment();

	// Hard-bounding threshold values is necessary for example
	// if all the content went into the picture mask.
	return BinaryThreshold(qBound(30, adjusted, 225));
}

BinaryThreshold
OutputGenerator::calcBinarizationThreshold(
	QImage const& image, BinaryImage const& mask) const
{
	GrayscaleHistogram hist(image, mask);
	return adjustThreshold(BinaryThreshold::otsuThreshold(hist));
}

BinaryThreshold
OutputGenerator::calcBinarizationThreshold(
	QImage const& image, QPolygonF const& crop_area, BinaryImage const* mask) const
{
	QPainterPath path;
	path.addPolygon(crop_area);
	
	if (path.contains(image.rect())) {
		return adjustThreshold(BinaryThreshold::otsuThreshold(image));
	} else {
		BinaryImage modified_mask(image.size(), BLACK);
		PolygonRasterizer::fillExcept(modified_mask, WHITE, crop_area, Qt::WindingFill);
		modified_mask = erodeBrick(modified_mask, QSize(3, 3), WHITE);
		
		if (mask) {
			rasterOp<RopAnd<RopSrc, RopDst> >(modified_mask, *mask);
		}
		
		return calcBinarizationThreshold(image, modified_mask);
	}
}

BinaryImage
OutputGenerator::binarize(QImage const& image, BinaryImage const& mask) const
{
	GrayscaleHistogram hist(image, mask);
	BinaryThreshold const bw_thresh(BinaryThreshold::otsuThreshold(hist));
	BinaryImage binarized(image, adjustThreshold(bw_thresh));
	
	// Fill masked out areas with white.
	rasterOp<RopAnd<RopSrc, RopDst> >(binarized, mask);
	
	return binarized;
}

BinaryImage
OutputGenerator::binarize(QImage const& image,
	QPolygonF const& crop_area, BinaryImage const* mask) const
{
	QPainterPath path;
	path.addPolygon(crop_area);
	
	if (path.contains(image.rect()) && !mask) {
		BinaryThreshold const bw_thresh(BinaryThreshold::otsuThreshold(image));
		return BinaryImage(image, adjustThreshold(bw_thresh));
	} else {
		BinaryImage modified_mask(image.size(), BLACK);
		PolygonRasterizer::fillExcept(modified_mask, WHITE, crop_area, Qt::WindingFill);
		modified_mask = erodeBrick(modified_mask, QSize(3, 3), WHITE);
		
		if (mask) {
			rasterOp<RopAnd<RopSrc, RopDst> >(modified_mask, *mask);
		}
		
		return binarize(image, modified_mask);
	}
}

/**
 * \brief Remove small connected components that are considered to be garbage.
 *
 * Both the size and the distance to other components are taken into account.
 *
 * \param[in,out] image The image to despeckle.
 * \param image_rect The rectangle corresponding to \p image in the same
 *        coordinate system where m_contentRect and m_cropRect are defined.
 * \param mask_rect The area within the image to consider.  Defined not
 *        relative to \p image, but in the same coordinate system where
 *        m_contentRect and m_cropRect are defined.  This only affects
 *        \p speckles_img, if provided.
 * \param level Despeckling aggressiveness.
 * \param speckles_img If provided, the removed black speckles will be written
 *        there.  The speckles image is always considered to correspond
 *        to m_cropRect, so it will have the size of m_cropRect.size().
 *        Only the area within \p mask_rect will be copied to \p speckles_img.
 *        The rest will be filled with white.
 * \param dpi The DPI of the input image.  See the note below.
 * \param status Task status.
 * \param dbg An optional sink for debugging images.
 *
 * \note This function only works effectively when the DPI is symmetric,
 * that is, its horizontal and vertical components are equal.
 */
void
OutputGenerator::maybeDespeckleInPlace(
	imageproc::BinaryImage& image,
	QRect const& image_rect, QRect const& mask_rect,
	DespeckleLevel const level, BinaryImage* speckles_img,
	Dpi const& dpi, TaskStatus const& status, DebugImages* dbg) const
{
	QRect const src_rect(mask_rect.translated(-image_rect.topLeft()));
	QRect const dst_rect(mask_rect);

	if (speckles_img) {
		BinaryImage(m_outRect.size(), WHITE).swap(*speckles_img);
		if (!mask_rect.isEmpty()) {
			rasterOp<RopSrc>(*speckles_img, dst_rect, image, src_rect.topLeft());
		}
	}

	if (level != DESPECKLE_OFF) {
		Despeckle::Level lvl = Despeckle::NORMAL;
		switch (level) {
			case DESPECKLE_CAUTIOUS:
				lvl = Despeckle::CAUTIOUS;
				break;
			case DESPECKLE_NORMAL:
				lvl = Despeckle::NORMAL;
				break;
			case DESPECKLE_AGGRESSIVE:
				lvl = Despeckle::AGGRESSIVE;
				break;
			default:;
		}

		Despeckle::despeckleInPlace(image, dpi, lvl, status, dbg);

		if (dbg) {
			dbg->add(image, "despeckled");
		}
	}

	if (speckles_img) {
		if (!mask_rect.isEmpty()) {
			rasterOp<RopSubtract<RopDst, RopSrc> >(
				*speckles_img, dst_rect, image, src_rect.topLeft()
			);
		}
	}
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

unsigned char
OutputGenerator::calcDominantBackgroundGrayLevel(QImage const& img)
{
	// TODO: make a color version.
	// In ColorPickupInteraction.cpp we have code for median color finding.
	// We can use that.

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
	for (int i = 1; i <= num_colors - window_size; ++i) {
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

void
OutputGenerator::applyFillZonesInPlace(
	QImage& img, ZoneSet const& zones,
	boost::function<QPointF(QPointF const&)> const& orig_to_output) const
{
	if (zones.empty()) {
		return;
	}

	QImage canvas(img.convertToFormat(QImage::Format_ARGB32_Premultiplied));
	
	{
		QPainter painter(&canvas);
		painter.setRenderHint(QPainter::Antialiasing, true);
		painter.setPen(Qt::NoPen);

		BOOST_FOREACH(Zone const& zone, zones) {
			QColor const color(zone.properties().locateOrDefault<FillColorProperty>()->color());
			QPolygonF const poly(zone.spline().transformed(orig_to_output).toPolygon());
			painter.setBrush(color);
			painter.drawPolygon(poly, Qt::WindingFill);
		}
	}

	if (img.format() == QImage::Format_Indexed8 && img.isGrayscale()) {
		img = toGrayscale(canvas);
	} else {
		img = canvas.convertToFormat(img.format());
	}
}

/**
 * A simplified version of the above, using toOutput() for translation
 * from original image to output image coordinates.
 */
void
OutputGenerator::applyFillZonesInPlace(QImage& img, ZoneSet const& zones) const
{
	typedef QPointF (QTransform::*MapPointFunc)(QPointF const&) const;
	applyFillZonesInPlace(
		img, zones, boost::bind((MapPointFunc)&QTransform::map, m_xform.transform(), _1)
	);
}

void
OutputGenerator::applyFillZonesInPlace(
	imageproc::BinaryImage& img, ZoneSet const& zones,
	boost::function<QPointF(QPointF const&)> const& orig_to_output) const
{
	if (zones.empty()) {
		return;
	}

	BOOST_FOREACH(Zone const& zone, zones) {
		QColor const color(zone.properties().locateOrDefault<FillColorProperty>()->color());
		BWColor const bw_color = qGray(color.rgb()) < 128 ? BLACK : WHITE;
		QPolygonF const poly(zone.spline().transformed(orig_to_output).toPolygon());
		PolygonRasterizer::fill(img, bw_color, poly, Qt::WindingFill);
	}
}

/**
 * A simplified version of the above, using toOutput() for translation
 * from original image to output image coordinates.
 */
void
OutputGenerator::applyFillZonesInPlace(
	imageproc::BinaryImage& img, ZoneSet const& zones) const
{
	typedef QPointF (QTransform::*MapPointFunc)(QPointF const&) const;
	applyFillZonesInPlace(
		img, zones, boost::bind((MapPointFunc)&QTransform::map, m_xform.transform(), _1)
	);
}

//begin of modified by monday2000
//Marginal_Dewarping
void 
OutputGenerator::movePointToTopMargin(BinaryImage& bw_image, XSpline& spline, int idx) const //added
{
	QPointF pos = spline.controlPointPosition(idx);

	for (int j=0; j<pos.y(); j++)
	{
		if (bw_image.getPixel(pos.x(),j) == WHITE)
		{
			int count = 0;
			int check_num = 16;

			for (int jj=j; jj<(j+check_num); jj++)
			{
				if (bw_image.getPixel(pos.x(),jj) == WHITE)
					count++;			
			}

			if (count == check_num)
			{
				pos.setY(j);

				spline.moveControlPoint(idx,pos);

				break;
			}
		}
	}
}

void 
OutputGenerator::movePointToBottomMargin(BinaryImage& bw_image, XSpline& spline, int idx) const //added
{
	QPointF pos = spline.controlPointPosition(idx);

	for (int j=bw_image.height()-1; j>pos.y(); j--)
	{
		if (bw_image.getPixel(pos.x(),j) == WHITE)
		{
			int count = 0;
			int check_num = 16;

			for (int jj=j; jj>(j-check_num); jj--)
			{
				if (bw_image.getPixel(pos.x(),jj) == WHITE)
					count++;
			}

			if (count == check_num)
			{
				pos.setY(j);

				spline.moveControlPoint(idx,pos);

				break;
			}
		}
	}
}

void
OutputGenerator::drawPoint(QImage& image, QPointF const& pt) const
{
	QPoint pts = pt.toPoint();

	for (int i=pts.x()-10;i<pts.x()+10;i++)
	{
		for (int j=pts.y()-10;j<pts.y()+10;j++)
		{		
		
		QPoint p1(i,j);	
		
		image.setPixel(p1, qRgb(255, 0, 0));

		}
	}
}

void 
OutputGenerator::movePointToTopMargin(BinaryImage& bw_image, std::vector<QPointF>& polyline, int idx) const //added
{
	QPointF& pos = polyline[idx];

	for (int j=0; j<pos.y(); j++)
	{
		if (bw_image.getPixel(pos.x(),j) == WHITE)
		{
			int count = 0;
			int check_num = 16;

			for (int jj=j; jj<(j+check_num); jj++)
			{
				if (bw_image.getPixel(pos.x(),jj) == WHITE)
					count++;			
			}

			if (count == check_num)
			{
				pos.setY(j);			

				break;
			}
		}
	}
}

void 
OutputGenerator::movePointToBottomMargin(BinaryImage& bw_image, std::vector<QPointF>& polyline, int idx) const //added
{
	QPointF& pos = polyline[idx];

	for (int j=bw_image.height()-1; j>pos.y(); j--)
	{
		if (bw_image.getPixel(pos.x(),j) == WHITE)
		{
			int count = 0;
			int check_num = 16;

			for (int jj=j; jj>(j-check_num); jj--)
			{
				if (bw_image.getPixel(pos.x(),jj) == WHITE)
					count++;
			}

			if (count == check_num)
			{
				pos.setY(j);				

				break;
			}
		}
	}
}

float
OutputGenerator::vert_border_skew_angle(QPointF const& top, QPointF const& bottom) const
{
	 return qFabs(qAtan((bottom.x() - top.x()) / (bottom.y() - top.y())) * 180/M_PI);
}

void
OutputGenerator::maybe_deskew(QImage* p_dewarped, DewarpingMode dewarping_mode) const
{
	if (dewarping_mode == DewarpingMode::MARGINAL 
		|| dewarping_mode == DewarpingMode::MANUAL
		)
	{
		//TiffWriter::writeImage("C:\\st\\dewarped.tif", dewarped);

		BinaryThreshold bw_threshold(128);	
		BinaryImage bw_image(*p_dewarped, bw_threshold);

		SkewFinder skew_finder;
		Skew const skew(skew_finder.findSkew(bw_image));
		if (skew.angle() != 0.0 && skew.confidence() >= Skew::GOOD_CONFIDENCE)
		{
			double const angle_deg = skew.angle();

			//QFile file("C:\\st\\scan_tailor.txt");
			//file.open(QIODevice::WriteOnly | QIODevice::Text);
			//QTextStream out(&file);
			//QString stDeskewAngle;	
			//stDeskewAngle.setNum(angle_deg);
			//out << "deskew angle: " << stDeskewAngle << endl;
			//QMessageBox::information(0,"i",stDeskewAngle);

			QPointF center(p_dewarped->width()/2, p_dewarped->height()/2);

			QTransform rot;		
			rot.translate(center.x(),center.y());
			rot.rotate(-angle_deg);		
			rot.translate(-center.x(),-center.y());

			*p_dewarped = imageproc::transform(*p_dewarped, rot, p_dewarped->rect(), OutsidePixels::assumeWeakColor(Qt::white));

			//TiffWriter::writeImage("C:\\st\\dewarped2.tif", dewarped);
			
			//file.close();
		}
	}

	//return dewarped;
}
//end of modified by monday2000

} // namespace output
