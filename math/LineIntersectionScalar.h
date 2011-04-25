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

#ifndef LINE_INTERSECTION_SCALAR_H_
#define LINE_INTERSECTION_SCALAR_H_

#include <QLineF>

/**
 * Finds such scalars s1 and s2, so that "line1.pointAt(s1)" and "line2.pointAt(s2)"
 * would be the intersection point between line1 and line2.  Returns false if the
 * lines are parallel or if any of the lines have zero length and therefore no direction.
 */
bool lineIntersectionScalar(QLineF const& line1, QLineF const& line2, double& s1, double& s2);

/**
 * Same as the one above, but doesn't bother to calculate s2.
 */
bool lineIntersectionScalar(QLineF const& line1, QLineF const& line2, double& s1);

#endif
