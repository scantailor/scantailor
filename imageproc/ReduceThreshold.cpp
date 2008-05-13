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

#include "ReduceThreshold.h"
#include <stdexcept>
#include <stdint.h>
#include <assert.h>

namespace imageproc
{

namespace
{

/**
 * This lookup table is filled like this:
 * \code
 * for (unsigned i = 0; i < 256; i += 2) {
 *	unsigned out =
 *		((i & (1 << 1)) >> 1) |  // bit 1 becomes bit 0
 *		((i & (1 << 3)) >> 2) |  // bit 3 becomes bit 1
 *		((i & (1 << 5)) >> 3) |  // bit 5 becomes bit 2
 *		((i & (1 << 7)) >> 4);   // bit 7 becomes bit 3
 *	compressBitsLut[i >> 1] = static_cast<uint8_t>(out);
 * }
 * \endcode
 * We take every other byte because bit 0 doesn't matter here.
 */
static uint8_t const compressBitsLut[128] = {
	0x0, 0x1, 0x0, 0x1, 0x2, 0x3, 0x2, 0x3,
	0x0, 0x1, 0x0, 0x1, 0x2, 0x3, 0x2, 0x3,
	0x4, 0x5, 0x4, 0x5, 0x6, 0x7, 0x6, 0x7,
	0x4, 0x5, 0x4, 0x5, 0x6, 0x7, 0x6, 0x7,
	0x0, 0x1, 0x0, 0x1, 0x2, 0x3, 0x2, 0x3,
	0x0, 0x1, 0x0, 0x1, 0x2, 0x3, 0x2, 0x3,
	0x4, 0x5, 0x4, 0x5, 0x6, 0x7, 0x6, 0x7,
	0x4, 0x5, 0x4, 0x5, 0x6, 0x7, 0x6, 0x7,
	0x8, 0x9, 0x8, 0x9, 0xa, 0xb, 0xa, 0xb,
	0x8, 0x9, 0x8, 0x9, 0xa, 0xb, 0xa, 0xb,
	0xc, 0xd, 0xc, 0xd, 0xe, 0xf, 0xe, 0xf,
	0xc, 0xd, 0xc, 0xd, 0xe, 0xf, 0xe, 0xf,
	0x8, 0x9, 0x8, 0x9, 0xa, 0xb, 0xa, 0xb,
	0x8, 0x9, 0x8, 0x9, 0xa, 0xb, 0xa, 0xb,
	0xc, 0xd, 0xc, 0xd, 0xe, 0xf, 0xe, 0xf,
	0xc, 0xd, 0xc, 0xd, 0xe, 0xf, 0xe, 0xf
};

/**
 * Throw away every other bit starting with bit 0 and
 * pack the remaining bits into the upper half of a word.
 */
inline uint32_t compressBitsUpperHalf(uint32_t const bits)
{
	uint32_t r;
	r = compressBitsLut[(bits >> 25) /*& 0x7F*/] << 28;
	r |= compressBitsLut[(bits >> 17) & 0x7F] << 24;
	r |= compressBitsLut[(bits >> 9) & 0x7F] << 20;
	r |= compressBitsLut[(bits >> 1) & 0x7F] << 16;
	return r;
}

/**
 * Throw away every other bit starting with bit 0 and
 * pack the remaining bits into the lower half of a word.
 */
inline uint32_t compressBitsLowerHalf(uint32_t const bits)
{
	uint32_t r;
	r = compressBitsLut[(bits >> 25) /*& 0x7F*/] << 12;
	r |= compressBitsLut[(bits >> 17) & 0x7F] << 8;
	r |= compressBitsLut[(bits >> 9) & 0x7F] << 4;
	r |= compressBitsLut[(bits >> 1) & 0x7F];
	return r;
}

inline uint32_t threshold1(uint32_t const top, uint32_t const bottom)
{
	uint32_t word = top | bottom;
	word |= word << 1;
	return word;
}

inline uint32_t threshold2(uint32_t const top, uint32_t const bottom)
{
	uint32_t word1 = top & bottom;
	word1 |= word1 << 1;
	uint32_t word2 = top | bottom;
	word2 &= word2 << 1;
	return word1 | word2;
}

inline uint32_t threshold3(uint32_t const top, uint32_t const bottom)
{
	uint32_t word1 = top | bottom;
	word1 &= word1 << 1;
	uint32_t word2 = top & bottom;
	word2 |= word2 << 1;
	return word1 & word2;
}

inline uint32_t threshold4(uint32_t const top, uint32_t const bottom)
{
	uint32_t word = top & bottom;
	word &= word << 1;
	return word;
}

} // anonymous namespace


ReduceThreshold::ReduceThreshold(BinaryImage const& image)
:	m_image(image)
{
}

ReduceThreshold&
ReduceThreshold::reduce(int const threshold)
{
	if (threshold < 1 || threshold > 4) {
		throw std::invalid_argument("ReduceThreshold: invalid threshold");
	}
	
	BinaryImage const& src = m_image;
	
	if (src.isNull()) {
		return *this;
	}
	
	int const dst_w = src.width() / 2;
	int const dst_h = src.height() / 2;
	
	if (dst_h == 0) {
		reduceHorLine(threshold);
		return *this;
	} else if (dst_w == 0) {
		reduceVertLine(threshold);
		return *this;
	}
	
	BinaryImage dst(dst_w, dst_h);
	
	int const dst_wpl = dst.wordsPerLine();
	int const src_wpl = src.wordsPerLine();
	int const steps_per_line = (dst_w * 2 + 31) / 32;
	assert(steps_per_line <= src_wpl);
	assert(steps_per_line / 2 <= dst_wpl);
	
	uint32_t const* src_line = src.data();
	uint32_t* dst_line = dst.data();
	
	uint32_t word;
	
	if (threshold == 1) {
		for (int i = dst_h; i > 0; --i) {
			for (int j = 0; j < steps_per_line; j += 2) {
				word = threshold1(src_line[j], src_line[j + src_wpl]);
				dst_line[j / 2] = compressBitsUpperHalf(word);
			}
			for (int j = 1; j < steps_per_line; j += 2) {
				word = threshold1(src_line[j], src_line[j + src_wpl]);
				dst_line[j / 2] |= compressBitsLowerHalf(word);
			}
			src_line += src_wpl * 2;
			dst_line += dst_wpl;
		}
	} else if (threshold == 2) {
		for (int i = dst_h; i > 0; --i) {
			for (int j = 0; j < steps_per_line; j += 2) {
				word = threshold2(src_line[j], src_line[j + src_wpl]);
				dst_line[j / 2] = compressBitsUpperHalf(word);
			}
			for (int j = 1; j < steps_per_line; j += 2) {
				word = threshold2(src_line[j], src_line[j + src_wpl]);
				dst_line[j / 2] |= compressBitsLowerHalf(word);
			}
			src_line += src_wpl * 2;
			dst_line += dst_wpl;
		}
	} else if (threshold == 3) {
		for (int i = dst_h; i > 0; --i) {
			for (int j = 0; j < steps_per_line; j += 2) {
				word = threshold3(src_line[j], src_line[j + src_wpl]);
				dst_line[j / 2] = compressBitsUpperHalf(word);
			}
			for (int j = 1; j < steps_per_line; j += 2) {
				word = threshold3(src_line[j], src_line[j + src_wpl]);
				dst_line[j / 2] |= compressBitsLowerHalf(word);
			}
			src_line += src_wpl * 2;
			dst_line += dst_wpl;
		}
	} else if (threshold == 4) {
		for (int i = dst_h; i > 0; --i) {
			for (int j = 0; j < steps_per_line; j += 2) {
				word = threshold4(src_line[j], src_line[j + src_wpl]);
				dst_line[j / 2] = compressBitsUpperHalf(word);
			}
			for (int j = 1; j < steps_per_line; j += 2) {
				word = threshold4(src_line[j], src_line[j + src_wpl]);
				dst_line[j / 2] |= compressBitsLowerHalf(word);
			}
			src_line += src_wpl * 2;
			dst_line += dst_wpl;
		}
	}
	
	m_image = dst;
	return *this;
}

void
ReduceThreshold::reduceHorLine(int const threshold)
{
	BinaryImage const& src = m_image;
	assert(src.height() == 1);
	
	if (src.width() == 1) {
		// 1x1 image remains the same no matter the threshold.
		return;
	}
	
	BinaryImage dst(src.width() / 2, 1);
	
	int const steps_per_line = (dst.width() * 2 + 31) / 32;
	uint32_t const* src_line = src.data();
	uint32_t* dst_line = dst.data();
	assert(steps_per_line <= src.wordsPerLine());
	assert(steps_per_line / 2 <= dst.wordsPerLine());
	
	uint32_t word;
	
	switch (threshold) {
		case 1:
		case 2: {
			for (int j = 0; j < steps_per_line; j += 2) {
				word = src_line[j];
				word |= word << 1;
				dst_line[j / 2] = compressBitsUpperHalf(word);
			}
			for (int j = 1; j < steps_per_line; j += 2) {
				word = src_line[j];
				word |= word << 1;
				dst_line[j / 2] |= compressBitsLowerHalf(word);
			}
			break;
		}
		case 3:
		case 4: {
			for (int j = 0; j < steps_per_line; j += 2) {
				word = src_line[j];
				word &= word << 1;
				dst_line[j / 2] = compressBitsUpperHalf(word);
			}
			for (int j = 1; j < steps_per_line; j += 2) {
				word = src_line[j];
				word &= word << 1;
				dst_line[j / 2] |= compressBitsLowerHalf(word);
			}
			break;
		}
	}
	
	m_image = dst;
}

void
ReduceThreshold::reduceVertLine(int const threshold)
{
	BinaryImage const& src = m_image;
	assert(src.width() == 1);
	
	if (src.height() == 1) {
		// 1x1 image remains the same no matter the threshold.
		return;
	}
	
	int const dst_h = src.height() / 2;
	BinaryImage dst(1, dst_h);
	
	int const src_wpl = src.wordsPerLine();
	int const dst_wpl = dst.wordsPerLine();
	uint32_t const* src_line = src.data();
	uint32_t* dst_line = dst.data();
	
	switch (threshold) {
		case 1:
		case 2: {
			for (int i = dst_h; i > 0; --i) {
				dst_line[0] = src_line[0] | src_line[src_wpl];
				src_line += src_wpl * 2;
				dst_line += dst_wpl;
			}
			break;
		}
		case 3:
		case 4: {
			for (int i = dst_h; i > 0; --i) {
				dst_line[0] = src_line[0] & src_line[src_wpl];
				src_line += src_wpl * 2;
				dst_line += dst_wpl;
			}
			break;
		}
	}
	
	m_image = dst;
}

} // namespace imageproc
