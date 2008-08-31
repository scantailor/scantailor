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

#ifndef IMAGEPROC_GRAYRASTEROP_H_
#define IMAGEPROC_GRAYRASTEROP_H_

#include "Grayscale.h"
#include <QImage>
#include <QPoint>
#include <QRect>
#include <QSize>
#include <stdexcept>
#include <stdint.h>
#include <assert.h>

namespace imageproc
{

/**
 * \brief Perform pixel-wise operations on two images.
 *
 * \param dst The destination image.  Changes will be written there.
 * \param src The source image.  May be the same as the destination image.
 *
 * The template argument is the operation to perform.  This is generally a
 * combination of several GRop* class templates, such as
 * GRopSubtract\<GRopSrc, GRopDst\>.
 */
template<typename GRop>
void grayRasterOp(QImage const& dst, QImage const& src);

/**
 * \brief Raster operation that takes source pixels as they are.
 * \see grayRasterOp()
 */
class GRopSrc
{
public:
	static uint8_t transform(uint8_t src, uint8_t /*dst*/) {
		return src;
	}
};

/**
 * \brief Raster operation that takes destination pixels as they are.
 * \see grayRasterOp()
 */
class GRopDst
{
public:
	static uint8_t transform(uint8_t /*src*/, uint8_t dst) {
		return dst;
	}
};

/**
 * \brief Raster operation that inverts the gray level.
 * \see grayRasterOp()
 */
template<typename Arg>
class GRopInvert
{
public:
	static uint8_t transform(uint8_t src, uint8_t dst) {
		return uint8_t(0xff) - Arg::transform(src, dst);
	}
};

/**
 * \brief Raster operation that performs subtracts gray levels of Rhs from Lhs.
 * \see grayRasterOp()
 */
template<typename Lhs, typename Rhs>
class GRopSubtract
{
public:
	static uint8_t transform(uint8_t src, uint8_t dst) {
		uint8_t const lhs = Lhs::transform(src, dst);
		uint8_t const rhs = Rhs::transform(src, dst);
		return lhs > rhs ? lhs - rhs : uint8_t(0);
	}
};


template<typename GRop>
void grayRasterOp(QImage& dst, QImage const& src)
{
	if (dst.isNull() || src.isNull()) {
		throw std::invalid_argument("grayRasterOp: can't operate on null images");
	}
	
	if (src.size() != dst.size()) {
		throw std::invalid_argument("grayRasterOp: images sizes are not the same");
	}
	
	if (src.format() != QImage::Format_Indexed8 || !src.isGrayscale()) {
		throw std::invalid_argument("grayRasterOp: source image is not grayscale");
	}
	
	if (dst.format() != QImage::Format_Indexed8 || !dst.isGrayscale()) {
		throw std::invalid_argument("grayRasterOp: source image is not grayscale");
	}
	
	if (dst.numColors() != 256) {
		dst.setColorTable(createGrayscalePalette());
	}
	
	uint8_t const* src_line = src.bits();
	uint8_t* dst_line = dst.bits();
	int const src_bpl = src.bytesPerLine();
	int const dst_bpl = dst.bytesPerLine();
	
	int const width = src.width();
	int const height = src.height();
	
	for (int y = 0; y < height; ++y) {
		for (int x = 0; x < width; ++x) {
			dst_line[x] = GRop::transform(src_line[x], dst_line[x]);
		}
		src_line += src_bpl;
		dst_line += dst_bpl;
	}
}

} // namespace imageproc

#endif
