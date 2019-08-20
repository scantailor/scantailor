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

#ifndef PAGE_LAYOUT_UTILS_H_
#define PAGE_LAYOUT_UTILS_H_

class QPolygonF;
class QPointF;
class QSizeF;
class QRectF;
class Margins;
class ImageTransformation;

namespace page_layout
{

class Alignment;
class Params;

class Utils
{
public:
	/**
	 * \brief Replace an empty content rectangle with a tiny centered one.
	 *
	 * If the content rectangle is empty (no content on the page), it
	 * creates various problems for us.  So, we replace it with a tiny
	 * non-empty rectangle centered in the page's crop area, which
	 * is retrieved from the ImageTransformation.
	 */
	static QRectF adaptContentRect(
		ImageTransformation const& xform, QRectF const& content_rect);
	
	/**
	 * \brief Calculates the physical size of a rectangle in a transformed space.
	 */
	static QSizeF calcRectSizeMM(
		ImageTransformation const& xform, QRectF const& rect);
	
	/**
	 * \brief Extend a rectangle transformed into a polygon with margins.
	 *
	 * The first edge of the polygon is considered to be the top edge, the
	 * next one is right, and so on.  The polygon must have 4 or 5 vertices
	 * (unclosed vs closed polygon).  It must have 90 degree angles and
	 * must not be empty.
	 */
	static void extendPolyRectWithMargins(
		QPolygonF& poly_rect, Margins const& margins);
	
	/**
	 * \brief Calculates margins to extend hard_size_mm to aggregate_hard_size_mm.
	 *
	 * \param hard_size_mm Source size in millimeters.
	 * \param aggregate_hard_size_mm Target size in millimeters.
	 * \param alignment Determines how exactly to grow the size.
	 * \return Non-negative margins that extend \p hard_size_mm to
	 *         \p aggregate_hard_size_mm.
	 */
	static Margins calcSoftMarginsMM(
		QSizeF const& hard_size_mm,
		QSizeF const& aggregate_hard_size_mm,
		Alignment const& alignment);
	
	/**
	 * \brief Calculates the page rect (content + hard margins + soft margins)
	 *
	 * \param xform Transformations applied to image.
	 * \param content_rect_phys Content rectangle in transformed coordinates.
	 * \param params Margins, alignment and other parameters.
	 * \param aggregate_hard_size_mm Maximum width and height across all pages.
	 * \return Page rectangle (as a polygon) in physical image coordinates.
	 */
	static QPolygonF calcPageRectPhys(
		ImageTransformation const& xform,
		QPolygonF const& content_rect_phys,
		Params const& params, QSizeF const& aggregate_hard_size_mm);
private:
	static QPointF getRightUnitVector(QPolygonF const& poly_rect);
	
	static QPointF getDownUnitVector(QPolygonF const& poly_rect);
};

} // namespace page_layout

#endif
