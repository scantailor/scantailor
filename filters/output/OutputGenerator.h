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

#ifndef OUTPUT_OUTPUTGENERATOR_H_
#define OUTPUT_OUTPUTGENERATOR_H_

#include "imageproc/Connectivity.h"
#include "Dpi.h"
#include "ColorParams.h"
#include <QSize>
#include <QRect>
#include <QTransform>
#include <QColor>
#include <QPolygonF>
#include <stdint.h>

class TaskStatus;
class DebugImages;
class ImageTransformation;
class FilterData;
class QSize;
class QImage;

namespace imageproc
{
	class BinaryImage;
	class BinaryThreshold;
}

namespace output
{

class PictureZoneList;

class OutputGenerator
{
public:
	OutputGenerator(
		Dpi const& dpi, ColorParams const& color_params,
		ImageTransformation const& pre_xform,
		QPolygonF const& content_rect_phys,
		QPolygonF const& page_rect_phys);
	
	QImage process(
		TaskStatus const& status, FilterData const& input, PictureZoneList const& zones,
		imageproc::BinaryImage* auto_picture_mask = 0, DebugImages* dbg = 0) const;
	
	/**
	 * Returns the transformation from original to output image coordinates.
	 */
	QTransform toOutput() const;

	QSize outputImageSize() const;
	
	/**
	 * \brief Returns the content rectangle in output image coordinates.
	 */
	QRect outputContentRect() const;
private:
	QImage processAsIs(FilterData const& input,
		TaskStatus const& status, DebugImages* dbg = 0) const;
	
	QImage processImpl(
		TaskStatus const& status, FilterData const& input, PictureZoneList const& zones,
		imageproc::BinaryImage* auto_picture_mask = 0, DebugImages* dbg = 0) const;
	
	static QSize from300dpi(QSize const& size, Dpi const& target_dpi);
	
	static QSize to300dpi(QSize const& size, Dpi const& source_dpi);
	
	static QImage normalizeIlluminationGray(
		TaskStatus const& status,
		QImage const& input, QPolygonF const& area_to_consider,
		QTransform const& xform, QRect const& target_rect, DebugImages* dbg);
	
	static QImage detectPictures(
		QImage const& input_300dpi, TaskStatus const& status,
		DebugImages* dbg = 0);
	
	imageproc::BinaryImage estimateBinarizationMask(
		TaskStatus const& status, QImage const& gray_source,
		QRect const& source_rect, QRect const& source_sub_rect,
		DebugImages* const dbg) const;

	void modifyBinarizationMask(
		imageproc::BinaryImage& bw_mask,
		QRect const& mask_rect, PictureZoneList const& zones) const;
	
	imageproc::BinaryThreshold adjustThreshold(
		imageproc::BinaryThreshold threshold) const;
	
	imageproc::BinaryImage binarize(
		QImage const& image, imageproc::BinaryImage const& mask) const;
	
	imageproc::BinaryImage binarize(
		QImage const& image, QPolygonF const& crop_area,
		imageproc::BinaryImage const* mask = 0) const;
	
	static QImage smoothToGrayscale(QImage const& src, Dpi const& dpi);
	
	static void despeckleInPlace(
		imageproc::BinaryImage& image, Dpi const& dpi,
		TaskStatus const& status, DebugImages* dbg);
	
	static void morphologicalSmoothInPlace(
		imageproc::BinaryImage& img, TaskStatus const& status);
	
	static void hitMissReplaceAllDirections(
		imageproc::BinaryImage& img, char const* pattern,
		int pattern_width, int pattern_height);
	
	static QSize calcLocalWindowSize(Dpi const& dpi);
	
	static void colorizeBitonal(
		QImage& img, QRgb light_color, QRgb dark_color);
	
	static unsigned char calcDominantBackgroundGrayLevel(QImage const& img);
	
	static QImage normalizeIllumination(QImage const& gray_input, DebugImages* dbg);
	
	QImage transformAndNormalizeIllumination(
		QImage const& gray_input, DebugImages* dbg,
		QImage const* morph_background = 0) const;
	
	QImage transformAndNormalizeIllumination2(
		QImage const& gray_input, DebugImages* dbg,
		QImage const* morph_background = 0) const;
	
	Dpi m_dpi;
	ColorParams m_colorParams;
	QPolygonF m_pageRectPhys;
	
	/**
	 * Transformation from the input image coordinates to coordinates
	 * of the output image before it's cropped.  This transformation
	 * includes all transformations from the ImageTransformation
	 * passed to a constructor, plus scaling to produce the
	 * desired output DPI.
	 */
	QTransform m_toUncropped;
	
	/**
	 * The content rectangle in m_toUncropped coordinates.
	 */
	QRect m_contentRect;
	
	/**
	 * The cropping rectangle in m_toUncropped coordinates.
	 * The cropping rectangle is the content rect plus margins.
	 */
	QRect m_cropRect;
	
	/**
	 * The bounding rectangle of a polygon that represents
	 * the page boundaries.  The polygon is usually formed
	 * by image edges and a split line.  This rectangle is
	 * in m_toUncropped coordinates.
	 */
	QRect m_fullPageRect;
};

} // namespace output

#endif
