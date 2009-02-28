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

#ifndef IMAGEPROC_UPSCALE_INTEGER_TIMES_H_
#define IMAGEPROC_UPSCALE_INTEGER_TIMES_H_

#include "BWColor.h"

class QSize;

namespace imageproc
{

class BinaryImage;

/**
 * \brief Upscale a binary image integer times in each direction.
 */
BinaryImage upscaleIntegerTimes(BinaryImage const& src, int xscale, int yscale);

/**
 * \brief Upscale a binary image integer times in each direction
 *        and add padding if necessary.
 *
 * The resulting image will have a size of \p dst_size, which is achieved
 * by upscaling the source image integer times in each direction and then
 * adding a padding to reach the requested size.
 */
BinaryImage upscaleIntegerTimes(
	BinaryImage const& src, QSize const& dst_size, BWColor padding);

} // namespace imageproc

#endif
