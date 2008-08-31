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

#include "SeedFill.h"
#include <QSize>
#include <QImage>
#include <stdexcept>
#include <algorithm>
#include <stdint.h>

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
	int const content_wpl = (w + 31) >> 5;
	uint32_t const last_word_mask = ~uint32_t(0) << ((content_wpl << 5) - w);
	
	uint32_t* seed_line = seed.data();
	uint32_t const* mask_line = mask.data();
	uint32_t const* prev_line = seed_line;
	
	// Top to bottom.
	for (int y = 0; y < h; ++y) {
		uint32_t prev_word = 0;
		
		// Make sure offscreen bits are 0.
		seed_line[content_wpl - 1] &= last_word_mask;
		
		// Left to right.
		for (int i = 0; i < content_wpl; ++i) {
			uint32_t const mask = mask_line[i];
			uint32_t word = prev_word << 31;
			word |= seed_line[i] | prev_line[i];
			word &= mask;
			word = fillWordHorizontally(word, mask);
			seed_line[i] = word;
			prev_word = word;
		}
		
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
		seed_line[content_wpl - 1] &= last_word_mask;
		
		// Right to left.
		for (int i = content_wpl - 1; i >= 0; --i) {
			uint32_t const mask = mask_line[i];
			uint32_t word = prev_word >> 31;
			word |= seed_line[i] | prev_line[i];
			word &= mask;
			word = fillWordHorizontally(word, mask);
			seed_line[i] = word;
			prev_word = word;
		}
		
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
	int const content_wpl = (w + 31) >> 5;
	uint32_t const last_word_mask = ~uint32_t(0) << ((content_wpl << 5) - w);
	
	uint32_t* seed_line = seed.data();
	uint32_t const* mask_line = mask.data();
	uint32_t const* prev_line = seed_line;
	
	// Top to bottom.
	for (int y = 0; y < h; ++y) {
		uint32_t prev_word = 0;
		
		// Make sure offscreen bits area 0.
		seed_line[content_wpl - 1] &= last_word_mask;
		
		// Left to right.
		for (int i = 0; i < content_wpl; ++i) {
			uint32_t const mask = mask_line[i];
			uint32_t word = prev_line[i];
			word |= (word << 1) | (word >> 1);
			word |= seed_line[i] | (prev_word << 31);
			word &= mask;
			word = fillWordHorizontally(word, mask);
			seed_line[i] = word;
			prev_word = word;
		}
		
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
		seed_line[content_wpl - 1] &= last_word_mask;
		
		// Right to left.
		for (int i = content_wpl - 1; i >= 0; --i) {
			uint32_t const mask = mask_line[i];
			uint32_t word = prev_line[i];
			word |= (word << 1) | (word >> 1);
			word |= seed_line[i] | (prev_word >> 31);
			word &= mask;
			word = fillWordHorizontally(word, mask);
			seed_line[i] = word;
			prev_word = word;
		}
		
		prev_line = seed_line;
		seed_line -= seed_wpl;
		mask_line -= mask_wpl;
	}
}

void seedFillGray4Iteration(QImage& seed, QImage const& mask)
{
	int const w = seed.width();
	int const h = seed.height();
	
	int const seed_bpl = seed.bytesPerLine();
	int const mask_bpl = mask.bytesPerLine();
	
	uint8_t* seed_line = seed.bits();
	uint8_t const* mask_line = mask.bits();
	uint8_t const* prev_line = seed_line;
	
	// Top to bottom.
	for (int y = 0; y < h; ++y) {
		uint8_t prev_pixel = 0xff;
		
		// Left to right.
		for (int x = 0; x < w; ++x) {
			uint8_t const pixel = std::max(
				mask_line[x],
				std::min(
					prev_pixel,
					std::min(seed_line[x], prev_line[x])
				)
			);
			seed_line[x] = pixel;
			prev_pixel = pixel;
		}
		
		prev_line = seed_line;
		seed_line += seed_bpl;
		mask_line += mask_bpl;
	}
	
	seed_line -= seed_bpl;
	mask_line -= mask_bpl;
	prev_line = seed_line;
	
	// Bottom to top.
	for (int y = h - 1; y >= 0; --y) {
		uint8_t prev_pixel = 0xff;
		
		// Right to left.
		for (int x = w - 1; x >= 0; --x) {
			uint8_t const pixel = std::max(
				mask_line[x],
				std::min(
					prev_pixel,
					std::min(seed_line[x], prev_line[x])
				)
			);
			seed_line[x] = pixel;
			prev_pixel = pixel;
		}
		
		prev_line = seed_line;
		seed_line -= seed_bpl;
		mask_line -= mask_bpl;
	}
}

