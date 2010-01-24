/*
	Scan Tailor - Interactive post-processing tool for scanned pages.
	Copyright (C)  Joseph Artsimovich <joseph.artsimich@gmail.com>

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

#ifndef IMAGEPROC_RASTER_OP_GENERIC_H_
#define IMAGEPROC_RASTER_OP_GENERIC_H_

#include "BinaryImage.h"
#include <QSize>
#include <stdint.h>

namespace imageproc
{

/**
 * \brief Perform an operation on a single image.
 *
 * \param data The pointer to image data.
 * \param stride The number of elements of type T per image line.
 * \param size Image size.
 * \param operation An operation to perform.  It will be called like this:
 * \code
 * operation(data[offset]);
 * \endcode
 * Depending on whether T is const, the operation may be able to modify the image.
 * Hinst: boost::lambda is an easy way to construct operations.
 */
template<typename T, typename Op>
void rasterOpGeneric(T* data, int stride, QSize size, Op operation)
{
	if (size.isEmpty()) {
		return;
	}

	int const w = size.width();
	int const h = size.height();

	for (int y = 0; y < h; ++y) {
		for (int x = 0; x < w; ++x) {
			operation(data[x]);
		}
		data += stride;
	}
}

/**
 * \brief Perform an operation on a pair of images.
 *
 * \param data1 The pointer to image data of the first image.
 * \param stride1 The number of elements of type T1 per line of the first image.
 * \param size Dimensions of both images.
 * \param data2 The pointer to image data of the second image.
 * \param stride2 The number of elements of type T2 per line of the second image.
 * \param operation An operation to perform.  It will be called like this:
 * \code
 * operation(data1[offset1], data2[offset2]);
 * \endcode
 * Depending on whether T1 / T2 are const, the operation may be able to modify
 * one or both of them.
 * Hinst: boost::lambda is an easy way to construct operations.
 */
template<typename T1, typename T2, typename Op>
void rasterOpGeneric(T1* data1, int stride1, QSize size,
					 T2* data2, int stride2, Op operation)
{
	if (size.isEmpty()) {
		return;
	}

	int const w = size.width();
	int const h = size.height();

	for (int y = 0; y < h; ++y) {
		for (int x = 0; x < w; ++x) {
			operation(data1[x], data2[x]);
		}
		data1 += stride1;
		data2 += stride2;
	}
}

} // namespace imageproc

#endif
