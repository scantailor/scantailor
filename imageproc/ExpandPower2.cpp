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

#include "ExpandPower2.h"
#include "BinaryImage.h"
#include <QSize>
#include <QRect>
#include <stdexcept>
#include <stdint.h>
#include <string.h>

namespace imageproc
{

namespace
{

inline uint32_t multiplyBit(uint32_t bit, int times)
{
	return (uint32_t(0) - bit) >> (32 - times);
}

void expandImpl(BinaryImage& dst, BinaryImage const& src, int const power)
{
	int const sw = src.width();
	int const sh = src.height();
	
	int const src_wpl = src.wordsPerLine();
	int const dst_wpl = dst.wordsPerLine();
	
	uint32_t const* src_line = src.data();
	uint32_t* dst_line = dst.data();
	
	for (int sy = 0; sy < sh; ++sy, src_line += src_wpl) {
		
		uint32_t dst_word = 0;
		int dst_bits_remaining = 32;
		int di = 0;
		
		for (int sx = 0; sx < sw; ++sx) {
			uint32_t const src_word = src_line[sx >> 5];
			int const src_bit = 31 - (sx & 31);
			uint32_t const bit = (src_word >> src_bit) & uint32_t(1);
			int todo = power;
			
			while (dst_bits_remaining <= todo) {
				dst_word |= multiplyBit(bit, dst_bits_remaining);
				dst_line[di++] = dst_word;
				todo -= dst_bits_remaining;
				dst_bits_remaining = 32;
				dst_word = 0;
			}
			if (todo > 0) {
				dst_bits_remaining -= todo;
				dst_word |= multiplyBit(bit, todo) << dst_bits_remaining;
			}
		}
		
		if (dst_bits_remaining != 32) {
			dst_line[di] = dst_word;
		}
		
		uint32_t const* first_dst_line = dst_line;
		dst_line += dst_wpl;
		for (int line = 1; line < power; ++line, dst_line += dst_wpl) {
			memcpy(dst_line, first_dst_line, dst_wpl * 4);
		}
	}
}

} // anonymous namespace

BinaryImage expandPower2(BinaryImage const& src, int const exponent)
{
	if (src.isNull() || exponent == 0) {
		return src;
	}
	
	if (exponent < 0) {
		throw std::runtime_error("expandPower2: exponent can't be negative");
	}
	
	BinaryImage dst(src.width() << exponent, src.height() << exponent);
	expandImpl(dst, src, 1 << exponent);
	
	return dst;
}

BinaryImage expandPower2(
	BinaryImage const& src, QSize const& dst_size, BWColor const bgcolor)
{
	if (src.isNull()) {
		BinaryImage dst(dst_size);
		dst.fill(bgcolor);
		return dst;
	}
	
	int const factor = dst_size.width() / src.width();
	if (factor != dst_size.height() / src.height()) {
		throw std::invalid_argument("expandPower2: bad dst_size aspect");
	}
	
	BinaryImage dst(dst_size);
	expandImpl(dst, src, factor);
	QRect const rect(0, 0, src.width() * factor, src.height() * factor);
	dst.fillExcept(rect, bgcolor);
	
	return dst;
}

} // namespace imageproc
