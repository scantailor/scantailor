/*
    Scan Tailor - Interactive post-processing tool for scanned pages.
    Copyright (C)  Joseph Artsimovich <joseph.artsimovich@gmail.com>

	Based on code from the GIMP project,
    Copyright (C) 1995 Spencer Kimball and Peter Mattis

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

#ifndef IMAGEPROC_GAUSSBLUR_H_
#define IMAGEPROC_GAUSSBLUR_H_

#include "ValueConv.h"
#include <QSize>
#include <boost/scoped_array.hpp>
#include <string.h>

namespace imageproc
{

class GrayImage;

GrayImage gaussBlur(GrayImage const& src, float h_sigma, float v_sigma);

template<typename T, typename OutputConverter>
void gaussBlurGeneric(OutputConverter out_conv, T* data, int stride,
					  QSize size, float h_sigma, float v_sigma);

namespace gauss_blur_impl
{

void find_iir_constants(
	float* n_p, float *n_m, float *d_p,
	float* d_m, float *bd_p, float *bd_m, float std_dev);

template<typename DstType, typename ConvToDst>
void save(int num_items, float const* src1, float const* src2,
		  DstType* dst, int dst_stride, ConvToDst dst_conv)
{
	while (num_items-- != 0) {
		*dst = dst_conv(*src1 + *src2);
		++src1;
		++src2;
		dst += dst_stride;
	}
}

} // namespace gauss_blur_impl

template<typename T, typename OutputConverter>
void gaussBlurGeneric(OutputConverter out_conv, T* data, int data_stride,
					  QSize size, float h_sigma, float v_sigma)
{
	if (size.isEmpty()) {
		return;
	}

	int const width = size.width();
	int const height = size.height();
	
	boost::scoped_array<float> val_p(new float[std::max(width, height)]);
	boost::scoped_array<float> val_m(new float[std::max(width, height)]);
	boost::scoped_array<float> intermediate_image(new float[width * height]);
	int const intermediate_stride = width;

	// IIR parameters.
	float n_p[5], n_m[5], d_p[5], d_m[5], bd_p[5], bd_m[5];
	
	// Vertical pass.
	gauss_blur_impl::find_iir_constants(n_p, n_m, d_p, d_m, bd_p, bd_m, v_sigma);
	for (int x = 0; x < width; ++x) {
		memset(&val_p[0], 0, height * sizeof(val_p[0]));
		memset(&val_m[0], 0, height * sizeof(val_m[0]));
		
		T const* sp_p = data + x;
		T const* sp_m = sp_p + (height - 1) * data_stride;
		float* vp = &val_p[0];
		float* vm = &val_m[0] + height - 1;
		T const initial_p(sp_p[0]);
		T const initial_m(sp_m[0]);
		
		for (int y = 0; y < height; ++y) {
			int const terms = y < 4 ? y : 4;
			int i = 0;
			int sp_off = 0;
			for (; i <= terms; ++i, sp_off += data_stride) {
				*vp += n_p[i] * sp_p[-sp_off] - d_p[i] * vp[-i];
				*vm += n_m[i] * sp_m[sp_off] - d_m[i] * vm[i];
			}
			for (; i <= 4; ++i) {
				*vp += (n_p[i] - bd_p[i]) * initial_p;
				*vm += (n_m[i] - bd_m[i]) * initial_m;
			}
			sp_p += data_stride;
			sp_m -= data_stride;
			++vp;
			--vm;
		}
		
		gauss_blur_impl::save(
			height, &val_p[0], &val_m[0], &intermediate_image[0] + x,
			intermediate_stride, StaticCastValueConv<float>()
		);
	}
	
	// Horizontal pass.
	gauss_blur_impl::find_iir_constants(n_p, n_m, d_p, d_m, bd_p, bd_m, h_sigma);
	float const* intermediate_line = &intermediate_image[0];
	T* data_line = data;
	for (int y = 0; y < height; ++y) {
		memset(&val_p[0], 0, width * sizeof(val_p[0]));
		memset(&val_m[0], 0, width * sizeof(val_m[0]));
		
		float const* sp_p = intermediate_line;
		float const* sp_m = intermediate_line + width - 1;
		float* vp = &val_p[0];
		float* vm = &val_m[0] + width - 1;
		unsigned char const initial_p = sp_p[0];
		unsigned char const initial_m = sp_m[0];
		
		for (int x = 0; x < width; ++x) {
			int const terms = x < 4 ? x : 4;
			int i = 0;
			for (; i <= terms; ++i) {
				*vp += n_p[i] * sp_p[-i] - d_p[i] * vp[-i];
				*vm += n_m[i] * sp_m[i] - d_m[i] * vm[i];
			}
			for (; i <= 4; ++i) {
				*vp += (n_p[i] - bd_p[i]) * initial_p;
				*vm += (n_m[i] - bd_m[i]) * initial_m;
			}
			++sp_p;
			--sp_m;
			++vp;
			--vm;
		}
		
		gauss_blur_impl::save(width, &val_p[0], &val_m[0], data_line, 1, out_conv);

		intermediate_line += intermediate_stride;
		data_line += data_stride;
	}
}

} // namespace imageproc

#endif
