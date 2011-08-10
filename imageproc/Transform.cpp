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

#include "Transform.h"
#include "Grayscale.h"
#include "GrayImage.h"
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

namespace
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

class Gray
{
public:
	Gray() : m_grayLevel(0) {}
	
	void add(uint8_t const gray_level, unsigned const area) {
		m_grayLevel += gray_level * area;
	}
	
	uint8_t result(unsigned const total_area) const {
		unsigned const half_area = total_area >> 1;
		unsigned const res = (m_grayLevel + half_area) / total_area;
		return static_cast<uint8_t>(res);
	}
private:
	unsigned m_grayLevel;
};

class RGB32
{
public:
	RGB32() : m_red(0), m_green(0), m_blue(0) {}
	
	void add(uint32_t rgb, unsigned const area) {
		m_blue += (rgb & 0xFF) * area;
		rgb >>= 8;
		m_green += (rgb & 0xFF) * area;
		rgb >>= 8;
		m_red += (rgb & 0xFF) * area;
	}
	
	uint32_t result(unsigned const total_area) const {
		unsigned const half_area = total_area >> 1;
		uint32_t rgb = 0x0000FF00;
		rgb |= (m_red + half_area) / total_area;
		rgb <<= 8;
		rgb |= (m_green + half_area) / total_area;
		rgb <<= 8;
		rgb |= (m_blue + half_area) / total_area;
		return rgb;
	}
private:
	unsigned m_red;
	unsigned m_green;
	unsigned m_blue;
};

class ARGB32
{
public:
	ARGB32() : m_alpha(0), m_red(0), m_green(0), m_blue(0) {}
	
	void add(uint32_t argb, unsigned const area) {
		m_blue += (argb & 0xFF) * area;
		argb >>= 8;
		m_green += (argb & 0xFF) * area;
		argb >>= 8;
		m_red += (argb & 0xFF) * area;
		argb >>= 8;
		m_alpha += argb * area;
	}
	
	uint32_t result(unsigned const total_area) const {
		unsigned const half_area = total_area >> 1;
		uint32_t argb = (m_alpha + half_area) / total_area;
		argb <<= 8;
		argb |= (m_red + half_area) / total_area;
		argb <<= 8;
		argb |= (m_green + half_area) / total_area;
		argb <<= 8;
		argb |= (m_blue + half_area) / total_area;
		return argb;
	}
private:
	unsigned m_alpha;
	unsigned m_red;
	unsigned m_green;
	unsigned m_blue;
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
		std::max(min32.width(), qreal(width)),
		std::max(min32.height(), qreal(height))
	);
}

