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

#include "OrthogonalRotation.h"
#include "BinaryImage.h"
#include "BWColor.h"
#include "RasterOp.h"
#include <QRect>
#include <algorithm>
#include <stdexcept>
#include <stdint.h>

namespace imageproc
{

static inline uint32_t mask(int x)
{
	return (uint32_t(1) << 31) >> (x % 32);
}

static BinaryImage rotate0(BinaryImage const& src, QRect const& src_rect)
{
	if (src_rect == src.rect()) {
		return src;
	}
	
	BinaryImage dst(src_rect.width(), src_rect.height());
	rasterOp<RopSrc>(dst, dst.rect(), src, src_rect.topLeft());
	
	return dst;
}

static BinaryImage rotate90(BinaryImage const& src, QRect const& src_rect)
{
	int const dst_w = src_rect.height();
	int const dst_h = src_rect.width();
	BinaryImage dst(dst_w, dst_h);
	dst.fill(WHITE);
	int const src_wpl = src.wordsPerLine();
	int const dst_wpl = dst.wordsPerLine();
	uint32_t const* const src_data = src.data() + src_rect.bottom() * src_wpl;
	uint32_t* dst_line = dst.data();
	
	/*
	 *   dst
	 *  ----->
	 * ^
	 * | src
	 * |
	 */
	
	for (int dst_y = 0; dst_y < dst_h; ++dst_y) {
		int const src_x = src_rect.left() + dst_y;
		uint32_t const* src_pword = src_data + src_x / 32;
		uint32_t const src_mask = mask(src_x);
		
		for (int dst_x = 0; dst_x < dst_w; ++dst_x) {
			if (*src_pword & src_mask) {
				dst_line[dst_x / 32] |= mask(dst_x);
			}
			src_pword -= src_wpl;
		}
		
		dst_line += dst_wpl;
	}
	
	return dst;
}

static BinaryImage rotate180(BinaryImage const& src, QRect const& src_rect)
{
	int const dst_w = src_rect.width();
	int const dst_h = src_rect.height();
	BinaryImage dst(dst_w, dst_h);
	dst.fill(WHITE);
	int const src_wpl = src.wordsPerLine();
	int const dst_wpl = dst.wordsPerLine();
	uint32_t const* src_line = src.data() + src_rect.bottom() * src_wpl;
	uint32_t* dst_line = dst.data();
	
	/*
	 *  dst
	 * ----->
	 * <-----
	 *  src
	 */
	
	for (int dst_y = 0; dst_y < dst_h; ++dst_y) {
		int src_x = src_rect.right();
		for (int dst_x = 0; dst_x < dst_w; --src_x, ++dst_x) {
			if (src_line[src_x / 32] & mask(src_x)) {
				dst_line[dst_x / 32] |= mask(dst_x);
			}
		}
		
		src_line -= src_wpl;
		dst_line += dst_wpl;
	}
	
	return dst;
}

static BinaryImage rotate270(BinaryImage const& src, QRect const& src_rect)
{
	int const dst_w = src_rect.height();
	int const dst_h = src_rect.width();
	BinaryImage dst(dst_w, dst_h);
	dst.fill(WHITE);
	int const src_wpl = src.wordsPerLine();
	int const dst_wpl = dst.wordsPerLine();
	uint32_t const* const src_data = src.data() + src_rect.top() * src_wpl;
	uint32_t* dst_line = dst.data();
	
	/*
	 *  dst
	 * ----->
	 *       |
	 *   src |
	 *       v
	 */
	
	for (int dst_y = 0; dst_y < dst_h; ++dst_y) {
		int const src_x = src_rect.right() - dst_y;
		uint32_t const* src_pword = src_data + src_x / 32;
		uint32_t const src_mask = mask(src_x);
		
		for (int dst_x = 0; dst_x < dst_w; ++dst_x) {
			if (*src_pword & src_mask) {
				dst_line[dst_x / 32] |= mask(dst_x);
			}
			src_pword += src_wpl;
		}
		
		dst_line += dst_wpl;
	}
	
	return dst;
}

BinaryImage orthogonalRotation(
	BinaryImage const& src, QRect const& src_rect, int const degrees)
{
	if (src.isNull() || src_rect.isNull()) {
		return BinaryImage();
	}
	
	if (src_rect.intersected(src.rect()) != src_rect) {
		throw std::invalid_argument("orthogonalRotation: invalid src_rect");
	}
	
	switch (degrees % 360) {
	case 0:
		return rotate0(src, src_rect);
	case 90:
	case -270:
		return rotate90(src, src_rect);
	case 180:
	case -180:
		return rotate180(src, src_rect);
	case 270:
	case -90:
		return rotate270(src, src_rect);
	default:
		throw std::invalid_argument("orthogonalRotation: invalid angle");
	}
}

BinaryImage orthogonalRotation(BinaryImage const& src, int const degrees)
{
	return orthogonalRotation(src, src.rect(), degrees);
}

} // namespace imageproc
