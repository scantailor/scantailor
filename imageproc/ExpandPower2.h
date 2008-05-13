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

#ifndef IMAGEPROC_EXPANDPOWER2_H_
#define IMAGEPROC_EXPANDPOWER2_H_

#include "BWColor.h"

class QSize;

namespace imageproc
{

class BinaryImage;

/**
 * \brief Expand the image in both directions by a power of 2.
 */
BinaryImage expandPower2(BinaryImage const& src, int exponent);

/**
 * \brief Expand the image in both directions by a power of 2.
 *
 * In this case, expanding factor will be calculated based on \p dst_size.
 * The resulting image will have a size equal to \p dst_size, which doesn't
 * have to be src.size() multiplied by a certain power of 2.  If it's not,
 * it's considered that the caller wants a slightly bigger resulting image
 * than src.size() multiplied by a certain power of 2.  That extra area
 * will be filled with \p bgcolor.
 */
BinaryImage expandPower2(BinaryImage const& src, QSize const& dst_size, BWColor bgcolor);

} // namespace imageproc

#endif
