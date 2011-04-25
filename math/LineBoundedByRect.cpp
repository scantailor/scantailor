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

#include "LineBoundedByRect.h"
#include "LineIntersectionScalar.h"
#include "NumericTraits.h"
#include <boost/foreach.hpp>

bool lineBoundedByRect(QLineF& line, QRectF const& rect)
{
	QLineF const rect_lines[4] = {
		QLineF(rect.topLeft(), rect.topRight()),
		QLineF(rect.bottomLeft(), rect.bottomRight()),
		QLineF(rect.topLeft(), rect.bottomLeft()),
		QLineF(rect.topRight(), rect.bottomRight())
	};
	
	double max = NumericTraits<double>::min();
	double min = NumericTraits<double>::max();

	double s1 = 0;
	double s2 = 0;
	BOOST_FOREACH(QLineF const& rect_line, rect_lines) {	
		if (!lineIntersectionScalar(rect_line, line, s1, s2)) {
			// line is parallel to rect_line.
			continue;
		}
		if (s1 < 0 || s1 > 1) {
			// Intersection outside of rect.
			continue;
		}

		if (s2 > max) {
			max = s2;
		}
		if (s2 < min) {
			min = s2;
		}
	}

	if (max > min) {
		line = QLineF(line.pointAt(min), line.pointAt(max));
		return true;
	} else {
		return false;
	}
}
