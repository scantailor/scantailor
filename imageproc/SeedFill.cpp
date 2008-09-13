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
#include <QDebug>
#include <stdexcept>
#include <queue>
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
	uint8_t* seed, int const seed_bpl,
	uint8_t const* mask, int const mask_bpl, int const line_len)
{
	assert(line_len > 0);
	
	*seed = lightest(*seed, *mask);
	
	for (int i = 1; i < line_len; ++i) {
		seed += seed_bpl;
		mask += mask_bpl;
		*seed = lightest(*mask, darkest(*seed, seed[-seed_bpl]));
	}
	
	for (int i = 1; i < line_len; ++i) {
		seed -= seed_bpl;
		mask -= mask_bpl;
		*seed = lightest(*mask, darkest(*seed, seed[seed_bpl]));
	}
}

/**
 * \return non-zero if more iterations are required, zero otherwise.
 */
uint8_t seedFillGray4SlowIteration(QImage& seed, QImage const& mask)
{
	int const w = seed.width();
	int const h = seed.height();
	
	uint8_t* seed_line = seed.bits();
	uint8_t const* mask_line = mask.bits();
	uint8_t const* prev_line = seed_line;
	
	int const seed_bpl = seed.bytesPerLine();
	int const mask_bpl = mask.bytesPerLine();
	
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
		seed_line -= seed_bpl;
		mask_line -= mask_bpl;
	}
	
	return modified;
}

/**
 * \return non-zero if more iterations are required, zero otherwise.
 */
uint8_t seedFillGray8SlowIteration(QImage& seed, QImage const& mask)
{
	int const w = seed.width();
	int const h = seed.height();
	
	uint8_t* seed_line = seed.bits();
	uint8_t const* mask_line = mask.bits();
	uint8_t const* prev_line = seed_line;
	
	int const seed_bpl = seed.bytesPerLine();
	int const mask_bpl = mask.bytesPerLine();
	
	uint8_t modified = 0;
	
	// Some code below doesn't handle such cases.
	if (w == 1) {
		seedFillGrayVertLine(seed_line, seed_bpl, mask_line, mask_bpl, h);
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
		seed_line -= seed_bpl;
		mask_line -= mask_bpl;
	}
	
	return modified;
}

struct HTransition
{
	int west_delta; // -1 or 0
	int east_delta; // 1 or 0
	
	HTransition(int west_delta_, int east_delta_)
	: west_delta(west_delta_), east_delta(east_delta_) {}
};

struct VTransition
{
	int north_mask; // 0 or ~0
	int south_mask; // 0 or ~0
	
	VTransition(int north_mask_, int south_mask_)
	: north_mask(north_mask_), south_mask(south_mask_) {}
};

struct Position
{
	uint8_t* seed;
	uint8_t const* mask;
	int x;
	int y;
	
	Position(uint8_t* seed_, uint8_t const* mask_, int x_, int y_)
	: seed(seed_), mask(mask_), x(x_), y(y_) {}
};

inline void processNeighbor(
	std::queue<Position>& queue, uint8_t const this_val,
	uint8_t* const neighbor, uint8_t const* const neighbor_mask,
	Position const& base_pos, int const x_delta, int const y_delta)
{
	uint8_t const new_val = lightest(this_val, *neighbor_mask);
	if (darker_than(new_val, *neighbor)) {
		*neighbor = new_val;
		int const x = base_pos.x + x_delta;
		int const y = base_pos.y + y_delta;
		queue.push(Position(neighbor, neighbor_mask, x, y));
	}
}

void spreadGray4(
	std::queue<Position>& queue,
	HTransition const* h_transitions,
	VTransition const* v_transitions,
	int const seed_bpl, int const mask_bpl)
{
	while (!queue.empty()) {
		Position const pos(queue.front());
		queue.pop();
		
		uint8_t const this_val = *pos.seed;
		HTransition const ht(h_transitions[pos.x]);
		VTransition const vt(v_transitions[pos.y]);
		uint8_t* seed;
		uint8_t const* mask;
		
		// Western neighbor.
		seed = pos.seed + ht.west_delta;
		mask = pos.mask + ht.west_delta;
		processNeighbor(
			queue, this_val, seed, mask, pos, ht.west_delta, 0
		);
		
		// Eastern neighbor.
		seed = pos.seed + ht.east_delta;
		mask = pos.mask + ht.east_delta;
		processNeighbor(
			queue, this_val, seed, mask, pos, ht.east_delta, 0
		);
		
		// Northern neighbor.
		seed = pos.seed - (seed_bpl & vt.north_mask);
		mask = pos.mask - (mask_bpl & vt.north_mask);
		processNeighbor(
			queue, this_val, seed, mask, pos, 0, -1 & vt.north_mask
		);
		
		// Southern neighbor.
		seed = pos.seed + (seed_bpl & vt.south_mask);
		mask = pos.mask + (mask_bpl & vt.south_mask);
		processNeighbor(
			queue, this_val, seed, mask, pos, 0, 1 & vt.south_mask
		);
	}
}

