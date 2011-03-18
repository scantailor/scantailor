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

#include "RasterDewarper.h"
#include "CylindricalSurfaceDewarper.h"
#include "HomographicTransform.h"
#include "VecNT.h"
#include "imageproc/ColorMixer.h"
#include "imageproc/GrayImage.h"
#include <QtGlobal>
#include <QColor>
#include <QImage>
#include <QSize>
#include <QRect>
#include <QDebug>
#include <math.h>

#define INTERP_NONE 0
#define INTERP_BILLINEAR 1
#define INTERP_AREA_MAPPING 2
#define INTERPOLATION_METHOD INTERP_AREA_MAPPING

using namespace imageproc;

namespace dewarping
{

namespace
{

#if INTERPOLATION_METHOD == INTERP_NONE

template<typename ColorMixer, typename PixelType>
void dewarpGeneric(
	PixelType const* const src_data, QSize const src_size,
	int const src_stride, PixelType* const dst_data,
	QSize const dst_size, int const dst_stride,
	CylindricalSurfaceDewarper const& distortion_model,
	QRectF const& model_domain, PixelType const bg_color)
{
	int const src_width = src_size.width();
	int const src_height = src_size.height();
	int const dst_width = dst_size.width();
	int const dst_height = dst_size.height();

	CylindricalSurfaceDewarper::State state;

	double const model_domain_left = model_domain.left();
	double const model_x_scale = 1.0 / (model_domain.right() - model_domain.left());

	float const model_domain_top = model_domain.top();
	float const model_y_scale = 1.0 / (model_domain.bottom() - model_domain.top());

	for (int dst_x = 0; dst_x < dst_width; ++dst_x) {
		double const model_x = (dst_x - model_domain_left) * model_x_scale;
		CylindricalSurfaceDewarper::Generatrix const generatrix(
			distortion_model.mapGeneratrix(model_x, state)
		);

		HomographicTransform<1, float> const homog(generatrix.pln2img.mat());
		Vec2f const origin(generatrix.imgLine.p1());
		Vec2f const vec(generatrix.imgLine.p2() - generatrix.imgLine.p1());
		for (int dst_y = 0; dst_y < dst_height; ++dst_y) {
			float const model_y = (float(dst_y) - model_domain_top) * model_y_scale;
			Vec2f const src_pt(origin + vec * homog(model_y));
			int const src_x = qRound(src_pt[0]);
			int const src_y = qRound(src_pt[1]);
			if (src_x < 0 || src_x >= src_width || src_y < 0 || src_y >= src_height) {
				dst_data[dst_y * dst_stride + dst_x] = bg_color;
				continue;
			}

			dst_data[dst_y * dst_stride + dst_x] = src_data[src_y * src_stride + src_x];
		}
	}
}

#elif INTERPOLATION_METHOD == INTERP_BILLINEAR

template<typename ColorMixer, typename PixelType>
void dewarpGeneric(
	PixelType const* const src_data, QSize const src_size,
	int const src_stride, PixelType* const dst_data,
	QSize const dst_size, int const dst_stride,
	CylindricalSurfaceDewarper const& distortion_model,
	QRectF const& model_domain, PixelType const bg_color)
{
	int const src_width = src_size.width();
	int const src_height = src_size.height();
	int const dst_width = dst_size.width();
	int const dst_height = dst_size.height();

	CylindricalSurfaceDewarper::State state;

	double const model_domain_left = model_domain.left() - 0.5f;
	double const model_x_scale = 1.0 / (model_domain.right() - model_domain.left());

	float const model_domain_top = model_domain.top() - 0.5f;
	float const model_y_scale = 1.0 / (model_domain.bottom() - model_domain.top());

	for (int dst_x = 0; dst_x < dst_width; ++dst_x) {
		double const model_x = (dst_x - model_domain_left) * model_x_scale;
		CylindricalSurfaceDewarper::Generatrix const generatrix(
			distortion_model.mapGeneratrix(model_x, state)
		);

		HomographicTransform<1, float> const homog(generatrix.pln2img.mat());
		Vec2f const origin(generatrix.imgLine.p1());
		Vec2f const vec(generatrix.imgLine.p2() - generatrix.imgLine.p1());
		for (int dst_y = 0; dst_y < dst_height; ++dst_y) {
			float const model_y = ((float)dst_y - model_domain_top) * model_y_scale;
			Vec2f const src_pt(origin + vec * homog(model_y));

			int const src_x0 = (int)floor(src_pt[0] - 0.5f);
			int const src_y0 = (int)floor(src_pt[1] - 0.5f);
			int const src_x1 = src_x0 + 1;
			int const src_y1 = src_y0 + 1;
			float const x = src_pt[0] - src_x0;
			float const y = src_pt[1] - src_y0;

			PixelType tl_color = bg_color;
			if (src_x0 >= 0 && src_x0 < src_width && src_y0 >= 0 && src_y0 < src_height) {
				tl_color = src_data[src_y0 * src_stride + src_x0];
			}

			PixelType tr_color = bg_color;
			if (src_x1 >= 0 && src_x1 < src_width && src_y0 >= 0 && src_y0 < src_height) {
				tr_color = src_data[src_y0 * src_stride + src_x1];
			}

			PixelType bl_color = bg_color;
			if (src_x0 >= 0 && src_x0 < src_width && src_y1 >= 0 && src_y1 < src_height) {
				bl_color = src_data[src_y1 * src_stride + src_x0];
			}

			PixelType br_color = bg_color;
			if (src_x1 >= 0 && src_x1 < src_width && src_y1 >= 0 && src_y1 < src_height) {
				br_color = src_data[src_y1 * src_stride + src_x1];
			}

			ColorMixer mixer;
			mixer.add(tl_color, (1.5f - y) * (1.5f - x));
			mixer.add(tr_color, (1.5f - y) * (x - 0.5f));
			mixer.add(bl_color, (y - 0.5f) * (1.5f - x));
			mixer.add(br_color, (y - 0.5f) * (x - 0.5f));
			dst_data[dst_y * dst_stride + dst_x] = mixer.mix(1.0f);
		}
	}
}

#elif INTERPOLATION_METHOD == INTERP_AREA_MAPPING

template<typename ColorMixer, typename PixelType>
void areaMapGeneratrix(
	PixelType const* const src_data, QSize const src_size,
	int const src_stride, PixelType* p_dst,
	QSize const dst_size, int const dst_stride,
	PixelType const bg_color,
	std::vector<Vec2f> const& prev_grid_column,
	std::vector<Vec2f> const& next_grid_column)
{
	int const sw = src_size.width();
	int const sh = src_size.height();
	int const dst_height = dst_size.height();

	Vec2f const* src_left_points = &prev_grid_column[0];
	Vec2f const* src_right_points = &next_grid_column[0];

	Vec2f f_src32_quad[4];

	for (int dst_y = 0; dst_y < dst_height; ++dst_y) {
		// Take a mid-point of each edge, pre-multiply by 32,
		// write the result to f_src32_quad. 16 comes from 32*0.5
		f_src32_quad[0] = 16.0f * (src_left_points[0] + src_right_points[0]);
		f_src32_quad[1] = 16.0f * (src_right_points[0] + src_right_points[1]);
		f_src32_quad[2] = 16.0f * (src_right_points[1] + src_left_points[1]);
		f_src32_quad[3] = 16.0f * (src_left_points[0] + src_left_points[1]);
		++src_left_points;
		++src_right_points;

		// Calculate the bounding box of src_quad.

		float f_src32_left = f_src32_quad[0][0];
		float f_src32_top = f_src32_quad[0][1];
		float f_src32_right = f_src32_left;
		float f_src32_bottom = f_src32_top;

		for (int i = 1; i < 4; ++i) {
			Vec2f const pt(f_src32_quad[i]);
			if (pt[0] < f_src32_left) {
				f_src32_left = pt[0];
			} else if (pt[0] > f_src32_right) {
				f_src32_right = pt[0];
			}
			if (pt[1] < f_src32_top) {
				f_src32_top = pt[1];
			} else if (pt[1] > f_src32_bottom) {
				f_src32_bottom = pt[1];
			}
		}

		if (f_src32_top < -32.0f * 10000.0f || f_src32_left < -32.0f * 10000.0f ||
				f_src32_bottom > 32.0f * (float(sh) + 10000.f) ||
				f_src32_right > 32.0f * (float(sw) + 10000.f)) {
			// This helps to prevent integer overflows.
			*p_dst = bg_color;
			p_dst += dst_stride;
			continue;
		}

		// Note: the code below is more or less the same as in transformGeneric()
		// in imageproc/Transform.cpp

		// Note that without using floor() and ceil()
		// we can't guarantee that src_bottom >= src_top
		// and src_right >= src_left.
		int src32_left = (int)floor(f_src32_left);
		int src32_right = (int)ceil(f_src32_right);
		int src32_top = (int)floor(f_src32_top);
		int src32_bottom = (int)ceil(f_src32_bottom);
		int src_left = src32_left >> 5;
		int src_right = (src32_right - 1) >> 5; // inclusive
		int src_top = src32_top >> 5;
		int src_bottom = (src32_bottom - 1) >> 5; // inclusive
		assert(src_bottom >= src_top);
		assert(src_right >= src_left);
		
		if (src_bottom < 0 || src_right < 0 || src_left >= sw || src_top >= sh) {
			// Completely outside of src image.
			*p_dst = bg_color;
			p_dst += dst_stride;
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
		
		ColorMixer mixer;
		//if (weak_background) {
		//	background_area = 0;
		//} else {
			mixer.add(bg_color, background_area);
		//}
		
		unsigned const left_fraction = 32 - (src32_left & 31);
		unsigned const top_fraction = 32 - (src32_top & 31);
		unsigned const right_fraction = src32_right - (src_right << 5);
		unsigned const bottom_fraction = src32_bottom - (src_bottom << 5);
		
		assert(left_fraction + right_fraction + (src_right - src_left - 1) * 32 == static_cast<unsigned>(src32_right - src32_left));
		assert(top_fraction + bottom_fraction + (src_bottom - src_top - 1) * 32 == static_cast<unsigned>(src32_bottom - src32_top));
		
		unsigned const src_area = (src32_bottom - src32_top) * (src32_right - src32_left);
		if (src_area == 0) {
			*p_dst = bg_color;
			p_dst += dst_stride;
			continue;
		}
		
		PixelType const* src_line = &src_data[src_top * src_stride];
		
		if (src_top == src_bottom) {
			if (src_left == src_right) {
				// dst pixel maps to a single src pixel
				PixelType const c = src_line[src_left];
				if (background_area == 0) {
					// common case optimization
					*p_dst = c;
					p_dst += dst_stride;
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

		*p_dst = mixer.mix(src_area + background_area);
		p_dst += dst_stride;
	}
}

template<typename ColorMixer, typename PixelType>
void dewarpGeneric(
	PixelType const* const src_data, QSize const src_size,
	int const src_stride, PixelType* const dst_data,
	QSize const dst_size, int const dst_stride,
	CylindricalSurfaceDewarper const& distortion_model,
	QRectF const& model_domain, PixelType const bg_color)
{
	int const src_width = src_size.width();
	int const src_height = src_size.height();
	int const dst_width = dst_size.width();
	int const dst_height = dst_size.height();

	CylindricalSurfaceDewarper::State state;

	double const model_domain_left = model_domain.left();
	double const model_x_scale = 1.0 / (model_domain.right() - model_domain.left());

	float const model_domain_top = model_domain.top();
	float const model_y_scale = 1.0 / (model_domain.bottom() - model_domain.top());

	std::vector<Vec2f> prev_grid_column(dst_height + 1);
	std::vector<Vec2f> next_grid_column(dst_height + 1);

	for (int dst_x = 0; dst_x <= dst_width; ++dst_x) {
		double const model_x = (dst_x - model_domain_left) * model_x_scale;
		CylindricalSurfaceDewarper::Generatrix const generatrix(
			distortion_model.mapGeneratrix(model_x, state)
		);

		HomographicTransform<1, float> const homog(generatrix.pln2img.mat());
		Vec2f const origin(generatrix.imgLine.p1());
		Vec2f const vec(generatrix.imgLine.p2() - generatrix.imgLine.p1());
		for (int dst_y = 0; dst_y <= dst_height; ++dst_y) {
			float const model_y = (float(dst_y) - model_domain_top) * model_y_scale;
			next_grid_column[dst_y] = origin + vec * homog(model_y);
		}

		if (dst_x != 0) {
			areaMapGeneratrix<ColorMixer, PixelType>(
				src_data, src_size, src_stride,
				dst_data + dst_x - 1, dst_size, dst_stride,
				bg_color, prev_grid_column, next_grid_column
			);
		}

		prev_grid_column.swap(next_grid_column);
	}
}

#endif // INTERPOLATION_METHOD

#if INTERPOLATION_METHOD == INTERP_BILLINEAR
typedef float MixingWeight;
#else
typedef unsigned MixingWeight;
#endif

QImage dewarpGrayscale(
	QImage const& src, QSize const& dst_size,
	CylindricalSurfaceDewarper const& distortion_model,
	QRectF const& model_domain, QColor const& bg_color)
{
	GrayImage dst(dst_size);
	uint8_t const bg_sample = qGray(bg_color.rgb());
	dst.fill(bg_sample);
	dewarpGeneric<GrayColorMixer<MixingWeight>, uint8_t>(
		src.bits(), src.size(), src.bytesPerLine(),
		dst.data(), dst_size, dst.stride(),
		distortion_model, model_domain, bg_sample
	);
	return dst.toQImage();
}

QImage dewarpRgb(
	QImage const& src, QSize const& dst_size,
	CylindricalSurfaceDewarper const& distortion_model,
	QRectF const& model_domain, QColor const& bg_color)
{
	QImage dst(dst_size, QImage::Format_RGB32);
	dst.fill(bg_color.rgb());
	dewarpGeneric<RgbColorMixer<MixingWeight>, uint32_t>(
		(uint32_t const*)src.bits(), src.size(), src.bytesPerLine()/4,
		(uint32_t*)dst.bits(), dst_size, dst.bytesPerLine()/4,
		distortion_model, model_domain, bg_color.rgb()
	);
	return dst;
}

QImage dewarpArgb(
	QImage const& src, QSize const& dst_size,
	CylindricalSurfaceDewarper const& distortion_model,
	QRectF const& model_domain, QColor const& bg_color)
{
	QImage dst(dst_size, QImage::Format_ARGB32);
	dst.fill(bg_color.rgba());
	dewarpGeneric<ArgbColorMixer<MixingWeight>, uint32_t>(
		(uint32_t const*)src.bits(), src.size(), src.bytesPerLine()/4,
		(uint32_t*)dst.bits(), dst_size, dst.bytesPerLine()/4,
		distortion_model, model_domain, bg_color.rgba()
	);
	return dst;
}

} // anonymous namespace

QImage
RasterDewarper::dewarp(
	QImage const& src, QSize const& dst_size,
	CylindricalSurfaceDewarper const& distortion_model,
	QRectF const& model_domain, QColor const& bg_color)
{
	if (model_domain.isEmpty()) {
		throw std::invalid_argument("RasterDewarper: model_domain is empty.");
	}

	switch (src.format()) {
		case QImage::Format_Invalid:
			return QImage();
		case QImage::Format_RGB32:
			return dewarpRgb(src, dst_size, distortion_model, model_domain, bg_color);
		case QImage::Format_ARGB32:
			return dewarpArgb(src, dst_size, distortion_model, model_domain, bg_color);
		case QImage::Format_Indexed8:
			if (src.isGrayscale()) {
				return dewarpGrayscale(src, dst_size, distortion_model, model_domain, bg_color);
			} else if (src.allGray()) {
				// Only shades of gray but non-standard palette.
				return dewarpGrayscale(
					GrayImage(src).toQImage(), dst_size, distortion_model,
					model_domain, bg_color
				);
			}
			break;
		case QImage::Format_Mono:
		case QImage::Format_MonoLSB:
			if (src.allGray()) {
				return dewarpGrayscale(
					GrayImage(src).toQImage(),
					dst_size, distortion_model, model_domain, bg_color
				);
			}
			break;
		default:;
	}

	// Generic case: convert to either RGB32 or ARGB32.
	if (src.hasAlphaChannel()) {
		return dewarpArgb(
			src.convertToFormat(QImage::Format_ARGB32),
			dst_size, distortion_model, model_domain, bg_color
		);
	} else {
		return dewarpRgb(
			src.convertToFormat(QImage::Format_RGB32),
			dst_size, distortion_model, model_domain, bg_color
		);
	}
}

} // namespace dewarping
