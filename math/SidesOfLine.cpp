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

#include "SidesOfLine.h"

qreal sidesOfLine(QLineF const& line, QPointF const& p1, QPointF const& p2)
{
	QPointF const normal(line.normalVector().p2() - line.p1());
	QPointF const vec1(p1 - line.p1());
	QPointF const vec2(p2 - line.p1());
	qreal const dot1 = normal.x() * vec1.x() + normal.y() * vec1.y();
	qreal const dot2 = normal.x() * vec2.x() + normal.y() * vec2.y();
	return dot1 * dot2;
}
