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

#ifndef OUTPUT_OUTPUT_IMAGE_PARAMS_H_
#define OUTPUT_OUTPUT_IMAGE_PARAMS_H_

#include "Dpi.h"
#include "ColorParams.h"
#include <QSize>
#include <QRect>

class ImageTransformation;
class QDomDocument;
class QDomElement;
class QTransform;

namespace output
{

/**
 * \brief Parameters of the output image used to determine if we need to re-generate it.
 */
class OutputImageParams
{
public:
	OutputImageParams(QSize const& out_image_size, QRect const& content_rect,
		ImageTransformation const& xform,
		Dpi const& dpi, ColorParams const& color_params);
	
	explicit OutputImageParams(QDomElement const& el);
	
	QDomElement toXml(QDomDocument& doc, QString const& name) const;
	
	/**
	 * \brief Returns true if two sets of parameters are close enough
	 *        to avoid re-generating the output image.
	 */
	bool matches(OutputImageParams const& other) const;
private:
	class PartialXform
	{
	public:
		PartialXform(QTransform const& xform);
		
		PartialXform(QDomElement const& el);
		
		QDomElement toXml(QDomDocument& doc, QString const& name) const;
		
		bool matches(PartialXform const& other) const;
	private:
		static bool closeEnough(double v1, double v2);
		
		double m_11;
		double m_12;
		double m_21;
		double m_22;
	};
	
	static bool colorParamsMatch(ColorParams const& cp1, ColorParams const& cp2);
	
	/** Pixel size of the output image. */
	QSize m_size;
	
	/** Content rectangle in output image coordinates. */
	QRect m_contentRect;
	
	/**
	 * Some parameters from the transformation matrix that maps
	 * source image coordinates to unscaled (disregarding dpi changes)
	 * output image coordinates.
	 */
	PartialXform m_partialXform;
	
	/** DPI of the output image. */
	Dpi m_dpi;
	
	/** Non-geometric parameters used to generate the output image. */
	ColorParams m_colorParams;
};

} // namespace output

#endif