void spreadGray8(
	std::queue<Position>& queue,
	HTransition const* h_transitions,
	VTransition const* v_transitions,
	int const seed_bpl, int const mask_bpl)
{
	while (!queue.empty()) {
		Position const pos(queue.front());
		queue.pop();
		
		uint8_t const this_val = *pos.seed;
		HTransition const ht(h_transitions[pos.x]);
		VTransition const vt(v_transitions[pos.y]);
		uint8_t* seed;
		uint8_t const* mask;
		
		// Northern neighbor.
		seed = pos.seed - (seed_bpl & vt.north_mask);
		mask = pos.mask - (mask_bpl & vt.north_mask);
		processNeighbor(
			queue, this_val, seed, mask, pos, 0, -1 & vt.north_mask
		);
		
		// North-Western neighbor.
		seed = pos.seed - (seed_bpl & vt.north_mask) + ht.west_delta;
		mask = pos.mask - (mask_bpl & vt.north_mask) + ht.west_delta;
		processNeighbor(
			queue, this_val, seed, mask,
			pos, ht.west_delta, -1 & vt.north_mask
		);
		
		// North-Eastern neighbor.
		seed = pos.seed - (seed_bpl & vt.north_mask) + ht.east_delta;
		mask = pos.mask - (mask_bpl & vt.north_mask) + ht.east_delta;
		processNeighbor(
			queue, this_val, seed, mask,
			pos, ht.east_delta, -1 & vt.north_mask
		);
		
		// Eastern neighbor.
		seed = pos.seed + ht.east_delta;
		mask = pos.mask + ht.east_delta;
		processNeighbor(
			queue, this_val, seed, mask, pos, ht.east_delta, 0
		);
		
		// Western neighbor.
		seed = pos.seed + ht.west_delta;
		mask = pos.mask + ht.west_delta;
		processNeighbor(
			queue, this_val, seed, mask, pos, ht.west_delta, 0
		);
		
		// Southern neighbor.
		seed = pos.seed + (seed_bpl & vt.south_mask);
		mask = pos.mask + (mask_bpl & vt.south_mask);
		processNeighbor(
			queue, this_val, seed, mask, pos, 0, 1 & vt.south_mask
		);
		
		// South-Eastern neighbor.
		seed = pos.seed + (seed_bpl & vt.south_mask) + ht.east_delta;
		mask = pos.mask + (mask_bpl & vt.south_mask) + ht.east_delta;
		processNeighbor(
			queue, this_val, seed, mask,
			pos, ht.east_delta, 1 & vt.south_mask
		);
		
		// South-Western neighbor.
		seed = pos.seed + (seed_bpl & vt.south_mask) + ht.west_delta;
		mask = pos.mask + (seed_bpl & vt.south_mask) + ht.west_delta;
		processNeighbor(
			queue, this_val, seed, mask,
			pos, ht.west_delta, 1 & vt.south_mask
		);
	}
}

void initHorTransitions(std::vector<HTransition>& transitions, int const width)
{
	transitions.reserve(width);
	
	if (width == 1) {
		// No transitions allowed.
		transitions.push_back(HTransition(0, 0));
		return;
	}
	
	// Only east transition is allowed.
	transitions.push_back(HTransition(0, 1));
	
	for (int i = 1; i < width - 1; ++i) {
		// Both transitions are allowed.
		transitions.push_back(HTransition(-1, 1));
	}
	
	// Only west transition is allowed.
	transitions.push_back(HTransition(-1, 0));
}

void initVertTransitions(std::vector<VTransition>& transitions, int const height)
{
	transitions.reserve(height);
	
	if (height == 1) {
		// No transitions allowed.
		transitions.push_back(VTransition(0, 0));
		return;
	}
	
	// Only south transition is allowed.
	transitions.push_back(VTransition(0, ~0));
	
	for (int i = 1; i < height - 1; ++i) {
		// Both transitions are allowed.
		transitions.push_back(VTransition(~0, ~0));
	}
	
	// Only north transition is allowed.
	transitions.push_back(VTransition(~0, 0));
}

