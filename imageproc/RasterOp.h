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

#ifndef IMAGEPROC_RASTEROP_H_
#define IMAGEPROC_RASTEROP_H_

#include "BinaryImage.h"
#include <QPoint>
#include <QRect>
#include <QSize>
#ifndef Q_MOC_RUN
#include <boost/cstdint.hpp>
#endif
#include <stdexcept>
#include <assert.h>

namespace imageproc
{

/**
 * \brief Perform pixel-wise logical operations on portions of images.
 *
 * \param dst The destination image.  Changes will be written there.
 * \param dr The rectangle within the destination image to process.
 * \param src The source image.  May be the same as the destination image.
 * \param sp The top-left corner of the rectangle within the source image
 *           to process.  The rectangle itself is assumed to be the same
 *           as the destination rectangle.
 *
 * The template argument is the operation to perform.  This is generally
 * a combination of several Rop* class templates, such as RopXor\<RopSrc, RopDst\>.
 */
template<typename Rop>
void rasterOp(BinaryImage& dst, QRect const& dr,
	BinaryImage const& src, QPoint const& sp);

/**
 * \brief Perform pixel-wise logical operations on whole images.
 *
 * \param dst The destination image.  Changes will be written there.
 * \param src The source image.  May be the same as the destination image,
 *        otherwise it must have the same dimensions.
 *
 * The template argument is the operation to perform.  This is generally
 * a combination of several Rop* class templates, such as RopXor\<RopSrc, RopDst\>.
 */
template<typename Rop>
void rasterOp(BinaryImage& dst, BinaryImage const& src);

/**
 * \brief Raster operation that takes source pixels as they are.
 * \see rasterOp()
 */
class RopSrc
{
public:
	static uint32_t transform(uint32_t src, uint32_t /*dst*/) {
		return src;
	}
};

/**
 * \brief Raster operation that takes destination pixels as they are.
 * \see rasterOp()
 */
class RopDst
{
public:
	static uint32_t transform(uint32_t /*src*/, uint32_t dst) {
		return dst;
	}
};

/**
 * \brief Raster operation that performs a logical NOT operation.
 * \see rasterOp()
 */
template<typename Arg>
class RopNot
{
public:
	static uint32_t transform(uint32_t src, uint32_t dst) {
		return ~Arg::transform(src, dst);
	}
};

/**
 * \brief Raster operation that performs a logical AND operation.
 * \see rasterOp()
 */
template<typename Arg1, typename Arg2>
class RopAnd
{
public:
	static uint32_t transform(uint32_t src, uint32_t dst) {
		return Arg1::transform(src, dst) & Arg2::transform(src, dst);
	}
};

/**
 * \brief Raster operation that performs a logical OR operation.
 * \see rasterOp()
 */
template<typename Arg1, typename Arg2>
class RopOr
{
public:
	static uint32_t transform(uint32_t src, uint32_t dst) {
		return Arg1::transform(src, dst) | Arg2::transform(src, dst);
	}
};

/**
 * \brief Raster operation that performs a logical XOR operation.
 * \see rasterOp()
 */
template<typename Arg1, typename Arg2>
class RopXor
{
public:
	static uint32_t transform(uint32_t src, uint32_t dst) {
		return Arg1::transform(src, dst) ^ Arg2::transform(src, dst);
	}
};

/**
 * \brief Raster operation that subtracts black pixels of Arg2 from Arg1.
 * \see rasterOp()
 */
template<typename Arg1, typename Arg2>
class RopSubtract
{
public:
	static uint32_t transform(uint32_t src, uint32_t dst) {
		uint32_t lhs = Arg1::transform(src, dst);
		uint32_t rhs = Arg2::transform(src, dst);
		return lhs & (lhs ^ rhs);
	}
};

/**
 * \brief Raster operation that subtracts white pixels of Arg2 from Arg1.
 * \see rasterOp()
 */
template<typename Arg1, typename Arg2>
class RopSubtractWhite
{
public:
	static uint32_t transform(uint32_t src, uint32_t dst) {
		uint32_t lhs = Arg1::transform(src, dst);
		uint32_t rhs = Arg2::transform(src, dst);
		return lhs | ~(lhs ^ rhs);
	}
};

/**
 * \brief Polymorphic interface for raster operations.
 *
 * If you want to parametrize some of your code with a raster operation,
 * one way to do it is to have Rop as a template argument.  The other,
 * and usually better way is to have this class as a non-template argument.
 */
class AbstractRasterOp
{
public:
	virtual ~AbstractRasterOp() {}
	