void seedFillGray8Iteration(QImage& seed, QImage const& mask)
{
	int const w = seed.width();
	int const h = seed.height();
	
	int const seed_bpl = seed.bytesPerLine();
	int const mask_bpl = mask.bytesPerLine();
	
	uint8_t* seed_line = seed.bits();
	uint8_t const* mask_line = mask.bits();
	uint8_t const* prev_line = seed_line;
	
	// Top to bottom.
	for (int y = 0; y < h; ++y) {
		int x = 0;
		
		// Leftmost pixel.
		seed_line[x] = std::max(
			mask_line[x],
			std::min(
				seed_line[x],
				std::min(prev_line[x], prev_line[x + 1])
			)
		);
		
		// Left to right.
		for (; x < w - 1; ++x) {
			seed_line[x] = std::max(
				mask_line[x],
				std::min(
					std::min(
						std::min(seed_line[x], seed_line[x - 1]),
						std::min(prev_line[x], prev_line[x - 1])
					),
					prev_line[x + 1]
				)
			);
		}
		
		// Rightmost pixel.
		seed_line[x] = std::max(
			mask_line[x],
			std::min(
				std::min(seed_line[x], seed_line[x - 1]),
				std::min(prev_line[x], prev_line[x - 1])
			)
		);
		
		prev_line = seed_line;
		seed_line += seed_bpl;
		mask_line += mask_bpl;
	}
	
	seed_line -= seed_bpl;
	mask_line -= mask_bpl;
	prev_line = seed_line;
	
	// Bottom to top.
	for (int y = h - 1; y >= 0; --y) {
		int x = w - 1;
		
		// Rightmost pixel.
		seed_line[x] = std::max(
			mask_line[x],
			std::min(
				seed_line[x],
				std::min(prev_line[x], prev_line[x - 1])
			)
		);
		
		// Right to left.
		for (; x > 0; --x) {
			seed_line[x] = std::max(
				mask_line[x],
				std::min(
					std::min(
						std::min(seed_line[x], seed_line[x + 1]),
						std::min(prev_line[x], prev_line[x + 1])
					),
					prev_line[x - 1]
				)
			);
		}
		
		// Leftmost pixel.
		seed_line[x] = std::max(
			mask_line[x],
			std::min(
				std::min(seed_line[x], seed_line[x + 1]),
				std::min(prev_line[x], prev_line[x + 1])
			)
		);
		
		prev_line = seed_line;
		seed_line -= seed_bpl;
		mask_line -= mask_bpl;
	}
}

} // anonymous namespace

BinaryImage seedFill(
	BinaryImage const& seed, BinaryImage const& mask,
	Connectivity const connectivity)
{
	if (seed.size() != mask.size()) {
		throw std::invalid_argument("inPlaceSeedFill: seed and mask have different sizes");
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

QImage seedFillGray(
	QImage const& seed, QImage const& mask, Connectivity const connectivity)
{
	if (seed.size() != mask.size()) {
		throw std::invalid_argument("seedFillGray: seed and mask have different sizes");
	}
	if (seed.format() != QImage::Format_Indexed8 || !seed.isGrayscale()) {
		throw std::invalid_argument("seedFillGray: seed is not grayscale");
	}
	if (mask.format() != QImage::Format_Indexed8 || !mask.isGrayscale()) {
		throw std::invalid_argument("seedFillGray: mask is not grayscale");
	}
	
	QImage prev;
	QImage img(seed);
	
	do {
		prev = img;
		if (connectivity == CONN4) {
			seedFillGray4Iteration(img, mask);
		} else {
			seedFillGray4Iteration(img, mask);
		}
	} while (img != prev);
	
	return img;
}

} // namespace imageproc