void seedFillGray4(QImage& seed_img, QImage const& mask_img)
{
	int const w = seed_img.width();
	int const h = seed_img.height();
	
	uint8_t* seed_line = seed_img.bits();
	uint8_t const* mask_line = mask_img.bits();
	
	uint8_t* const seed_top_line = seed_line;
	uint8_t const* const mask_top_line = mask_line;
	
	int const seed_bpl = seed_img.bytesPerLine();
	int const mask_bpl = mask_img.bytesPerLine();
	
	uint8_t* prev_line = seed_line;
	
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
			seed_line[x] = pixel;
			prev_pixel = pixel;
		}
		
		prev_line = seed_line;
		seed_line += seed_bpl;
		mask_line += mask_bpl;
	}
	
	seed_line -= seed_bpl;
	mask_line -= mask_bpl;
	
	std::queue<Position> queue;
	std::vector<HTransition> h_transitions;
	std::vector<VTransition> v_transitions;
	initHorTransitions(h_transitions, w);
	initVertTransitions(v_transitions, h);
	
	// Bottom to top.
	for (int y = h - 1; y >= 0; --y) {
		VTransition const vt(v_transitions[y]);
		
		// Right to left.
		for (int x = w - 1; x >= 0; --x) {
			HTransition const ht(h_transitions[x]);
			
			uint8_t* const p_base_seed = seed_line + x;
			uint8_t const* const p_base_mask = mask_line + x;
			
			uint8_t* const p_east_seed = p_base_seed + ht.east_delta;
			uint8_t* const p_south_seed = p_base_seed + (seed_bpl & vt.south_mask);
			
			uint8_t const new_val = lightest(
				*p_base_mask,
				darkest(*p_east_seed, *p_south_seed)
			);
			if (darker_than(*p_base_seed, new_val)) {
				continue;
			}
			
			*p_base_seed = new_val;
			
			uint8_t const east_mask = p_base_mask[ht.east_delta];
			uint8_t const south_mask = p_base_mask[mask_bpl & vt.south_mask];
			
			// Note that we access seeds through pointers.  That's
			// because some of they may point to *p_base_seed,
			// which has just changed.
			
			int flag = int(lightest(new_val, east_mask)) - int(*p_east_seed);
			flag |= int(lightest(new_val, south_mask)) - int(*p_south_seed);
			
			if (flag < 0) {
				// This indicates that at least one if the
				// subtractions above gave negative result.
				queue.push(Position(p_base_seed, p_base_mask, x, y));
			}
		}
		
		seed_line -= seed_bpl;
		mask_line -= mask_bpl;
	}
	
	spreadGray4(
		queue, &h_transitions[0],
		&v_transitions[0], seed_bpl, mask_bpl
	);
}