	/**
	 * \see rasterOp()
	 */
	virtual void operator()(
		BinaryImage& dst, QRect const& dr,
		BinaryImage const& src, QPoint const& sp) const = 0;
};

/**
 * \brief A pre-defined raster operation to be called polymorphically.
 */
template<typename Rop>
class TemplateRasterOp : public AbstractRasterOp
{
public:
	/**
	 * \see rasterOp()
	 */
	virtual void operator()(
		BinaryImage& dst, QRect const& dr,
		BinaryImage const& src, QPoint const& sp) const {
		
		rasterOp<Rop>(dst, dr, src, sp);
	}
};


namespace detail
{

template<typename Rop>
void rasterOpInDirection(
	BinaryImage& dst, QRect const& dr,
	BinaryImage const& src, QPoint const& sp, int const dy, int const dx)
{
	int const src_start_bit = sp.x() % 32;
	int const dst_start_bit = dr.x() % 32;
	int const rightmost_dst_bit = dr.right(); // == dr.x() + dr.width() - 1;
	int const rightmost_dst_word = rightmost_dst_bit / 32 - dr.x() / 32;
	uint32_t const leftmost_dst_mask = ~uint32_t(0) >> dst_start_bit;
	uint32_t const rightmost_dst_mask = ~uint32_t(0) << (31 - rightmost_dst_bit % 32);
	
	int first_dst_word;
	int last_dst_word;
	uint32_t first_dst_mask;
	uint32_t last_dst_mask;
	if (dx == 1) {
		first_dst_word = 0;
		last_dst_word = rightmost_dst_word;
		first_dst_mask = leftmost_dst_mask;
		last_dst_mask = rightmost_dst_mask;
	} else {
		assert(dx == -1);
		first_dst_word = rightmost_dst_word;
		last_dst_word = 0;
		first_dst_mask = rightmost_dst_mask;
		last_dst_mask = leftmost_dst_mask;
	}
	
	int src_span_delta;
	int dst_span_delta;
	uint32_t* dst_span;
	uint32_t const* src_span;
	if (dy == 1) {
		src_span_delta = src.wordsPerLine();
		dst_span_delta = dst.wordsPerLine();
		dst_span = dst.data() + dr.y() * dst_span_delta + dr.x() / 32;
		src_span = src.data() + sp.y() * src_span_delta + sp.x() / 32;
	} else {
		assert(dy == -1);
		src_span_delta = -src.wordsPerLine();
		dst_span_delta = -dst.wordsPerLine();
		assert(dr.bottom() == dr.y() + dr.height() - 1);
		dst_span = dst.data() - dr.bottom() * dst_span_delta + dr.x() / 32;
		src_span = src.data() - (sp.y() + dr.height() - 1)
		                        * src_span_delta + sp.x() / 32;
	}
	
	int src_word1_shift;
	int src_word2_shift;
	if (src_start_bit > dst_start_bit) {
		src_word1_shift = src_start_bit - dst_start_bit;
		src_word2_shift = 32 - src_word1_shift;
	} else if (src_start_bit < dst_start_bit) {
		src_word2_shift = dst_start_bit - src_start_bit;
		src_word1_shift = 32 - src_word2_shift;
		--src_span;
	} else {
		// Here we have a simple case of dst_x % 32 == src_x % 32.
		// Note that the rest of the code doesn't work with such
		// a case because of hardcoded widx + 1.
		if (first_dst_word == last_dst_word) {
			assert(first_dst_word == 0);
			uint32_t const mask = first_dst_mask & last_dst_mask;
			
			for (int i = dr.height(); i > 0; --i,
			     src_span += src_span_delta, dst_span += dst_span_delta) {
				uint32_t const src_word = src_span[0];
				uint32_t const dst_word = dst_span[0];
				uint32_t const new_dst_word = Rop::transform(src_word, dst_word);
				dst_span[0] = (dst_word & ~mask) | (new_dst_word & mask);
			}
		} else {
			for (int i = dr.height(); i > 0; --i,
			     src_span += src_span_delta, dst_span += dst_span_delta) {
				
				int widx = first_dst_word;
				
				// Handle the first (possibly incomplete) dst word in the line.
				uint32_t src_word = src_span[widx];
				uint32_t dst_word = dst_span[widx];
				uint32_t new_dst_word = Rop::transform(src_word, dst_word);
				dst_span[widx] = (dst_word & ~first_dst_mask) | (new_dst_word & first_dst_mask);
				
				while ((widx += dx) != last_dst_word) {
					src_word = src_span[widx];
					dst_word = dst_span[widx];
					dst_span[widx] = Rop::transform(src_word, dst_word);
				}
				
				// Handle the last (possibly incomplete) dst word in the line.
				src_word = src_span[widx];
				dst_word = dst_span[widx];
				new_dst_word = Rop::transform(src_word, dst_word);
				dst_span[widx] = (dst_word & ~last_dst_mask) | (new_dst_word & last_dst_mask);
			}
		}
		return;
	}
	
	if (first_dst_word == last_dst_word) {
		assert(first_dst_word == 0);
		uint32_t const mask = first_dst_mask & last_dst_mask;
		uint32_t const can_word1 = (~uint32_t(0) << src_word1_shift) & mask;
		uint32_t const can_word2 = (~uint32_t(0) >> src_word2_shift) & mask;
		
		for (int i = dr.height(); i > 0; --i,
		     src_span += src_span_delta, dst_span += dst_span_delta) {
			uint32_t src_word = 0;
			if (can_word1) {
				uint32_t const src_word1 = src_span[0];
				src_word |= src_word1 << src_word1_shift;
			}
			if (can_word2) {
				uint32_t const src_word2 = src_span[1];
				src_word |= src_word2 >> src_word2_shift;
			}
			uint32_t const dst_word = dst_span[0];
			uint32_t const new_dst_word = Rop::transform(src_word, dst_word);
			dst_span[0] = (dst_word & ~mask) | (new_dst_word & mask);
		}
	} else {
		uint32_t const can_first_word1 = (~uint32_t(0) << src_word1_shift) & first_dst_mask;
		uint32_t const can_first_word2 = (~uint32_t(0) >> src_word2_shift) & first_dst_mask;
		uint32_t const can_last_word1 = (~uint32_t(0) << src_word1_shift) & last_dst_mask;
		uint32_t const can_last_word2 = (~uint32_t(0) >> src_word2_shift) & last_dst_mask;
		
		for (int i = dr.height(); i > 0; --i,
		     src_span += src_span_delta, dst_span += dst_span_delta) {
			
			int widx = first_dst_word;
			
			// Handle the first (possibly incomplete) dst word in the line.
			uint32_t src_word = 0;
			if (can_first_word1) {
				uint32_t const src_word1 = src_span[widx];
				src_word |= src_word1 << src_word1_shift;
			}
			if (can_first_word2) {
				uint32_t const src_word2 = src_span[widx + 1];
				src_word |= src_word2 >> src_word2_shift;
			}
			uint32_t dst_word = dst_span[widx];
			uint32_t new_dst_word = Rop::transform(src_word, dst_word);
			new_dst_word = (dst_word & ~first_dst_mask) | (new_dst_word & first_dst_mask);
			
			while ((widx += dx) != last_dst_word) {
				uint32_t const src_word1 = src_span[widx];
				uint32_t const src_word2 = src_span[widx + 1];
				
				dst_word = dst_span[widx];
				dst_span[widx - dx] = new_dst_word;
				
				new_dst_word = Rop::transform(
					(src_word1 << src_word1_shift) |
					(src_word2 >> src_word2_shift),
					dst_word
				);
			}
			
			// Handle the last (possibly incomplete) dst word in the line.
			src_word = 0;
			if (can_last_word1) {
				uint32_t const src_word1 = src_span[widx];
				src_word |= src_word1 << src_word1_shift;
			}
			if (can_last_word2) {
				uint32_t const src_word2 = src_span[widx + 1];
				src_word |= src_word2 >> src_word2_shift;
			}
			
			dst_word = dst_span[widx];
			dst_span[widx - dx] = new_dst_word;
			
			new_dst_word = Rop::transform(src_word, dst_word);
			new_dst_word = (dst_word & ~last_dst_mask) | (new_dst_word & last_dst_mask);
			dst_span[widx] = new_dst_word;
		}
	}
}

} // namespace detail


template<typename Rop>
void rasterOp(BinaryImage& dst, QRect const& dr,
	BinaryImage const& src, QPoint const& sp)
{
	using namespace detail;
	
	if (dr.isEmpty()) {
		return;
	}
	
	if (dst.isNull() || src.isNull()) {
		throw std::invalid_argument("rasterOp: can't operate on null images");
	}
	
	if (!dst.rect().contains(dr)) {
		throw std::invalid_argument("rasterOp: raster area exceedes the dst image");
	}
	
	if (!src.rect().contains(QRect(sp, dr.size()))) {
		throw std::invalid_argument("rasterOp: raster area exceedes the src image");
	}
	
	// We need to avoid a situation where we write some output
	// and then read it as input.  This can happen if src and dst
	// are the same images.
	
	if (&dst == &src) {
		// Note that if src and dst are different objects sharing
		// the same data, dst will get a private copy when
		// dst.data() is called.
		
		if (dr.y() > sp.y()) {
			rasterOpInDirection<Rop>(dst, dr, src, sp, -1, 1);
			return;
		}
		
		if (dr.y() == sp.y() && dr.x() > sp.x()) {
			rasterOpInDirection<Rop>(dst, dr, src, sp, 1, -1);
			return;
		}
	}
	
	rasterOpInDirection<Rop>(dst, dr, src, sp, 1, 1);
}

template<typename Rop>
void rasterOp(BinaryImage& dst, BinaryImage const& src)
{
	using namespace detail;
	
	if (dst.isNull() || src.isNull()) {
		throw std::invalid_argument("rasterOp: can't operate on null images");
	}
	
	if (dst.size() != src.size()) {
		throw std::invalid_argument("rasterOp: images have different sizes");
	}
	
	rasterOpInDirection<Rop>(dst, dst.rect(), src, QPoint(0, 0), 1, 1);
}

} // namespace imageproc

#endif
