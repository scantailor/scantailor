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

#ifndef SIDES_OF_LINE_H_
#define SIDES_OF_LINE_H_

#include <QPointF>
#include <QLineF>

/**
 * This function allows you to check if a pair of points is on the same
 * or different sides of a line.
 *
 * Returns:
 * \li Negative value, if points are on different sides of line.
 * \li Positive value, if points are on the same side of line.
 * \li Zero, if one or both of the points are on the line.
 *
 * \note Line's endpoints don't matter - consider the whole line,
 *       not a line segment.  If the line is really a point, zero will
 *       always be returned.
 */
qreal sidesOfLine(QLineF const& line, QPointF const& p1, QPointF const& p2);

#endif