template<typename StorageUnit, typename Mixer>
static void transformGeneric(
	StorageUnit const* const src_data, int const src_stride, QSize const src_size,
	StorageUnit* const dst_data, int const dst_stride, QTransform const& xform,
	QRect const& dst_rect, StorageUnit const outside_color, int const outside_flags,
	QSizeF const& min_mapping_area)
{
	int const sw = src_size.width();
	int const sh = src_size.height();
	int const dw = dst_rect.width();
	int const dh = dst_rect.height();

	StorageUnit* dst_line = dst_data;
	
	QTransform inv_xform;
	inv_xform.translate(dst_rect.x(), dst_rect.y());
	inv_xform *= xform.inverted();
	inv_xform *= QTransform().scale(32.0, 32.0);
	
	// sx32 = dx*inv_xform.m11() + dy*inv_xform.m21() + inv_xform.dx();
	// sy32 = dy*inv_xform.m22() + dx*inv_xform.m12() + inv_xform.dy();
	
	QSizeF const src32_unit_size(calcSrcUnitSize(inv_xform, min_mapping_area));
	int const src32_unit_w = std::max<int>(1, qRound(src32_unit_size.width()));
	int const src32_unit_h = std::max<int>(1, qRound(src32_unit_size.height()));
	
	for (int dy = 0; dy < dh; ++dy, dst_line += dst_stride) {
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
				if (outside_flags & OutsidePixels::COLOR) {
					dst_line[dx] = outside_color;
				} else {
					int const src_x = qBound<int>(0, (src_left + src_right) >> 1, sw - 1);
					int const src_y = qBound<int>(0, (src_top + src_bottom) >> 1, sh - 1);
					dst_line[dx] = src_data[src_y * src_stride + src_x];
				}
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
			
			Mixer mixer;
			if (outside_flags & OutsidePixels::WEAK) {
				background_area = 0;
			} else {
				mixer.add(outside_color, background_area);
			}
			
			unsigned const left_fraction = 32 - (src32_left & 31);
			unsigned const top_fraction = 32 - (src32_top & 31);
			unsigned const right_fraction = src32_right - (src_right << 5);
			unsigned const bottom_fraction = src32_bottom - (src_bottom << 5);
			
			assert(left_fraction + right_fraction + (src_right - src_left - 1) * 32 == static_cast<unsigned>(src32_right - src32_left));
			assert(top_fraction + bottom_fraction + (src_bottom - src_top - 1) * 32 == static_cast<unsigned>(src32_bottom - src32_top));
			
			unsigned const src_area = (src32_bottom - src32_top) * (src32_right - src32_left);
			if (src_area == 0) {
				if ((outside_flags & OutsidePixels::COLOR)) {
					dst_line[dx] = outside_color;
				} else {
					int const src_x = qBound<int>(0, (src_left + src_right) >> 1, sw - 1);
					int const src_y = qBound<int>(0, (src_top + src_bottom) >> 1, sh - 1);
					dst_line[dx] = src_data[src_y * src_stride + src_x];
				}
				continue;
			}
			
			StorageUnit const* src_line = &src_data[src_top * src_stride];
			
			if (src_top == src_bottom) {
				if (src_left == src_right) {
					// dst pixel maps to a single src pixel
					StorageUnit const c = src_line[src_left];
					if (background_area == 0) {
						// common case optimization
						dst_line[dx] = c;
						continue;
					}
					mixer.add(c, src_area);
				} else {
					// dst pixel maps to a horizontal line of src pixels
					unsigned const vert_fraction = src32_bottom - src32_top;
					unsigned const left_area = vert_fraction * left_fraction;
					unsigned const middle_area = vert_fraction << 5;
					unsigned const right_area = vert_fraction * right_fraction;
					
					mixer.add(src_line[src_left], left_area);
					
					for (int sx = src_left + 1; sx < src_right; ++sx) {
						mixer.add(src_line[sx], middle_area);
					}
					
					mixer.add(src_line[src_right], right_area);
				}
			} else if (src_left == src_right) {
				// dst pixel maps to a vertical line of src pixels
				unsigned const hor_fraction = src32_right - src32_left;
				unsigned const top_area = hor_fraction * top_fraction;
				unsigned const middle_area = hor_fraction << 5;
				unsigned const bottom_area =  hor_fraction * bottom_fraction;
				
				src_line += src_left;
				mixer.add(*src_line, top_area);
				
				src_line += src_stride;
				
				for (int sy = src_top + 1; sy < src_bottom; ++sy) {
					mixer.add(*src_line, middle_area);
					src_line += src_stride;
				}
				
				mixer.add(*src_line, bottom_area);
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
				mixer.add(src_line[src_left], topleft_area);
				
				// process the top line (without corners)
				for (int sx = src_left + 1; sx < src_right; ++sx) {
					mixer.add(src_line[sx], top_area);
				}
				
				// process the top-right corner
				mixer.add(src_line[src_right], topright_area);
				
				src_line += src_stride;
				
				// process middle lines
				for (int sy = src_top + 1; sy < src_bottom; ++sy) {
					mixer.add(src_line[src_left], left_area);
					
					for (int sx = src_left + 1; sx < src_right; ++sx) {
						mixer.add(src_line[sx], 32*32);
					}
					
					mixer.add(src_line[src_right], right_area);
					
					src_line += src_stride;
				}
				
				// process bottom-left corner
				mixer.add(src_line[src_left], bottomleft_area);
				
				// process the bottom line (without corners)
				for (int sx = src_left + 1; sx < src_right; ++sx) {
					mixer.add(src_line[sx], bottom_area);
				}
				
				// process the bottom-right corner
				mixer.add(src_line[src_right], bottomright_area);
			}

			dst_line[dx] = mixer.result(src_area + background_area);
		}
	}
}

} // anonymous namespace

