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
#include <stdexcept>

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
	
	// Top from bottom.
	for (int y = 0; y < h; ++y, seed_line += seed_wpl, mask_line += mask_wpl) {
		uint32_t const* north_line = (y == 0) ? seed_line : seed_line - seed_wpl;
		uint32_t west_word = 0;
		
		// Make sure offscreen bits area 0.
		seed_line[content_wpl - 1] &= last_word_mask;
		
		// Left to right.
		for (int i = 0; i < content_wpl; ++i) {
			uint32_t const mask = mask_line[i];
			uint32_t word = west_word << 31;
			word |= seed_line[i] | north_line[i];
			word &= mask;
			word = fillWordHorizontally(word, mask);
			seed_line[i] = word;
			west_word = word;
		}
		
		// Note that we might have set some offscreen bits to 1.
	}
	
	// Bottom from top.
	for (int y = h - 1; y >= 0; --y) {
		seed_line -= seed_wpl;
		mask_line -= mask_wpl;
		
		uint32_t const* south_line = (y == h - 1) ? seed_line : seed_line + seed_wpl;
		uint32_t east_word = 0;
		
		// Make sure offscreen bits area 0.
		seed_line[content_wpl - 1] &= last_word_mask;
		
		// Right to left.
		for (int i = content_wpl - 1; i >= 0; --i) {
			uint32_t const mask = mask_line[i];
			uint32_t word = east_word >> 31;
			word |= seed_line[i] | south_line[i];
			word &= mask;
			word = fillWordHorizontally(word, mask);
			seed_line[i] = word;
			east_word = word;
		}
		
		// Note that we might have set some offscreen bits to 1.
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
	
	// Top to bottom.
	for (int y = 0; y < h; ++y, seed_line += seed_wpl, mask_line += mask_wpl) {
		uint32_t const* north_line = (y == 0) ? seed_line : seed_line - seed_wpl;
		uint32_t west_word = 0;
		
		// Make sure offscreen bits area 0.
		seed_line[content_wpl - 1] &= last_word_mask;
		
		// Left to right.
		for (int i = 0; i < content_wpl; ++i) {
			uint32_t const mask = mask_line[i];
			uint32_t word = north_line[i];
			word |= (word << 1) | (word >> 1);
			word |= seed_line[i] | (west_word << 31);
			word &= mask;
			word = fillWordHorizontally(word, mask);
			seed_line[i] = word;
			west_word = word;
		}
		
		// Note that we might have set some offscreen bits to 1.
	}
	
	// Bottom to top.
	for (int y = h - 1; y >= 0; --y) {
		seed_line -= seed_wpl;
		mask_line -= mask_wpl;
		
		uint32_t const* south_line = (y == h - 1) ? seed_line : seed_line + seed_wpl;
		uint32_t east_word = 0;
		
		// Make sure offscreen bits area 0.
		seed_line[content_wpl - 1] &= last_word_mask;
		
		// Right to left.
		for (int i = content_wpl - 1; i >= 0; --i) {
			uint32_t const mask = mask_line[i];
			uint32_t word = south_line[i];
			word |= (word << 1) | (word >> 1);
			word |= seed_line[i] | (east_word >> 31);
			word &= mask;
			word = fillWordHorizontally(word, mask);
			seed_line[i] = word;
			east_word = word;
		}
		
		// Note that we might have set some offscreen bits to 1.
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

} // namespace imageproc
