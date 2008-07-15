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
class Margins;
class ImageTransformation;

namespace page_layout
{

class Alignment;

class Utils
{
public:
	static void extendPolyRectWithMargins(
		QPolygonF& poly_rect, Margins const& margins);
	
	static Margins calcSoftMarginsMM(
		QSizeF const& hard_size_mm,
		QSizeF const& aggregate_hard_size_mm,
		Alignment const& alignment);
	
	static ImageTransformation calcPresentationTransform(
		ImageTransformation const& orig_xform,
		QPolygonF const& physical_crop_area);
private:
	static QPointF getRightUnitVector(QPolygonF const& poly_rect);
	
	static QPointF getDownUnitVector(QPolygonF const& poly_rect);
};

} // namespace page_layout

#endif
