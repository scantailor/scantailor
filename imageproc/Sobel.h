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

#ifndef IMAGEPROC_SOBEL_H_
#define IMAGEPROC_SOBEL_H_

/**
 * \file
 *
 * The Sobel operator treats a 2D grid of data points as a function f(x, y)
 * and approximates its gradient.  It computes not the gradient itself,
 * but the gradient multiplied by 8.
 */

namespace imageproc
{

/**
 * Computes approximation of the horizontal gradient component, that is
 * the partial derivative with respect to x (multiplied by 8).
 *
 * \tparam T The type used for intermediate calculations.  Must be signed.
 * \param width Horizontal size of a grid.  Zero or negative value
 *        will cause this function to return without doing anything.
 * \param height Vertical size of a grid.  Zero or negative value
 *        will cause this function to return without doing anything.
 * \param src Pointer or a random access iterator to the top-left corner
 *        of the source grid.
 * \param src_stride The distance from a point on the source grid to the
 *        point directly below it, in terms of iterator difference.
 * \param src_reader A functor that gets passed a dereferenced iterator
 *        to the source grid and returns some type convertible to T.
 *        It's called like this:
 *        \code
 *        SrcIt src_it = ...;
 *        T const var(src_reader(*src_it));
 *        \endcode
 *        Consider using boost::lambda for constructing such a functor,
 *        possibly combined with one of the functors from ValueConf.h
 * \param tmp Pointer or a random access iterator to the top-left corner
 *        of the temporary grid.  The temporary grid will have the same
 *        width and height as the source and destination grids.
 *        Having the destination grid also serve as a temporary grid
 *        is supported, provided it's able to store signed values.
 *        Having all 3 to be the same is supported as well, subject
 *        to the same condition.
 * \param tmp_stride The distance from a point on the temporary grid to the
 *        point directly below it, in terms of iterator difference.
 * \param tmp_writer A functor that writes a value to the temporary grid.
 *        It's called like this:
 *        \code
 *        TmpIt tmp_it = ...;
 *        T val = ...;
 *        tmp_writer(*tmp_it, val);
 *        \endcode
 * \param tmp_reader A functor that gets passed a dereferenced iterator
 *        to the temporary grid and returns some type convertible to T.
 *        See \p src_reader for more info.
 * \param dst Pointer or a random access iterator to the top-left corner
 *        of the destination grid.
 * \param dst_stride The distance from a point on the destination grid to the
 *        point directly below it, in terms of iterator difference.
 * \param dst_writer A functor that writes a value to the destination grid.
 *        See \p tmp_writer for more info.
 */
template<
	typename T, typename SrcIt, typename TmpIt, typename DstIt,
	typename SrcReader, typename TmpWriter, typename TmpReader, typename DstWriter
>
void horizontalSobel(
	int width, int height, SrcIt src, int src_stride, SrcReader src_reader,
	TmpIt tmp, int tmp_stride, TmpWriter tmp_writer, TmpReader tmp_reader,
	DstIt dst, int dst_stride, DstWriter dst_writer);


/**
 * \see horizontalSobel()
 */
template<
	typename T, typename SrcIt, typename TmpIt, typename DstIt,
	typename SrcReader, typename TmpWriter,	typename TmpReader, typename DstWriter
>
void verticalSobel(
	int width, int height, SrcIt src, int src_stride, SrcReader src_reader,
	TmpIt tmp, int tmp_stride, TmpWriter tmp_writer, TmpReader tmp_reader,
	DstIt dst, int dst_stride, DstWriter dst_writer);


template<
	typename T, typename SrcIt, typename TmpIt, typename DstIt,
	typename SrcReader, typename TmpWriter, typename TmpReader, typename DstWriter
>
void horizontalSobel(
	int const width, int const height, SrcIt src, int src_stride, SrcReader src_reader,
	TmpIt tmp, int const tmp_stride, TmpWriter tmp_writer, TmpReader tmp_reader,
	DstIt dst, int const dst_stride, DstWriter dst_writer)
{
	if (width <= 0 || height <= 0) {
		return;
	}

	// Vertical pre-accumulation pass: mid = top + mid*2 + bottom
	for (int x = 0; x < width; ++x) {
		SrcIt p_src(src + x);
		TmpIt p_tmp(tmp + x);
		
		T top(src_reader(*p_src));
		if (height == 1) {
			tmp_writer(*p_tmp, top + top + top + top);
			continue;
		}

		T mid(src_reader(p_src[src_stride]));
		tmp_writer(*p_tmp, top + top + top + mid);
		
		for (int y = 1; y < height - 1; ++y) {
			p_src += src_stride;
			p_tmp += tmp_stride;
			T const bottom(src_reader(p_src[src_stride]));
			tmp_writer(*p_tmp, top + mid + mid + bottom);
			top = mid;
			mid = bottom;
		}

		p_src += src_stride;
		p_tmp += tmp_stride;
		tmp_writer(*p_tmp, top + mid + mid + mid);
	}

	// Horizontal pass: mid = right - left
	for (int y = 0; y < height; ++y) {
		T left(tmp_reader(*tmp));
		
		if (width == 1) {
			dst_writer(*dst, left - left);
		} else {
			T mid(tmp_reader(tmp[1]));
			dst_writer(dst[0], mid - left);
			
			int x = 1;
			for (; x < width - 1; ++x) {
				T const right(tmp_reader(tmp[x + 1]));
				dst_writer(dst[x], right - left);
				left = mid;
				mid = right;
			}

			dst_writer(dst[x], mid - left);
		}

		tmp += tmp_stride;
		dst += dst_stride;
	}
}

template<
	typename T, typename SrcIt, typename TmpIt, typename DstIt,
	typename SrcReader, typename TmpWriter, typename TmpReader, typename DstWriter
>
void verticalSobel(
	int const width, int const height, SrcIt src, int src_stride, SrcReader src_reader,
	TmpIt tmp, int const tmp_stride, TmpWriter tmp_writer, TmpReader tmp_reader,
	DstIt dst, int const dst_stride, DstWriter dst_writer)
{
	if (width <= 0 || height <= 0) {
		return;
	}

	TmpIt const tmp_orig(tmp);

	// Horizontal pre-accumulation pass: mid = left + mid*2 + right
	for (int y = 0; y < height; ++y) {
		T left(src_reader(*src));

		if (width == 1) {
			tmp_writer(*tmp, left + left + left + left);
		} else {
			T mid(src_reader(src[1]));
			tmp_writer(tmp[0], left + left + left + mid);
			
			int x = 1;
			for (; x < width - 1; ++x) {
				T const right(src_reader(src[x + 1]));
				tmp_writer(tmp[x], left + mid + mid + right);
				left = mid;
				mid = right;
			}

			tmp_writer(tmp[x], left + mid + mid + mid);
		}
		src += src_stride;
		tmp += tmp_stride;
	}

	// Vertical pass: mid = bottom - top
	for (int x = 0; x < width; ++x) {
		TmpIt p_tmp(tmp_orig + x);
		TmpIt p_dst(dst + x);

		T top(tmp_reader(*p_tmp));
		if (height == 1) {
			dst_writer(*p_dst, top - top);
			continue;
		}

		T mid(tmp_reader(p_tmp[tmp_stride]));
		dst_writer(*p_dst, mid - top);
		
		for (int y = 1; y < height - 1; ++y) {
			p_tmp += tmp_stride;
			p_dst += dst_stride;
			T const bottom(tmp_reader(p_tmp[tmp_stride]));
			dst_writer(*p_dst, bottom - top);
			top = mid;
			mid = bottom;
		}

		p_tmp += tmp_stride;
		p_dst += dst_stride;
		dst_writer(*p_dst, mid - top);
	}
}

} // namespace imageproc

#endif