void seedFillGray8(QImage& seed_img, QImage const& mask_img)
{
	int const w = seed_img.width();
	int const h = seed_img.height();
	
	uint8_t* seed_line = seed_img.bits();
	uint8_t const* mask_line = mask_img.bits();
	
	int const seed_bpl = seed_img.bytesPerLine();
	int const mask_bpl = mask_img.bytesPerLine();
	
	// Some code below doesn't handle such cases.
	if (w == 1) {
		seedFillGrayVertLine(seed_line, seed_bpl, mask_line, mask_bpl, h);
		return;
	} else if (h == 1) {
		seedFillGrayHorLine(seed_line, mask_line, w);
		return;
	}
	
	// Note: we usually process the first line by assigning
	// prev_line = seed_line, but in this case prev_line[x + 1]
	// won't be clipped by its mask when we use it to update seed_line[x].
	// The wrong value may propagate further from there, so clipping
	// we do on the anti-raster pass won't help.
	// That's why we process the first line separately.
	seed_line[0] = lightest(seed_line[0], mask_line[0]);
	for (int x = 1; x < w; ++x) {
		seed_line[x] = lightest(
			mask_line[x],
			darkest(seed_line[x], seed_line[x - 1])
		);
	}
	
	uint8_t* prev_line = seed_line;
	
	// Top to bottom.
	for (int y = 1; y < h; ++y) {
		seed_line += seed_bpl;
		mask_line += mask_bpl;
		
		int x = 0;
		
		// Leftmost pixel.
		seed_line[x] = lightest(
			mask_line[x],
			darkest(
				seed_line[x],
				darkest(prev_line[x], prev_line[x + 1])
			)
		);
		
		// Left to right.
		while (++x < w - 1) {
			seed_line[x] = lightest(
				mask_line[x],
				darkest(
					darkest(
						darkest(seed_line[x], seed_line[x - 1]),
						darkest(prev_line[x], prev_line[x - 1])
					),
					prev_line[x + 1]
				)
			);
		}
		
		// Rightmost pixel.
		seed_line[x] = lightest(
			mask_line[x],
			darkest(
				darkest(seed_line[x], seed_line[x - 1]),
				darkest(prev_line[x], prev_line[x - 1])
			)
		);
		
		prev_line = seed_line;
	}
	
	std::queue<Position> queue;
	std::vector<HTransition> h_transitions;
	std::vector<VTransition> v_transitions;
	initHorTransitions(h_transitions, w);
	initVertTransitions(v_transitions, h);
	
	// Bottom to top.
	for (int y = h - 1; y >= 0; --y) {
		VTransition const vt(v_transitions[y]);
		
		for (int x = w - 1; x >= 0; --x) {
			HTransition const ht(h_transitions[x]);
			
			uint8_t* const p_base_seed = seed_line + x;
			uint8_t const* const p_base_mask = mask_line + x;
			
			uint8_t* const p_east_seed = p_base_seed + ht.east_delta;
			uint8_t* const p_south_seed = p_base_seed + (seed_bpl & vt.south_mask);
			uint8_t* const p_south_west_seed = p_south_seed + ht.west_delta;
			uint8_t* const p_south_east_seed = p_south_seed + ht.east_delta;
			
			uint8_t const new_val = lightest(
				*p_base_mask,
				darkest(
					darkest(*p_east_seed, *p_south_east_seed),
					darkest(*p_south_seed, *p_south_west_seed)
				)
			);
			if (darker_than(*p_base_seed, new_val)) {
				continue;
			}
			
			*p_base_seed = new_val;
			
			uint8_t const east_mask = p_base_mask[ht.east_delta];
			uint8_t const* const p_south_mask = p_base_mask + (mask_bpl & vt.south_mask);
			uint8_t const south_mask = *p_south_mask;
			uint8_t const south_west_mask = p_south_mask[ht.west_delta];
			uint8_t const south_east_mask = p_south_mask[ht.east_delta];
			
			// Note that we access seeds through pointers.  That's
			// because some of they may point to *p_base_seed,
			// which has just changed.
			
			int flag = int(lightest(new_val, east_mask)) - int(*p_east_seed);
			flag |= int(lightest(new_val, south_mask)) - int(*p_south_seed);
			flag |= int(lightest(new_val, south_west_mask)) - int(*p_south_west_seed);
			flag |= int(lightest(new_val, south_east_mask)) - int(*p_south_east_seed);
			
			if (flag < 0) {
				// This indicates that at least one if the
				// subtractions above gave negative result.
				queue.push(Position(p_base_seed, p_base_mask, x, y));
			}
		}
		
		seed_line -= seed_bpl;
		mask_line -= mask_bpl;
	}
	
	spreadGray8(
		queue, &h_transitions[0],
		&v_transitions[0], seed_bpl, mask_bpl
	);
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

QImage seedFillGray(
	QImage const& seed, QImage const& mask, Connectivity const connectivity)
{
	QImage result(seed);
	seedFillGrayInPlace(result, mask, connectivity);
	return result;
}

void seedFillGrayInPlace(
	QImage& seed, QImage const& mask, Connectivity const connectivity)
{
	if (seed.size() != mask.size()) {
		throw std::invalid_argument("seedFillGrayInPlace: seed and mask have different sizes");
	}
	
	// These will also catch null images.
	if (seed.format() != QImage::Format_Indexed8 || !seed.isGrayscale()) {
		throw std::invalid_argument("seedFillGrayInPlace: seed is not grayscale");
	}
	if (mask.format() != QImage::Format_Indexed8 || !mask.isGrayscale()) {
		throw std::invalid_argument("seedFillGrayInPlace: mask is not grayscale");
	}
	
	if (connectivity == CONN4) {
		seedFillGray4(seed, mask);
	} else {
		seedFillGray8(seed, mask);
	}
}

QImage seedFillGraySlow(
	QImage const& seed, QImage const& mask, Connectivity const connectivity)
{
	QImage img(seed);
	
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
