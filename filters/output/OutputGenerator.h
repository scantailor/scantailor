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

#ifndef OUTPUT_OUTPUTGENERATOR_H_
#define OUTPUT_OUTPUTGENERATOR_H_

#include "Dpi.h"
#include "ColorParams.h"
#include <QRect>
#include <QTransform>
#include <QColor>
#include <QPolygonF>

class TaskStatus;
class ImageTransformation;
class DebugImages;
class QSize;
class QImage;

namespace imageproc
{
	class BinaryImage;
}

namespace output
{

class OutputGenerator
{
public:
	OutputGenerator(
		Dpi const& dpi, ColorParams const& color_params,
		ImageTransformation const& pre_xform,
		QPolygonF const& content_rect_phys,
		QPolygonF const& page_rect_phys);
	
	QImage process(QImage const& input,
		TaskStatus const& status, DebugImages* dbg = 0) const;
private:
	QImage processBitonalOrBW(QImage const& input,
		TaskStatus const& status, DebugImages* dbg = 0) const;
	
	QImage processColorOrGrayscale(QImage const& input,
		TaskStatus const& status, DebugImages* dbg = 0) const;
	
	QImage processAutoHalftone(QImage const& input,
		TaskStatus const& status, DebugImages* dbg = 0) const;
	
	static QSize from300dpi(QSize const& size, Dpi const& target_dpi);
	
	static void hitMissReplaceAllDirections(
		imageproc::BinaryImage& img, char const* pattern,
		int pattern_width, int pattern_height);
	
	static QSize calcLocalWindowSize(Dpi const& dpi);
	
	static void colorizeBitonal(
		QImage& img, QRgb light_color, QRgb dark_color);
	
	static unsigned char calcDominantBackgroundGrayLevel(QImage const& img);
	
	Dpi m_dpi;
	ColorParams m_colorParams;
	QPolygonF m_pageRectPhys;
	
	/**
	 * Transformation from the input image coordinates to coordinates
	 * of the output image before it's cropped.
	 */
	QTransform m_toUncropped;
	
	/**
	 * The cropping rectangle in uncropped coordinates.
	 */
	QRect m_cropRect;
	
	/**
	 * The content rectangle in uncropped coordinates.
	 */
	QRect m_contentRect;
};

} // namespace output

#endif
