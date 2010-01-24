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

#ifndef IMAGEPROC_GRAYRASTEROP_H_
#define IMAGEPROC_GRAYRASTEROP_H_

#include "Grayscale.h"
#include "GrayImage.h"
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
void grayRasterOp(GrayImage& dst, GrayImage const& src);

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
 * \brief Raster operation that subtracts gray levels of Rhs from Lhs.
 *
 * The "Clipped" part of the name indicates that negative subtraction results
 * are turned into zero.
 *
 * \see grayRasterOp()
 */
template<typename Lhs, typename Rhs>
class GRopClippedSubtract
{
public:
	static uint8_t transform(uint8_t src, uint8_t dst) {
		uint8_t const lhs = Lhs::transform(src, dst);
		uint8_t const rhs = Rhs::transform(src, dst);
		return lhs > rhs ? lhs - rhs : uint8_t(0);
	}
};

/**
 * \brief Raster operation that subtracts gray levels of Rhs from Lhs.
 *
 * The "Unclipped" part of the name indicates that underflows aren't handled.
 * Negative results will appear as 256 - |negative_result|.
 *
 * \see grayRasterOp()
 */
template<typename Lhs, typename Rhs>
class GRopUnclippedSubtract
{
public:
	static uint8_t transform(uint8_t src, uint8_t dst) {
		uint8_t const lhs = Lhs::transform(src, dst);
		uint8_t const rhs = Rhs::transform(src, dst);
		return lhs - rhs;
	}
};

/**
 * \brief Raster operation that sums Rhs and Lhs gray levels.
 *
 * The "Clipped" part of the name indicates that overflow are clipped at 255.
 *
 * \see grayRasterOp()
 */
template<typename Lhs, typename Rhs>
class GRopClippedAdd
{
public:
	static uint8_t transform(uint8_t src, uint8_t dst) {
		unsigned const lhs = Lhs::transform(src, dst);
		unsigned const rhs = Rhs::transform(src, dst);
		unsigned const sum = lhs + rhs;
		return sum < 256 ? static_cast<uint8_t>(sum) : uint8_t(255);
	}
};

/**
 * \brief Raster operation that sums Rhs and Lhs gray levels.
 *
 * The "Unclipped" part of the name indicates that overflows aren't handled.
 * Results exceeding 255 will appear as result - 256.
 *
 * \see grayRasterOp()
 */
template<typename Lhs, typename Rhs>
class GRopUnclippedAdd
{
public:
	static uint8_t transform(uint8_t src, uint8_t dst) {
		uint8_t const lhs = Lhs::transform(src, dst);
		uint8_t const rhs = Rhs::transform(src, dst);
		return lhs + rhs;
	}
};

/**
 * \brief Raster operation that takes the darkest of its arguments.
 * \see grayRasterOp()
 */
template<typename Lhs, typename Rhs>
class GRopDarkest
{
public:
	static uint8_t transform(uint8_t src, uint8_t dst) {
		uint8_t const lhs = Lhs::transform(src, dst);
		uint8_t const rhs = Rhs::transform(src, dst);
		return lhs < rhs ? lhs : rhs;
	}
};

/**
 * \brief Raster operation that takes the lightest of its arguments.
 * \see grayRasterOp()
 */
template<typename Lhs, typename Rhs>
class GRopLightest
{
public:
	static uint8_t transform(uint8_t src, uint8_t dst) {
		uint8_t const lhs = Lhs::transform(src, dst);
		uint8_t const rhs = Rhs::transform(src, dst);
		return lhs > rhs ? lhs : rhs;
	}
};

template<typename GRop>
void grayRasterOp(GrayImage& dst, GrayImage const& src)
{
	if (dst.isNull() || src.isNull()) {
		throw std::invalid_argument("grayRasterOp: can't operate on null images");
	}
	
	if (src.size() != dst.size()) {
		throw std::invalid_argument("grayRasterOp: images sizes are not the same");
	}
	
	uint8_t const* src_line = src.data();
	uint8_t* dst_line = dst.data();
	int const src_stride = src.stride();
	int const dst_stride = dst.stride();
	
	int const width = src.width();
	int const height = src.height();
	
	for (int y = 0; y < height; ++y) {
		for (int x = 0; x < width; ++x) {
			dst_line[x] = GRop::transform(src_line[x], dst_line[x]);
		}
		src_line += src_stride;
		dst_line += dst_stride;
	}
}

} // namespace imageproc

#endif
