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

#include "Scale.h"
#include "GrayImage.h"
#include <QImage>
#include <QSize>
#include <stdexcept>
#include <stdint.h>
#include <assert.h>

namespace imageproc
{

/**
 * This is an optimized implementation for the case when every destination
 * pixel maps exactly to a M x N block of source pixels.
 */
static GrayImage scaleDownIntGrayToGray(GrayImage const& src, QSize const& dst_size)
{
	int const sw = src.width();
	int const sh = src.height();
	int const dw = dst_size.width();
	int const dh = dst_size.height();
	
	int const xscale = sw / dw;
	int const yscale = sh / dh;
	int const total_area = xscale * yscale;
	
	GrayImage dst(dst_size);
	
	uint8_t const* src_line = src.data();
	uint8_t* dst_line = dst.data();
	int const src_stride = src.stride();
	int const src_stride_scaled = src_stride * yscale;
	int const dst_stride = dst.stride();
	
	int sy = 0;
	int dy = 0;
	for (; dy < dh; ++dy, sy += yscale) {
		
		int sx = 0;
		int dx = 0;
		for (; dx < dw; ++dx, sx += xscale) {
			
			unsigned gray_level = 0;
			uint8_t const* psrc = src_line + sx;
			
			for (int i = 0; i < yscale; ++i, psrc += src_stride) {
				for (int j = 0; j < xscale; ++j) {
					gray_level += psrc[j];
				}
			}
			
			unsigned const pix_value = (gray_level + (total_area >> 1)) / total_area;
			assert(pix_value < 256);
			dst_line[dx] = static_cast<uint8_t>(pix_value);
		}
		
		src_line += src_stride_scaled;
		dst_line += dst_stride;
	}
	
	return dst;
}

/**
 * This is an optimized implementation for the case when every destination
 * pixel maps to a single source pixel (possibly to a part of it).
 */
static GrayImage scaleUpIntGrayToGray(GrayImage const& src, QSize const& dst_size)
{
	int const sw = src.width();
	int const sh = src.height();
	int const dw = dst_size.width();
	int const dh = dst_size.height();
	
	int const xscale = dw / sw;
	int const yscale = dh / sh;
	
	GrayImage dst(dst_size);
	
	uint8_t const* src_line = src.data();
	uint8_t* dst_line = dst.data();
	int const src_stride = src.stride();
	int const dst_stride = dst.stride();
	int const dst_stride_scaled = dst_stride * yscale;
	
	int sy = 0;
	int dy = 0;
	for (; dy < dh; ++sy, dy += xscale) {
		int sx = 0;
		int dx = 0;
		
		for (; dx < dw; ++sx, dx += xscale) {
			uint8_t* pdst = dst_line + dx;
			
			for (int i = 0; i < yscale; ++i, pdst += dst_stride) {
				for (int j = 0; j < xscale; ++j) {
					pdst[j] = src_line[sx];
				}
			}
		}
		
		src_line += src_stride;
		dst_line += dst_stride_scaled;
	}
	
	return dst;
}

/**
 * This function is used to calculate the ratio for going
 * from \p dst to \p src multiplied by 32, so that
 * \code
 * int(ratio * (dst_limit - 1)) / 32 < src_limit - 1
 * \endcode
 */
static double calc32xRatio1(int const dst, int const src)
{
	assert(dst > 0);
	assert(src > 0);
	
	int src32 = src << 5;
	double ratio = (double)src32 / dst;
	while ((int(ratio * (dst - 1)) >> 5) + 1 >= src) {
		--src32;
		ratio = (double)src32 / dst;
	}
	
	return ratio;
}

/**
 * This is an optimized implementation for the case when
 * the destination image is larger than the source image both
 * horizontally and vertically.
 */
static GrayImage scaleUpGrayToGray(GrayImage const& src, QSize const& dst_size)
{
	int const sw = src.width();
	int const sh = src.height();
	int const dw = dst_size.width();
	int const dh = dst_size.height();
	
	double const dx2sx32 = calc32xRatio1(dw, sw);
	double const dy2sy32 = calc32xRatio1(dh, sh);
	
	GrayImage dst(dst_size);
	
	uint8_t const* const src_data = src.data();
	uint8_t* dst_line = dst.data();
	int const src_stride = src.stride();
	int const dst_stride = dst.stride();
	
	for (int dy = 0; dy < dh; ++dy, dst_line += dst_stride) {
		int const sy32 = (int)(dy * dy2sy32);
		int const sy = sy32 >> 5;
		unsigned const top_fraction = 32 - (sy32 & 31);
		unsigned const bottom_fraction = sy32 & 31;
		assert(sy + 1 < sh); // calc32xRatio1() ensures that.
		
		uint8_t const* src_line = src_data + sy * src_stride;
		
		for (int dx = 0; dx < dw; ++dx) {
			int const sx32 = (int)(dx * dx2sx32);
			int const sx = sx32 >> 5;
			unsigned const left_fraction = 32 - (sx32 & 31);
			unsigned const right_fraction = sx32 & 31;
			assert(sx + 1 < sw); // calc32xRatio1() ensures that.
			
			unsigned gray_level = 0;
			
			uint8_t const* psrc = src_line + sx;
			gray_level += *psrc * left_fraction * top_fraction;
			++psrc;
			gray_level += *psrc * right_fraction * top_fraction;
			psrc += src_stride;
			gray_level += *psrc * right_fraction * bottom_fraction;
			--psrc;
			gray_level += *psrc * left_fraction * bottom_fraction;
			
			unsigned const total_area = 32 * 32;
			unsigned const pix_value = (gray_level + (total_area >> 1)) / total_area;
			assert(pix_value < 256);
			dst_line[dx] = static_cast<uint8_t>(pix_value);
		}
	}
	
	return dst;
}

/**
 * This function is used to calculate the ratio for going
 * from \p dst to \p src multiplied by 32, so that
 * \code
 * (int(ratio * dst_limit) - 1) / 32 < src_limit
 * \endcode
 */
static double calc32xRatio2(int const dst, int const src)
{
	assert(dst > 0);
	assert(src > 0);
	
	int src32 = src << 5;
	double ratio = (double)src32 / dst;
	while ((int(ratio * dst) - 1) >> 5 >= src) {
		--src32;
		ratio = (double)src32 / dst;
	}
	
	return ratio;
}

/**
 * This is a generic implementation of the scaling algorithm.
 */
static GrayImage scaleGrayToGray(GrayImage const& src, QSize const& dst_size)
{
	int const sw = src.width();
	int const sh = src.height();
	int const dw = dst_size.width();
	int const dh = dst_size.height();
	
	// Try versions optimized for a particular case.
	if (sw == dw && sh == dh) {
		return src;
	} else if (sw % dw == 0 && sh % dh == 0) {
		return scaleDownIntGrayToGray(src, dst_size);
	} else if (dw % sw == 0 && dh % sh == 0) {
		return scaleUpIntGrayToGray(src, dst_size);
	} else if (dw > sw && dh > sh) {
		return scaleUpGrayToGray(src, dst_size);
	}
	
	double const dx2sx32 = calc32xRatio2(dw, sw);
	double const dy2sy32 = calc32xRatio2(dh, sh);
	
	GrayImage dst(dst_size);
	
	uint8_t const* const src_data = src.data();
	uint8_t* dst_line = dst.data();
	int const src_stride = src.stride();
	int const dst_stride = dst.stride();
	
	int sy32bottom = 0;
	for (int dy1 = 1; dy1 <= dh; ++dy1, dst_line += dst_stride) {
		int const sy32top = sy32bottom;
		sy32bottom = (int)(dy1 * dy2sy32);
		int const sytop = sy32top >> 5;
		int const sybottom = (sy32bottom - 1) >> 5;
		unsigned const top_fraction = 32 - (sy32top & 31);
		unsigned const bottom_fraction = sy32bottom - (sybottom << 5);
		assert(sybottom < sh); // calc32xRatio2() ensures that.
		
		unsigned const top_area = top_fraction << 5;
		unsigned const bottom_area = bottom_fraction << 5;
		
		uint8_t const* const src_line_const = src_data + sytop * src_stride;
		
		int sx32right = 0;
		for (int dx = 0; dx < dw; ++dx) {
			int const sx32left = sx32right;
			sx32right = (int)((dx + 1) * dx2sx32);
			int const sxleft = sx32left >> 5;
			int const sxright = (sx32right - 1) >> 5;
			unsigned const left_fraction = 32 - (sx32left & 31);
			unsigned const right_fraction = sx32right - (sxright << 5);
			assert(sxright < sw); // calc32xRatio2() ensures that.
			
			uint8_t const* src_line = src_line_const;
			unsigned gray_level = 0;
			
			if (sytop == sybottom) {
				if (sxleft == sxright) {
					// dst pixel maps to a single src pixel
					dst_line[dx] = src_line[sxleft];
					continue;
				} else {
					// dst pixel maps to a horizontal line of src pixels
					unsigned const vert_fraction = sy32bottom - sy32top;
					unsigned const left_area = vert_fraction * left_fraction;
					unsigned const middle_area = vert_fraction << 5;
					unsigned const right_area = vert_fraction * right_fraction;
					
					gray_level += src_line[sxleft] * left_area;
					
					for (int sx = sxleft + 1; sx < sxright; ++sx) {
						gray_level += src_line[sx] * middle_area;
					}
					
					gray_level += src_line[sxright] * right_area;
				}
			} else if (sxleft == sxright) {
				// dst pixel maps to a vertical line of src pixels
				unsigned const hor_fraction = sx32right - sx32left;
				unsigned const top_area = hor_fraction * top_fraction;
				unsigned const middle_area = hor_fraction << 5;
				unsigned const bottom_area =  hor_fraction * bottom_fraction;
				
				gray_level += src_line[sxleft] * top_area;
				
				src_line += src_stride;
				
				for (int sy = sytop + 1; sy < sybottom; ++sy) {
					gray_level += src_line[sxleft] * middle_area;
					src_line += src_stride;
				}
				
				gray_level += src_line[sxleft] * bottom_area;
			} else {
				// dst pixel maps to a block of src pixels
				unsigned const left_area = left_fraction << 5;
				unsigned const right_area = right_fraction << 5;
				unsigned const topleft_area = top_fraction * left_fraction;
				unsigned const topright_area = top_fraction * right_fraction;
				unsigned const bottomleft_area = bottom_fraction * left_fraction;
				unsigned const bottomright_area = bottom_fraction * right_fraction;
				
				// process the top-left corner
				gray_level += src_line[sxleft] * topleft_area;
				
				// process the top line (without corners)
				for (int sx = sxleft + 1; sx < sxright; ++sx) {
					gray_level += src_line[sx] * top_area;
				}
				
				// process the top-right corner
				gray_level += src_line[sxright] * topright_area;
				
				src_line += src_stride;
				
				// process middle lines
				for (int sy = sytop + 1; sy < sybottom; ++sy) {
					gray_level += src_line[sxleft] * left_area;
					
					for (int sx = sxleft + 1; sx < sxright; ++sx) {
						gray_level += src_line[sx] << (5 + 5);
					}
					
					gray_level += src_line[sxright] * right_area;
					
					src_line += src_stride;
				}
				
				// process bottom-left corner
				gray_level += src_line[sxleft] * bottomleft_area;
				
				// process the bottom line (without corners)
				for (int sx = sxleft + 1; sx < sxright; ++sx) {
					gray_level += src_line[sx] * bottom_area;
				}
				
				// process the bottom-right corner
				gray_level += src_line[sxright] * bottomright_area;
			}
			
			unsigned const total_area = (sy32bottom - sy32top) * (sx32right - sx32left);
			unsigned const pix_value = (gray_level + (total_area >> 1)) / total_area;
			assert(pix_value < 256);
			dst_line[dx] = static_cast<uint8_t>(pix_value);
		}
	}
	
	return dst;
}

GrayImage scaleToGray(GrayImage const& src, QSize const& dst_size)
{
	if (src.isNull()) {
		return src;
	}
	
	if (!dst_size.isValid()) {
		throw std::invalid_argument("scaleToGray: dst_size is invalid");
	}

	if (dst_size.isEmpty()) {
		return GrayImage();
	}
	
	return scaleGrayToGray(src, dst_size);
}

} // namespace imageproc