QImage transform(
	QImage const& src, QTransform const& xform,
	QRect const& dst_rect, OutsidePixels const outside_pixels,
	QSizeF const& min_mapping_area)
{
	if (src.isNull() || dst_rect.isEmpty()) {
		return QImage();
	}
	
	if (!xform.isAffine()) {
		throw std::invalid_argument("transform: only affine transformations are supported");
	}
	
	if (!dst_rect.isValid()) {
		throw std::invalid_argument("transform: dst_rect is invalid");
	}
	
	if (src.format() == QImage::Format_Indexed8 && src.allGray()) {
		// The palette of src may be non-standard, so we create a GrayImage,
		// which is guaranteed to have a standard palette.
		GrayImage gray_src(src);
		GrayImage gray_dst(dst_rect.size());
		transformGeneric<uint8_t, Gray>(
			gray_src.data(), gray_src.stride(), src.size(),
			gray_dst.data(), gray_dst.stride(), xform, dst_rect,
			outside_pixels.grayLevel(), outside_pixels.flags(),
			min_mapping_area
		);
		return gray_dst;
	} else {
		if (src.hasAlphaChannel() || qAlpha(outside_pixels.rgba()) != 0xff) {
			QImage const src_argb32(src.convertToFormat(QImage::Format_ARGB32));
			QImage dst(dst_rect.size(), QImage::Format_ARGB32);
			transformGeneric<uint32_t, ARGB32>(
				(uint32_t const*)src_argb32.bits(), src_argb32.bytesPerLine() / 4, src_argb32.size(),
				(uint32_t*)dst.bits(), dst.bytesPerLine() / 4, xform, dst_rect,
				outside_pixels.rgba(), outside_pixels.flags(), min_mapping_area
			);
			return dst;
		} else {
			QImage const src_rgb32(src.convertToFormat(QImage::Format_RGB32));
			QImage dst(dst_rect.size(), QImage::Format_RGB32);
			transformGeneric<uint32_t, RGB32>(
				(uint32_t const*)src_rgb32.bits(), src_rgb32.bytesPerLine() / 4, src_rgb32.size(),
				(uint32_t*)dst.bits(), dst.bytesPerLine() / 4, xform, dst_rect,
				outside_pixels.rgb(), outside_pixels.flags(), min_mapping_area
			);
			return dst;
		}
	}
}

GrayImage transformToGray(
	QImage const& src, QTransform const& xform,
	QRect const& dst_rect, OutsidePixels const outside_pixels,
	QSizeF const& min_mapping_area)
{
	if (src.isNull() || dst_rect.isEmpty()) {
		return GrayImage();
	}
	
	if (!xform.isAffine()) {
		throw std::invalid_argument("transformToGray: only affine transformations are supported");
	}
	
	if (!dst_rect.isValid()) {
		throw std::invalid_argument("transformToGray: dst_rect is invalid");
	}
	
	GrayImage const gray_src(src);
	GrayImage dst(dst_rect.size());
	
	transformGeneric<uint8_t, Gray>(
		gray_src.data(), gray_src.stride(), gray_src.size(),
		dst.data(), dst.stride(), xform, dst_rect,
		outside_pixels.grayLevel(), outside_pixels.flags(),
		min_mapping_area
	);
	
	return dst;
}

} // namespace imageproc
