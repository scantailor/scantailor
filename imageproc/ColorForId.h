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

#ifndef IMAGEPROC_COLOR_FOR_ID_H_
#define IMAGEPROC_COLOR_FOR_ID_H_

#include "BitOps.h"
#include <QColor>

namespace imageproc
{

/**
 * \brief Generates a color corresponding to a particular numeric ID.
 *
 * Colors for IDs that are numerically close will tend to be significantly
 * different.  Positive IDs are handled better.
 */
template<typename T>
QColor colorForId(T id) {
	int const bits_unused = countMostSignificantZeroes(id);
	int const bits_used = sizeof(T) * 8 - bits_unused;
	T const reversed = reverseBits(id) >> bits_unused;
	T const mask = (T(1) << bits_used) - 1;

	double const H = 0.99 * double(reversed + 1) / (mask + 1);
	double const S = 1.0;
	double const V = 1.0;
	QColor color;
	color.setHsvF(H, S, V);
	
	return color;
}

} // namespace imageproc

#endif
