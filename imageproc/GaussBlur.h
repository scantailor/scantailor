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
#ifndef Q_MOC_RUN
#include <boost/scoped_array.hpp>
#endif
#include <iterator>
#include <string.h>

namespace imageproc
{

class GrayImage;

/**
 * \brief Applies gaussian blur on a GrayImage.
 *
 * \param src The image to apply gaussian blur to.
 * \param h_sigma The standard deviation in horizontal direction.
 * \param v_sigma The standard deviation in vertical direction.
 * \return The blurred image.
 */
GrayImage gaussBlur(GrayImage const& src, float h_sigma, float v_sigma);

/**
 * \brief Applies a 2D gaussian filter on an arbitrary data grid. 
 *
 * \param size Data grid dimensions.
 * \param h_sigma The standard deviation in horizontal direction.
 * \param v_sigma The standard deviation in vertical direction.
 * \param input A random access iterator (usually a pointer)
 *        to the beginning of input data.
 * \param input_stride The distance (in terms of iterator difference)
 *        from an input grid cell to the one directly below it.
 * \param float_reader A functor to convert whatever value corresponds to *input
 *        into a float.  Consider using one of the functors from ValueConv.h
 *        The functor will be called like this:
 * \code
 * FloatReader const reader = ...;
 * float const val = reader(input[x]);
 * \endcode
 * \param output A random access iterator (usually a pointer)
 *        to the beginning of output data.  Output may point to the same
 *        memory as input.
 * \param output_stride The distance (in terms of iterator difference)
 *        from an output grid cell to the one directly below it.
 * \param float_writer A functor that takes a float value, optionally
 *        converts it into another type and updates an output item.
 *        The functor will be called like this:
 * \code
 * FloatWriter const writer = ...;
 * float const val = ...;
 * writer(output[x], val);
 * \endcode
 * Consider using boost::lambda, possible in conjunction with one of the functors
 * from ValueConv.h:
 * \code
 * using namespace boost::lambda;
 *
 * // Just copying.
 * gaussBlurGeneric(..., _1 = _2);
 *
 * // Convert to uint8_t, with rounding and clipping.
 * gaussBlurGeneric(..., _1 = bind<uint8_t>(RoundAndClipValueConv<uint8_t>(), _2);
 * \endcode
 */
template<typename SrcIt, typename DstIt, typename FloatReader, typename FloatWriter>
void gaussBlurGeneric(QSize size, float h_sigma, float v_sigma,
					  SrcIt input, int input_stride, FloatReader float_reader,
					  DstIt output, int output_stride, FloatWriter float_writer);

namespace gauss_blur_impl
{

void find_iir_constants(
	float* n_p, float *n_m, float *d_p,
	float* d_m, float *bd_p, float *bd_m, float std_dev);

template<typename Src1It, typename Src2It, typename DstIt, typename FloatWriter>
void save(int num_items, Src1It src1, Src2It src2,
		  DstIt dst, int dst_stride, FloatWriter writer)
{
	while (num_items-- != 0) {
		writer(*dst, *src1 + *src2);
		++src1;
		++src2;
		dst += dst_stride;
	}
}

class FloatToFloatWriter
{
public:
	void operator()(float& dst, float src) const { dst = src; }
};

} // namespace gauss_blur_impl

template<typename SrcIt, typename DstIt, typename FloatReader, typename FloatWriter>
void gaussBlurGeneric(QSize const size, float const h_sigma, float const v_sigma,
					  SrcIt const input, int const input_stride, FloatReader const float_reader,
					  DstIt const output, int const output_stride, FloatWriter const float_writer)
{
	if (size.isEmpty()) {
		return;
	}

	int const width = size.width();
	int const height = size.height();
	int const width_height_max = width > height ? width : height;

	boost::scoped_array<float> val_p(new float[width_height_max]);
	boost::scoped_array<float> val_m(new float[width_height_max]);
	boost::scoped_array<float> intermediate_image(new float[width * height]);
	int const intermediate_stride = width;

	// IIR parameters.
	float n_p[5], n_m[5], d_p[5], d_m[5], bd_p[5], bd_m[5];
	
	// Vertical pass.
	gauss_blur_impl::find_iir_constants(n_p, n_m, d_p, d_m, bd_p, bd_m, v_sigma);
	for (int x = 0; x < width; ++x) {
		memset(&val_p[0], 0, height * sizeof(val_p[0]));
		memset(&val_m[0], 0, height * sizeof(val_m[0]));
		
		SrcIt sp_p(input + x);
		SrcIt sp_m(sp_p + (height - 1) * input_stride);
		float* vp = &val_p[0];
		float* vm = &val_m[0] + height - 1;
		float const initial_p = float_reader(sp_p[0]);
		float const initial_m = float_reader(sp_m[0]);
		
		for (int y = 0; y < height; ++y) {
			int const terms = y < 4 ? y : 4;
			int i = 0;
			int sp_off = 0;
			for (; i <= terms; ++i, sp_off += input_stride) {
				*vp += n_p[i] * float_reader(sp_p[-sp_off]) - d_p[i] * vp[-i];
				*vm += n_m[i] * float_reader(sp_m[sp_off]) - d_m[i] * vm[i];
			}
			for (; i <= 4; ++i) {
				*vp += (n_p[i] - bd_p[i]) * initial_p;
				*vm += (n_m[i] - bd_m[i]) * initial_m;
			}
			sp_p += input_stride;
			sp_m -= input_stride;
			++vp;
			--vm;
		}
		
		gauss_blur_impl::save(
			height, &val_p[0], &val_m[0], &intermediate_image[0] + x,
			intermediate_stride, gauss_blur_impl::FloatToFloatWriter()
		);
	}
	
	// Horizontal pass.
	gauss_blur_impl::find_iir_constants(n_p, n_m, d_p, d_m, bd_p, bd_m, h_sigma);
	float const* intermediate_line = &intermediate_image[0];
	DstIt output_line(output);
	for (int y = 0; y < height; ++y) {
		memset(&val_p[0], 0, width * sizeof(val_p[0]));
		memset(&val_m[0], 0, width * sizeof(val_m[0]));
		
		float const* sp_p = intermediate_line;
		float const* sp_m = intermediate_line + width - 1;
		float* vp = &val_p[0];
		float* vm = &val_m[0] + width - 1;
		float const initial_p = sp_p[0];
		float const initial_m = sp_m[0];
		
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
		
		gauss_blur_impl::save(width, &val_p[0], &val_m[0], output_line, 1, float_writer);

		intermediate_line += intermediate_stride;
		output_line += output_stride;
	}
}

} // namespace imageproc

#endif
