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

#ifndef IMAGEPROC_LOCAL_MIN_MAX_GENERIC_H_
#define IMAGEPROC_LOCAL_MIN_MAX_GENERIC_H_

#include <QSize>
#include <QRect>
#include <vector>
#include <algorithm>
#include <assert.h>

namespace imageproc
{

namespace detail
{

namespace local_min_max
{

template<typename T, typename MinMaxSelector>
void fillAccumulator(
	MinMaxSelector selector, int todo_before, int todo_within, int todo_after,
	T outside_values, T const* src, int const src_delta,
	T* dst, int const dst_delta)
{
	T extremum(outside_values);

	if (todo_before <= 0 && todo_within > 0) {
		extremum = *src;
	}

	while (todo_before-- > 0) {
		*dst = extremum;
		dst += dst_delta;
		src += src_delta;
	}

	while (todo_within-- > 0) {
		extremum = selector(extremum, *src);
		*dst = extremum;
		src += src_delta;
		dst += dst_delta;
	}

	if (todo_after > 0) {
		extremum = selector(extremum, outside_values);
		do {
			*dst = extremum;
			dst += dst_delta;
		} while (--todo_after > 0);
	}
}

template<typename T>
void fillWithConstant(T* from, T* to, T constant)
{
	for (++to; from != to; ++from) {
		*from = constant;
	}
}

template<typename T, typename MinMaxSelector>
void horizontalPass(
	MinMaxSelector selector, QRect const neighborhood, T const outside_values,
	T const* input, int const input_stride, QSize const input_size,
	T* output, int const output_stride)
{
	int const se_len = neighborhood.width();
	int const width = input_size.width();
	int const width_m1 = width - 1;
	int const height = input_size.height();
	int const dx1 = neighborhood.left();
	int const dx2 = neighborhood.right();

	std::vector<T> accum(se_len * 2 - 1);
	T* const accum_middle = &accum[se_len - 1];

	for (int y = 0; y < height; ++y) {
		for (int dst_segment_first = 0; dst_segment_first < width;
				dst_segment_first += se_len) {
			int const dst_segment_last = std::min(
				dst_segment_first + se_len, width
			) - 1; // inclusive
			int const src_segment_first = dst_segment_first + dx1;
			int const src_segment_last = dst_segment_last + dx2;
			int const src_segment_middle =
				(src_segment_first + src_segment_last) >> 1;

			// Fill the first half of the accumulator buffer.
			if (src_segment_first > width_m1 || src_segment_middle < 0) {
				// This half is completely outside the image.
				// Note that the branch below can't deal with such a case.
				fillWithConstant(&accum.front(), accum_middle, outside_values);
			} else {
				// after <- [to <- within <- from] <- before
				int const from = std::min(width_m1, src_segment_middle);
				int const to = std::max(0, src_segment_first);

				int const todo_before = src_segment_middle - from;
				int const todo_within = from - to + 1;
				int const todo_after = to - src_segment_first;
				int const src_delta = -1;
				int const dst_delta = -1;

				fillAccumulator(
					selector, todo_before, todo_within, todo_after, outside_values,
					input + src_segment_middle, src_delta, accum_middle, dst_delta
				);
			}

			// Fill the second half of the accumulator buffer.
			if (src_segment_last < 0 || src_segment_middle > width_m1) {
				// This half is completely outside the image.
				// Note that the branch below can't deal with such a case.
				fillWithConstant(accum_middle, &accum.back(), outside_values);
			} else {
				// before -> [from -> within -> to] -> after
				int const from = std::max(0, src_segment_middle);
				int const to = std::min(width_m1, src_segment_last);

				int const todo_before = from - src_segment_middle;
				int const todo_within = to - from + 1;
				int const todo_after = src_segment_last - to;
				int const src_delta = 1;
				int const dst_delta = 1;

				fillAccumulator(
					selector, todo_before, todo_within, todo_after, outside_values,
					input + src_segment_middle, src_delta, accum_middle, dst_delta
				);
			}

			int const offset1 = dx1 - src_segment_middle;
			int const offset2 = dx2 - src_segment_middle;
			for (int x = dst_segment_first; x <= dst_segment_last; ++x) {
				output[x] = selector(accum_middle[x + offset1], accum_middle[x + offset2]);
			}
		}

		input += input_stride;
		output += output_stride;
	}
}

template<typename T, typename MinMaxSelector>
void verticalPass(
	MinMaxSelector selector, QRect const neighborhood, T const outside_values,
	T const* input, int const input_stride, QSize const input_size,
	T* output, int const output_stride)
{
	int const se_len = neighborhood.height();
	int const width = input_size.width();
	int const height = input_size.height();
	int const height_m1 = height - 1;
	int const dy1 = neighborhood.top();
	int const dy2 = neighborhood.bottom();

	std::vector<T> accum(se_len * 2 - 1);
	T* const accum_middle = &accum[se_len - 1];

	for (int x = 0; x < width; ++x) {
		for (int dst_segment_first = 0; dst_segment_first < height;
				dst_segment_first += se_len) {
			int const dst_segment_last = std::min(
				dst_segment_first + se_len, height
			) - 1; // inclusive
			int const src_segment_first = dst_segment_first + dy1;
			int const src_segment_last = dst_segment_last + dy2;
			int const src_segment_middle =
				(src_segment_first + src_segment_last) >> 1;

			// Fill the first half of accumulator buffer.
			if (src_segment_first > height_m1 || src_segment_middle < 0) {
				// This half is completely outside the image.
				// Note that the branch below can't deal with such a case.
				fillWithConstant(&accum.front(), accum_middle, outside_values);
			} else {
				// after <- [to <- within <- from] <- before
				int const from = std::min(height_m1, src_segment_middle);
				int const to = std::max(0, src_segment_first);

				int const todo_before = src_segment_middle - from;
				int const todo_within = from - to + 1;
				int const todo_after = to - src_segment_first;
				int const src_delta = -input_stride;
				int const dst_delta = -1;

				fillAccumulator(
					selector, todo_before, todo_within, todo_after, outside_values,
					input + src_segment_middle * input_stride, src_delta,
					accum_middle, dst_delta
				);
			}

			// Fill the second half of accumulator buffer.
			if (src_segment_last < 0 || src_segment_middle > height_m1) {
				// This half is completely outside the image.
				// Note that the branch below can't deal with such a case.
				fillWithConstant(accum_middle, &accum.back(), outside_values);
			} else {
				// before -> [from -> within -> to] -> after
				int const from = std::max(0, src_segment_middle);
				int const to = std::min(height_m1, src_segment_last);

				int const todo_before = from - src_segment_middle;
				int const todo_within = to - from + 1;
				int const todo_after = src_segment_last - to;
				int const src_delta = input_stride;
				int const dst_delta = 1;

				fillAccumulator(
					selector, todo_before, todo_within, todo_after, outside_values,
					input + src_segment_middle * input_stride, src_delta,
					accum_middle, dst_delta
				);
			}

			int const offset1 = dy1 - src_segment_middle;
			int const offset2 = dy2 - src_segment_middle;
			T* p_out = output + dst_segment_first * output_stride;
			for (int y = dst_segment_first; y <= dst_segment_last; ++y) {
				*p_out = selector(accum_middle[y + offset1], accum_middle[y + offset2]);
				p_out += output_stride;
			}
		}

		++input;
		++output;
	}
}

} // namespace local_min_max

} // namespace detail


/**
 * \brief For each cell on a 2D grid, finds the minimum or the maximum value
 *        in a rectangular neighborhood.
 *
 * This can be seen as a generalized version of grayscale erode and dilate operations.
 *
 * \param selector A functor or a pointer to a free function that can be called with
 *        two arguments of type T and return the bigger or the smaller of the two.
 * \param neighborhood The rectangular neighborhood to search for maximum or minimum values.
 *        The (0, 0) point would usually be located at the center of the neighborhood
 *        rectangle, although it's not strictly required.  The neighborhood rectangle
 *        can't be empty.
 * \param outside_values Values that are assumed to be outside of the grid bounds.
 * \param input Pointer to the input buffer.
 * \param input_stride The size of a row in input buffer, in terms of the number of T objects.
 * \param input_size Dimensions of the input grid.
 * \param output Pointer to the output data.  Note that the input and output buffers
 *        may be the same.
 * \param output_stride The size of a row in the output buffer, in terms of the number of T objects.
 *        The output grid is presumed to have the same dimensions as the input grid.
 *
 * This code is an implementation of the following algorithm:\n
 * A fast algorithm for local minimum and maximum filters on rectangular and octagonal kernels,
 * Patt. Recog. Letters, 13, pp. 517-521, 1992
 *
 * A good description of this algorithm is available online at:
 * http://leptonica.com/grayscale-morphology.html#FAST-IMPLEMENTATION
 */
template<typename T, typename MinMaxSelector>
void localMinMaxGeneric(
	MinMaxSelector selector, QRect const neighborhood, T const outside_values,
	T const* input, int const input_stride, QSize const input_size,
	T* output, int const output_stride)
{
	assert(!neighborhood.isEmpty());

	if (input_size.isEmpty()) {
		return;
	}

	std::vector<T> temp(input_size.width() * input_size.height());
	int const temp_stride = input_size.width();

	detail::local_min_max::horizontalPass(
		selector, neighborhood, outside_values,
		input, input_stride, input_size,
		&temp[0], temp_stride
	);

	detail::local_min_max::verticalPass(
		selector, neighborhood, outside_values,
		&temp[0], temp_stride, input_size,
		output, output_stride
	);
}

} // namespace imageproc

#endif
