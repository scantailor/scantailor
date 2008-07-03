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

#ifndef IMAGEPROC_POLYGONUTILS_H_
#define IMAGEPROC_POLYGONUTILS_H_

class QPolygonF;
class QPointF;

namespace imageproc
{

class PolygonUtils
{
public:
	/**
	 * \brief Adjust vertices to more round coordinates.
	 *
	 * The method exists to workaround bugs in QPainterPath and QPolygonF
	 * composition operations.  It turns out rounding vertex coordinates
	 * solves many of those bugs.  We don't round to integer values, we
	 * only make a very minor adjustments.
	 */
	static QPolygonF round(QPolygonF const& poly);
private:
	static QPointF roundPoint(QPointF const& p);
	
	static double roundValue(double val);
};

} // namespace imageproc

#endif
