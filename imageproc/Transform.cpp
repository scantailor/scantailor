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

#include "Scale.h"
#include "Grayscale.h"
#include <QImage>
#include <QRect>
#include <QSizeF>
#include <QPointF>
#include <QPolygonF>
#include <QColor>
#include <QTransform>
#include <QtGlobal>
#include <QDebug>
#include <stdexcept>
#include <algorithm>
#include <stdint.h>
#include <math.h>
#include <assert.h>

namespace imageproc
{

struct XLess
{
	bool operator()(QPointF const& lhs, QPointF const& rhs) const {
		return lhs.x() < rhs.x();
	}
};

struct YLess
{
	bool operator()(QPointF const& lhs, QPointF const& rhs) const {
		return lhs.y() < rhs.y();
	}
};

static QSizeF calcSrcUnitSize(QTransform const& xform, QSizeF const& min)
{
	// Imagine a rectangle of (0, 0, 1, 1), except we take
	// centers of its edges instead of its vertices.
	QPolygonF dst_poly;
	dst_poly.push_back(QPointF(0.5, 0.0));
	dst_poly.push_back(QPointF(1.0, 0.5));
	dst_poly.push_back(QPointF(0.5, 1.0));
	dst_poly.push_back(QPointF(0.0, 0.5));
	
	QPolygonF src_poly(xform.map(dst_poly));
	std::sort(src_poly.begin(), src_poly.end(), XLess());
	double const width = src_poly.back().x() - src_poly.front().x();
	std::sort(src_poly.begin(), src_poly.end(), YLess());
	double const height = src_poly.back().y() - src_poly.front().y();
	
	QSizeF const min32(min * 32.0);
	return QSizeF(
		std::max(min32.width(), width),
		std::max(min32.height(), height)
	);
}

static QImage transformGrayToGray(
	QImage const& src, QTransform const& xform,
	QRect const& dst_rect, QColor const background_color,
	QSizeF const& min_mapping_area)
{
	int const sw = src.width();
	int const sh = src.height();
	int const dw = dst_rect.width();
	int const dh = dst_rect.height();
	
	QImage dst(dw, dh, QImage::Format_Indexed8);
	dst.setColorTable(src.colorTable());
	
	uint8_t const* const src_data = src.bits();
	uint8_t* dst_line = dst.bits();
	int const src_bpl = src.bytesPerLine();
	int const dst_bpl = dst.bytesPerLine();
	
	QTransform inv_xform;
	inv_xform.translate(dst_rect.x(), dst_rect.y());
	inv_xform *= xform.inverted();
	inv_xform *= QTransform().scale(32.0, 32.0);
	
	// sx32 = dx*inv_xform.m11() + dy*inv_xform.m21() + inv_xform.dx();
	// sy32 = dy*inv_xform.m22() + dx*inv_xform.m12() + inv_xform.dy();
	
	QSizeF const src32_unit_size(calcSrcUnitSize(inv_xform, min_mapping_area));
	int const src32_unit_w = std::max<int>(1, qRound(src32_unit_size.width()));
	int const src32_unit_h = std::max<int>(1, qRound(src32_unit_size.height()));
	
	unsigned const background_gray_level = qGray(background_color.rgb());
	
	for (int dy = 0; dy < dh; ++dy, dst_line += dst_bpl) {
		double const f_dy_center = dy + 0.5;
		double const f_sx32_base = f_dy_center * inv_xform.m21() + inv_xform.dx();
		double const f_sy32_base = f_dy_center * inv_xform.m22() + inv_xform.dy();
		
		for (int dx = 0; dx < dw; ++dx) {
			double const f_dx_center = dx + 0.5;
			double const f_sx32_center = f_sx32_base + f_dx_center * inv_xform.m11();
			double const f_sy32_center = f_sy32_base + f_dx_center * inv_xform.m12();
			int src32_left = (int)f_sx32_center - (src32_unit_w >> 1);
			int src32_top = (int)f_sy32_center - (src32_unit_h >> 1);
			int src32_right = src32_left + src32_unit_w;
			int src32_bottom = src32_top + src32_unit_h;
			int src_left = src32_left >> 5;
			int src_right = (src32_right - 1) >> 5; // inclusive
			int src_top = src32_top >> 5;
			int src_bottom = (src32_bottom - 1) >> 5; // inclusive
			assert(src_bottom >= src_top);
			assert(src_right >= src_left);
			
			if (src_bottom < 0 || src_right < 0 || src_left >= sw || src_top >= sh) {
				// Completely outside of src image.
				dst_line[dx] = background_gray_level;
				continue;
			}
			
			/*
			 * Note that (intval / 32) is not the same as (intval >> 5).
			 * The former rounds towards zero, while the latter rounds towards
			 * negative infinity.
			 * Likewise, (intval % 32) is not the same as (intval & 31).
			 * The following expression:
			 * top_fraction = 32 - (src32_top & 31);
			 * works correctly with both positive and negative src32_top.
			 */
			
			unsigned background_area = 0;
			
			if (src_top < 0) {
				unsigned const top_fraction = 32 - (src32_top & 31);
				unsigned const hor_fraction = src32_right - src32_left;
				background_area += top_fraction * hor_fraction;
				unsigned const full_pixels_ver = -1 - src_top;
				background_area += hor_fraction * (full_pixels_ver << 5);
				src_top = 0;
				src32_top = 0;
			}
			if (src_bottom >= sh) {
				unsigned const bottom_fraction = src32_bottom - (src_bottom << 5);
				unsigned const hor_fraction = src32_right - src32_left;
				background_area += bottom_fraction * hor_fraction;
				unsigned const full_pixels_ver = src_bottom - sh;
				background_area += hor_fraction * (full_pixels_ver << 5);
				src_bottom = sh - 1; // inclusive
				src32_bottom = sh << 5; // exclusive
			}
			if (src_left < 0) {
				unsigned const left_fraction = 32 - (src32_left & 31);
				unsigned const vert_fraction = src32_bottom - src32_top;
				background_area += left_fraction * vert_fraction;
				unsigned const full_pixels_hor = -1 - src_left;
				background_area += vert_fraction * (full_pixels_hor << 5);
				src_left = 0;
				src32_left = 0;
			}
			if (src_right >= sw) {
				unsigned const right_fraction = src32_right - (src_right << 5);
				unsigned const vert_fraction = src32_bottom - src32_top;
				background_area += right_fraction * vert_fraction;
				unsigned const full_pixels_hor = src_right - sw;
				background_area += vert_fraction * (full_pixels_hor << 5);
				src_right = sw - 1; // inclusive
				src32_right = sw << 5; // exclusive
			}
			assert(src_bottom >= src_top);
			assert(src_right >= src_left);
			
			unsigned gray_level = background_gray_level * background_area;
			
			unsigned const left_fraction = 32 - (src32_left & 31);
			unsigned const top_fraction = 32 - (src32_top & 31);
			unsigned const right_fraction = src32_right - (src_right << 5);
			unsigned const bottom_fraction = src32_bottom - (src_bottom << 5);
			
			assert(left_fraction + right_fraction + (src_right - src_left - 1) * 32 == static_cast<unsigned>(src32_right - src32_left));
			assert(top_fraction + bottom_fraction + (src_bottom - src_top - 1) * 32 == static_cast<unsigned>(src32_bottom - src32_top));
			
			unsigned const src_area = (src32_bottom - src32_top) * (src32_right - src32_left);
			
			uint8_t const* src_line = &src_data[src_top * src_bpl];
			
			if (src_top == src_bottom) {
				if (src_left == src_right) {
					// dst pixel maps to a single src pixel
					uint8_t const c = src_line[src_left];
					if (background_area == 0) {
						// common case optimization
						dst_line[dx] = c;
						continue;
					}
					gray_level += c * src_area;
				} else {
					// dst pixel maps to a horizontal line of src pixels
					unsigned const vert_fraction = src32_bottom - src32_top;
					unsigned const left_area = vert_fraction * left_fraction;
					unsigned const middle_area = vert_fraction << 5;
					unsigned const right_area = vert_fraction * right_fraction;
					
					gray_level += src_line[src_left] * left_area;
					
					for (int sx = src_left + 1; sx < src_right; ++sx) {
						gray_level += src_line[sx] * middle_area;
					}
					
					gray_level += src_line[src_right] * right_area;
				}
			} else if (src_left == src_right) {
				// dst pixel maps to a vertical line of src pixels
				unsigned const hor_fraction = src32_right - src32_left;
				unsigned const top_area = hor_fraction * top_fraction;
				unsigned const middle_area = hor_fraction << 5;
				unsigned const bottom_area =  hor_fraction * bottom_fraction;
				
				src_line += src_left;
				gray_level += *src_line * top_area;
				
				src_line += src_bpl;
				
				for (int sy = src_top + 1; sy < src_bottom; ++sy) {
					gray_level += *src_line * middle_area;
					src_line += src_bpl;
				}
				
				gray_level += *src_line * bottom_area;
			} else {
				// dst pixel maps to a block of src pixels
				unsigned const top_area = top_fraction << 5;
				unsigned const bottom_area = bottom_fraction << 5;
				unsigned const left_area = left_fraction << 5;
				unsigned const right_area = right_fraction << 5;
				unsigned const topleft_area = top_fraction * left_fraction;
				unsigned const topright_area = top_fraction * right_fraction;
				unsigned const bottomleft_area = bottom_fraction * left_fraction;
				unsigned const bottomright_area = bottom_fraction * right_fraction;
				
				// process the top-left corner
				gray_level += src_line[src_left] * topleft_area;
				
				// process the top line (without corners)
				for (int sx = src_left + 1; sx < src_right; ++sx) {
					gray_level += src_line[sx] * top_area;
				}
				
				// process the top-right corner
				gray_level += src_line[src_right] * topright_area;
				
				src_line += src_bpl;
				
				// process middle lines
				for (int sy = src_top + 1; sy < src_bottom; ++sy) {
					gray_level += src_line[src_left] * left_area;
					
					for (int sx = src_left + 1; sx < src_right; ++sx) {
						gray_level += src_line[sx] << (5 + 5);
					}
					
					gray_level += src_line[src_right] * right_area;
					
					src_line += src_bpl;
				}
				
				// process bottom-left corner
				gray_level += src_line[src_left] * bottomleft_area;
				
				// process the bottom line (without corners)
				for (int sx = src_left + 1; sx < src_right; ++sx) {
					gray_level += src_line[sx] * bottom_area;
				}
				
				// process the bottom-right corner
				gray_level += src_line[src_right] * bottomright_area;
			}
			
			unsigned const total_area = src_area + background_area;
			unsigned const pix_value = (gray_level + (total_area >> 1)) / total_area;
			assert(pix_value < 256);
			dst_line[dx] = static_cast<uint8_t>(pix_value);
		}
	}
	
	return dst;
}

QImage transformToGray(
	QImage const& src, QTransform const& xform,
	QRect const& dst_rect, QColor const& background_color,
	QSizeF const& min_mapping_area)
{
	if (src.isNull()) {
		return src;
	}
	
	if (!xform.isAffine()) {
		throw std::invalid_argument("transformToGray: only affine transformations are supported");
	}
	
	if (!dst_rect.isValid()) {
		throw std::invalid_argument("transformToGray: dst_rect is invalid");
	}
	
	return transformGrayToGray(
		toGrayscale(src), xform, dst_rect,
		background_color, min_mapping_area
	);
}

} // namespace imageproc
