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

#ifndef IMAGE_PRESENTATION_H_
#define IMAGE_PRESENTATION_H_

#include <QTransform>
#include <QPolygonF>
#include <QRectF>

/**
 * Image presentation consists of 3 components:
 * \li A transformation from pixel coordinates to some arbitrary virtual coordinates.
 *     Such transformation usually comes from an instance of ImageTransformation.
 * \li Image crop area, in virtual coordinates, that specifies which parts of the
 *     image should be visible.
 * \li Display area, in virtual coordinates, which is usually the bounding box
 *     of the image crop area, but can be larger, to reserve space for custom drawing
 *     or extra controls.
 */
class ImagePresentation
{
	// Member-wise copying is OK.
public:
	ImagePresentation(QTransform const& xform, QPolygonF const& crop_area)
			: m_xform(xform), m_cropArea(crop_area), m_displayArea(crop_area.boundingRect()) {}

	ImagePresentation(QTransform const& xform, QPolygonF const& crop_area, QRectF const& display_area)
			: m_xform(xform), m_cropArea(crop_area), m_displayArea(display_area) {}

	QTransform const& transform() const { return m_xform; }

	void setTransform(QTransform const& xform) { m_xform = xform; }

	QPolygonF const& cropArea() const { return m_cropArea; }

	void setCropArea(QPolygonF const& crop_area) { m_cropArea = crop_area; }

	QRectF const& displayArea() const { return m_displayArea; }

	void setDisplayArea(QRectF const& display_area) { m_displayArea = display_area; }
private:
	QTransform m_xform;
	QPolygonF m_cropArea;
	QRectF m_displayArea;
};

#endif
