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

#include "SeedFill.h"
#include "SeedFillGeneric.h"
#include "GrayImage.h"
#include <QSize>
#include <QImage>
#include <QDebug>
#include <algorithm>
#include <stdexcept>
#include <vector>
#include <stdint.h>
#include <string.h>
#include <assert.h>

namespace imageproc
{

namespace
{

inline uint32_t fillWordHorizontally(uint32_t word, uint32_t const mask)
{
	uint32_t prev_word;
	
	do {
		prev_word = word;
		word |= (word << 1) | (word >> 1);
		word &= mask;
	} while (word != prev_word);
	
	return word;
}

void seedFill4Iteration(BinaryImage& seed, BinaryImage const& mask)
{
	int const w = seed.width();
	int const h = seed.height();
	
	int const seed_wpl = seed.wordsPerLine();
	int const mask_wpl = mask.wordsPerLine();
	int const last_word_idx = (w - 1) >> 5;
	uint32_t const last_word_mask = ~uint32_t(0) << (((last_word_idx + 1) << 5) - w);
	
	uint32_t* seed_line = seed.data();
	uint32_t const* mask_line = mask.data();
	uint32_t const* prev_line = seed_line;
	
	// Top to bottom.
	for (int y = 0; y < h; ++y) {
		uint32_t prev_word = 0;
		
		// Make sure offscreen bits are 0.
		seed_line[last_word_idx] &= last_word_mask;
		
		// Left to right (except the last word).
		for (int i = 0; i <= last_word_idx; ++i) {
			uint32_t const mask = mask_line[i];
			uint32_t word = prev_word << 31;
			word |= seed_line[i] | prev_line[i];
			word &= mask;
			word = fillWordHorizontally(word, mask);
			seed_line[i] = word;
			prev_word = word;
		}
		
		// If we don't do this, prev_line[last_word_idx] on the next
		// iteration may contain garbage in the off-screen area.
		// That garbage can easily leak back.
		seed_line[last_word_idx] &= last_word_mask;
		
		prev_line = seed_line;
		seed_line += seed_wpl;
		mask_line += mask_wpl;
	}
	
	seed_line -= seed_wpl;
	mask_line -= mask_wpl;
	prev_line = seed_line;
	
	// Bottom to top.
	for (int y = h - 1; y >= 0; --y) {
		uint32_t prev_word = 0;
		
		// Make sure offscreen bits area 0.
		seed_line[last_word_idx] &= last_word_mask;
		
		// Right to left.
		for (int i = last_word_idx; i >= 0; --i) {
			uint32_t const mask = mask_line[i];
			uint32_t word = prev_word >> 31;
			word |= seed_line[i] | prev_line[i];
			word &= mask;
			word = fillWordHorizontally(word, mask);
			seed_line[i] = word;
			prev_word = word;
		}
		
		// If we don't do this, prev_line[last_word_idx] on the next
		// iteration may contain garbage in the off-screen area.
		// That garbage can easily leak back.
		// Fortunately, garbage can't spread through prev_word,
		// as only 1 bit is used from it, which can't be garbage.
		seed_line[last_word_idx] &= last_word_mask;
		
		prev_line = seed_line;
		seed_line -= seed_wpl;
		mask_line -= mask_wpl;
	}
}

void seedFill8Iteration(BinaryImage& seed, BinaryImage const& mask)
{
	int const w = seed.width();
	int const h = seed.height();
	
	int const seed_wpl = seed.wordsPerLine();
	int const mask_wpl = mask.wordsPerLine();
	int const last_word_idx = (w - 1) >> 5;
	uint32_t const last_word_mask = ~uint32_t(0) << (((last_word_idx + 1) << 5) - w);
	
	uint32_t* seed_line = seed.data();
	uint32_t const* mask_line = mask.data();
	uint32_t const* prev_line = seed_line;
	
	// Note: we start with prev_line == seed_line, but in this case
	// prev_line[i + 1] won't be clipped by its mask when we use it to
	// update seed_line[i].  The wrong value may propagate further from
	// there, so clipping we do on the anti-raster pass won't help.
	// That's why we clip the first line here.
	for (int i = 0; i <= last_word_idx; ++i) {
		seed_line[i] &= mask_line[i];
	}
	
	// Top to bottom.
	for (int y = 0; y < h; ++y) {
		uint32_t prev_word = 0;
		
		// Make sure offscreen bits area 0.
		seed_line[last_word_idx] &= last_word_mask;
		
		// Left to right (except the last word).
		int i = 0;
		for (; i < last_word_idx; ++i) {
			uint32_t const mask = mask_line[i];
			uint32_t word = prev_line[i];
			word |= (word << 1) | (word >> 1);
			word |= seed_line[i];
			word |= prev_line[i + 1] >> 31;
			word |= prev_word << 31;
			word &= mask;
			word = fillWordHorizontally(word, mask);
			seed_line[i] = word;
			prev_word = word;
		}
		
		// Last word.
		uint32_t const mask = mask_line[i] & last_word_mask;
		uint32_t word = prev_line[i];
		word |= (word << 1) | (word >> 1);
		word |= seed_line[i];
		word |= prev_word << 31;
		word &= mask;
		word = fillWordHorizontally(word, mask);
		seed_line[i] = word;
		
		prev_line = seed_line;
		seed_line += seed_wpl;
		mask_line += mask_wpl;
	}
	
	seed_line -= seed_wpl;
	mask_line -= mask_wpl;
	prev_line = seed_line;
	
	// Bottom to top.
	for (int y = h - 1; y >= 0; --y) {
		uint32_t prev_word = 0;
		
		// Make sure offscreen bits area 0.
		seed_line[last_word_idx] &= last_word_mask;
		
		// Right to left (except the last word).
		int i = last_word_idx;
		for (; i > 0; --i) {
			uint32_t const mask = mask_line[i];
			uint32_t word = prev_line[i];
			word |= (word << 1) | (word >> 1);
			word |= seed_line[i];
			word |= prev_line[i - 1] << 31;
			word |= prev_word >> 31;
			word &= mask;
			word = fillWordHorizontally(word, mask);
			seed_line[i] = word;
			prev_word = word;
		}
		
		// Last word.
		uint32_t const mask = mask_line[i];
		uint32_t word = prev_line[i];
		word |= (word << 1) | (word >> 1);
		word |= seed_line[i];
		word |= prev_word >> 31;
		word &= mask;
		word = fillWordHorizontally(word, mask);
		seed_line[i] = word;
		
		// If we don't do this, prev_line[last_word_idx] on the next
		// iteration may contain garbage in the off-screen area.
		// That garbage can easily leak back.
		// Fortunately, garbage can't spread through prev_word,
		// as only 1 bit is used from it, which can't be garbage.
		seed_line[last_word_idx] &= last_word_mask;
		
		prev_line = seed_line;
		seed_line -= seed_wpl;
		mask_line -= mask_wpl;
	}
}

inline uint8_t lightest(uint8_t lhs, uint8_t rhs)
{
	return lhs > rhs ? lhs : rhs;
}

inline uint8_t darkest(uint8_t lhs, uint8_t rhs)
{
	return lhs < rhs ? lhs : rhs;
}

inline bool darker_than(uint8_t lhs, uint8_t rhs)
{
	return lhs < rhs;
}

void seedFillGrayHorLine(uint8_t* seed, uint8_t const* mask, int const line_len)
{
	assert(line_len > 0);
	
	*seed = lightest(*seed, *mask);
	
	for (int i = 1; i < line_len; ++i) {
		++seed;
		++mask;
		*seed = lightest(*mask, darkest(*seed, seed[-1]));
	}
	
	for (int i = 1; i < line_len; ++i) {
		--seed;
		--mask;
		*seed = lightest(*mask, darkest(*seed, seed[1]));
	}
}

void seedFillGrayVertLine(
	uint8_t* seed, int const seed_stride,
	uint8_t const* mask, int const mask_stride, int const line_len)
{
	assert(line_len > 0);
	
	*seed = lightest(*seed, *mask);
	
	for (int i = 1; i < line_len; ++i) {
		seed += seed_stride;
		mask += mask_stride;
		*seed = lightest(*mask, darkest(*seed, seed[-seed_stride]));
	}
	
	for (int i = 1; i < line_len; ++i) {
		seed -= seed_stride;
		mask -= mask_stride;
		*seed = lightest(*mask, darkest(*seed, seed[seed_stride]));
	}
}

/**
 * \return non-zero if more iterations are required, zero otherwise.
 */
uint8_t seedFillGray4SlowIteration(GrayImage& seed, GrayImage const& mask)
{
	int const w = seed.width();
	int const h = seed.height();
	
	uint8_t* seed_line = seed.data();
	uint8_t const* mask_line = mask.data();
	uint8_t const* prev_line = seed_line;
	
	int const seed_stride = seed.stride();
	int const mask_stride = mask.stride();
	
	uint8_t modified = 0;
	
	// Top to bottom.
	for (int y = 0; y < h; ++y) {
		uint8_t prev_pixel = 0xff;
		
		// Left to right.
		for (int x = 0; x < w; ++x) {
			uint8_t const pixel = lightest(
				mask_line[x],
				darkest(
					prev_pixel,
					darkest(seed_line[x], prev_line[x])
				)
			);
			modified |= seed_line[x] ^ pixel;
			seed_line[x] = pixel;
			prev_pixel = pixel;
		}
		
		prev_line = seed_line;
		seed_line += seed_stride;
		mask_line += mask_stride;
	}
	
	seed_line -= seed_stride;
	mask_line -= mask_stride;
	prev_line = seed_line;
	
	// Bottom to top.
	for (int y = h - 1; y >= 0; --y) {
		uint8_t prev_pixel = 0xff;
		
		// Right to left.
		for (int x = w - 1; x >= 0; --x) {
			uint8_t const pixel = lightest(
				mask_line[x],
				darkest(
					prev_pixel,
					darkest(seed_line[x], prev_line[x])
				)
			);
			modified |= seed_line[x] ^ pixel;
			seed_line[x] = pixel;
			prev_pixel = pixel;
		}
		
		prev_line = seed_line;
		seed_line -= seed_stride;
		mask_line -= mask_stride;
	}
	
	return modified;
}

/**
 * \return non-zero if more iterations are required, zero otherwise.
 */
uint8_t seedFillGray8SlowIteration(GrayImage& seed, GrayImage const& mask)
{
	int const w = seed.width();
	int const h = seed.height();
	
	uint8_t* seed_line = seed.data();
	uint8_t const* mask_line = mask.data();
	uint8_t const* prev_line = seed_line;
	
	int const seed_stride = seed.stride();
	int const mask_stride = mask.stride();
	
	uint8_t modified = 0;
	
	// Some code below doesn't handle such cases.
	if (w == 1) {
		seedFillGrayVertLine(seed_line, seed_stride, mask_line, mask_stride, h);
		return 0;
	} else if (h == 1) {
		seedFillGrayHorLine(seed_line, mask_line, w);
		return 0;
	}
	
	// The prev_line[x + 1] below actually refers to seed_line[x + 1]
	// for the first line in raster order.  When working with seed_line[x],
	// seed_line[x + 1] would not yet be clipped by its mask.  So, we
	// have to do it now.
	for (int x = 0; x < w; ++x) {
		seed_line[x] = lightest(seed_line[x], mask_line[x]);
	}
	
	// Top to bottom.
	for (int y = 0; y < h; ++y) {
		int x = 0;
		
		// Leftmost pixel.
		uint8_t pixel = lightest(
			mask_line[x],
			darkest(
				seed_line[x],
				darkest(prev_line[x], prev_line[x + 1])
			)
		);
		modified |= seed_line[x] ^ pixel;
		seed_line[x] = pixel;
		
		// Left to right.
		while (++x < w - 1) {
			pixel = lightest(
				mask_line[x],
				darkest(
					darkest(
						darkest(seed_line[x], seed_line[x - 1]),
						darkest(prev_line[x], prev_line[x - 1])
					),
					prev_line[x + 1]
				)
			);
			modified |= seed_line[x] ^ pixel;
			seed_line[x] = pixel;
		}
		
		// Rightmost pixel.
		pixel = lightest(
			mask_line[x],
			darkest(
				darkest(seed_line[x], seed_line[x - 1]),
				darkest(prev_line[x], prev_line[x - 1])
			)
		);
		modified |= seed_line[x] ^ pixel;
		seed_line[x] = pixel;
		
		prev_line = seed_line;
		seed_line += seed_stride;
		mask_line += mask_stride;
	}
	
	seed_line -= seed_stride;
	mask_line -= mask_stride;
	prev_line = seed_line;
	
	// Bottom to top.
	for (int y = h - 1; y >= 0; --y) {
		int x = w - 1;
		
		// Rightmost pixel.
		uint8_t pixel = lightest(
			mask_line[x],
			darkest(
				seed_line[x],
				darkest(prev_line[x], prev_line[x - 1])
			)
		);
		modified |= seed_line[x] ^ pixel;
		seed_line[x] = pixel;
		
		// Right to left.
		while (--x > 0) {
			pixel = lightest(
				mask_line[x],
				darkest(
					darkest(
						darkest(seed_line[x], seed_line[x + 1]),
						darkest(prev_line[x], prev_line[x + 1])
					),
					prev_line[x - 1]
				)
			);
			modified |= seed_line[x] ^ pixel;
			seed_line[x] = pixel;
		}
		
		// Leftmost pixel.
		pixel = lightest(
			mask_line[x],
			darkest(
				darkest(seed_line[x], seed_line[x + 1]),
				darkest(prev_line[x], prev_line[x + 1])
			)
		);
		modified |= seed_line[x] ^ pixel;
		seed_line[x] = pixel;
		
		prev_line = seed_line;
		seed_line -= seed_stride;
		mask_line -= mask_stride;
	}
	
	return modified;
}

} // anonymous namespace

BinaryImage seedFill(
	BinaryImage const& seed, BinaryImage const& mask,
	Connectivity const connectivity)
{
	if (seed.size() != mask.size()) {
		throw std::invalid_argument("seedFill: seed and mask have different sizes");
	}
	
	BinaryImage prev;
	BinaryImage img(seed);
	
	do {
		prev = img;
		if (connectivity == CONN4) {
			seedFill4Iteration(img, mask);
		} else {
			seedFill8Iteration(img, mask);
		}
	} while (img != prev);
	
	return img;
}

GrayImage seedFillGray(
	GrayImage const& seed, GrayImage const& mask, Connectivity const connectivity)
{
	GrayImage result(seed);
	seedFillGrayInPlace(result, mask, connectivity);
	return result;
}

void seedFillGrayInPlace(
	GrayImage& seed, GrayImage const& mask, Connectivity const connectivity)
{
	if (seed.size() != mask.size()) {
		throw std::invalid_argument("seedFillGrayInPlace: seed and mask have different sizes");
	}

	if (seed.isNull()) {
		return;
	}
	
	seedFillGenericInPlace(
		&darkest, &lightest, connectivity,
		seed.data(), seed.stride(), seed.size(),
		mask.data(), mask.stride()
	);
}

GrayImage seedFillGraySlow(
	GrayImage const& seed, GrayImage const& mask, Connectivity const connectivity)
{
	GrayImage img(seed);
	
	if (connectivity == CONN4) {
		while (seedFillGray4SlowIteration(img, mask)) {
			// Continue until done.
		}
	} else {
		while (seedFillGray8SlowIteration(img, mask)) {
			// Continue until done.
		}
	}
	
	return img;
}

} // namespace imageproc
