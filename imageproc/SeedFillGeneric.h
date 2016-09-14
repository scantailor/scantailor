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

#ifndef IMAGEPROC_SEEDFILL_GENERIC_H_
#define IMAGEPROC_SEEDFILL_GENERIC_H_

#include "Connectivity.h"
#include "FastQueue.h"
#include <QSize>
#include <vector>
#include <assert.h>

namespace imageproc
{

namespace detail
{

namespace seed_fill_generic
{

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

template<typename T>
struct Position
{
	T* seed;
	T const* mask;
	int x;
	int y;

	Position(T* seed_, T const* mask_, int x_, int y_)
	: seed(seed_), mask(mask_), x(x_), y(y_) {}
};

void initHorTransitions(std::vector<HTransition>& transitions, int width);

void initVertTransitions(std::vector<VTransition>& transitions, int height);

template<typename T, typename SpreadOp, typename MaskOp>
void seedFillSingleLine(
	SpreadOp spread_op, MaskOp mask_op, int const line_len,
	T* seed, int const seed_stride, T const* mask, int const mask_stride)
{
	if (line_len == 0) {
		return;
	}

	*seed = mask_op(*seed, *mask);

	for (int i = 1; i < line_len; ++i) {
		seed += seed_stride;
		mask += mask_stride;
		*seed = mask_op(*mask, spread_op(*seed, seed[-seed_stride]));
	}

	for (int i = 1; i < line_len; ++i) {
		seed -= seed_stride;
		mask -= mask_stride;
		*seed = mask_op(*mask, spread_op(*seed, seed[seed_stride]));
	}
}

template<typename T, typename SpreadOp, typename MaskOp>
inline void processNeighbor(
	SpreadOp spread_op, MaskOp mask_op,
	FastQueue<Position<T> >& queue, T const this_val,
	T* const neighbor, T const* const neighbor_mask,
	Position<T> const& base_pos, int const x_delta, int const y_delta)
{
	T const new_val(mask_op(*neighbor_mask, spread_op(this_val, *neighbor)));
	if (new_val != *neighbor) {
		*neighbor = new_val;
		int const x = base_pos.x + x_delta;
		int const y = base_pos.y + y_delta;
		queue.push(Position<T>(neighbor, neighbor_mask, x, y));
	}
}

template<typename T, typename SpreadOp, typename MaskOp>
void spread4(
	SpreadOp spread_op, MaskOp mask_op,
	FastQueue<Position<T> >& queue,
	HTransition const* h_transitions,
	VTransition const* v_transitions,
	int const seed_stride, int const mask_stride)
{
	while (!queue.empty()) {
		Position<T> const pos(queue.front());
		queue.pop();

		T const this_val(*pos.seed);
		HTransition const ht(h_transitions[pos.x]);
		VTransition const vt(v_transitions[pos.y]);
		T* seed;
		T const* mask;

		// Western neighbor.
		seed = pos.seed + ht.west_delta;
		mask = pos.mask + ht.west_delta;
		processNeighbor(
			spread_op, mask_op, queue, this_val, seed, mask, pos, ht.west_delta, 0
		);

		// Eastern neighbor.
		seed = pos.seed + ht.east_delta;
		mask = pos.mask + ht.east_delta;
		processNeighbor(
			spread_op, mask_op, queue, this_val, seed, mask, pos, ht.east_delta, 0
		);

		// Northern neighbor.
		seed = pos.seed - (seed_stride & vt.north_mask);
		mask = pos.mask - (mask_stride & vt.north_mask);
		processNeighbor(
			spread_op, mask_op, queue, this_val, seed, mask, pos, 0, -1 & vt.north_mask
		);

		// Southern neighbor.
		seed = pos.seed + (seed_stride & vt.south_mask);
		mask = pos.mask + (mask_stride & vt.south_mask);
		processNeighbor(
			spread_op, mask_op, queue, this_val, seed, mask, pos, 0, 1 & vt.south_mask
		);
	}
}

template<typename T, typename SpreadOp, typename MaskOp>
void spread8(
	SpreadOp spread_op, MaskOp mask_op,
	FastQueue<Position<T> >& queue,
	HTransition const* h_transitions,
	VTransition const* v_transitions,
	int const seed_stride, int const mask_stride)
{
	while (!queue.empty()) {
		Position<T> const pos(queue.front());
		queue.pop();

		T const this_val(*pos.seed);
		HTransition const ht(h_transitions[pos.x]);
		VTransition const vt(v_transitions[pos.y]);
		T* seed;
		T const* mask;

		// Northern neighbor.
		seed = pos.seed - (seed_stride & vt.north_mask);
		mask = pos.mask - (mask_stride & vt.north_mask);
		processNeighbor(
			spread_op, mask_op, queue, this_val, seed, mask,
			pos, 0, -1 & vt.north_mask
		);

		// North-Western neighbor.
		seed = pos.seed - (seed_stride & vt.north_mask) + ht.west_delta;
		mask = pos.mask - (mask_stride & vt.north_mask) + ht.west_delta;
		processNeighbor(
			spread_op, mask_op, queue, this_val, seed, mask,
			pos, ht.west_delta, -1 & vt.north_mask
		);

		// North-Eastern neighbor.
		seed = pos.seed - (seed_stride & vt.north_mask) + ht.east_delta;
		mask = pos.mask - (mask_stride & vt.north_mask) + ht.east_delta;
		processNeighbor(
			spread_op, mask_op, queue, this_val, seed, mask,
			pos, ht.east_delta, -1 & vt.north_mask
		);

		// Eastern neighbor.
		seed = pos.seed + ht.east_delta;
		mask = pos.mask + ht.east_delta;
		processNeighbor(
			spread_op, mask_op, queue, this_val, seed, mask, pos, ht.east_delta, 0
		);

		// Western neighbor.
		seed = pos.seed + ht.west_delta;
		mask = pos.mask + ht.west_delta;
		processNeighbor(
			spread_op, mask_op, queue, this_val, seed, mask, pos, ht.west_delta, 0
		);

		// Southern neighbor.
		seed = pos.seed + (seed_stride & vt.south_mask);
		mask = pos.mask + (mask_stride & vt.south_mask);
		processNeighbor(
			spread_op, mask_op, queue, this_val, seed, mask, pos, 0, 1 & vt.south_mask
		);

		// South-Eastern neighbor.
		seed = pos.seed + (seed_stride & vt.south_mask) + ht.east_delta;
		mask = pos.mask + (mask_stride & vt.south_mask) + ht.east_delta;
		processNeighbor(
			spread_op, mask_op, queue, this_val, seed, mask,
			pos, ht.east_delta, 1 & vt.south_mask
		);

		// South-Western neighbor.
		seed = pos.seed + (seed_stride & vt.south_mask) + ht.west_delta;
		mask = pos.mask + (seed_stride & vt.south_mask) + ht.west_delta;
		processNeighbor(
			spread_op, mask_op, queue, this_val, seed, mask,
			pos, ht.west_delta, 1 & vt.south_mask
		);
	}
}

template<typename T, typename SpreadOp, typename MaskOp>
void seedFill4(
	SpreadOp spread_op, MaskOp mask_op,
	T* const seed, int const seed_stride, QSize const size,
	T const* const mask, int const mask_stride)
{
	int const w = size.width();
	int const h = size.height();

	T* seed_line = seed;
	T const* mask_line = mask;
	T* prev_line = seed_line;

	// Top to bottom.
	for (int y = 0; y < h; ++y) {
		int x = 0;

		// First item in line.
		T prev(mask_op(mask_line[x], spread_op(seed_line[x], prev_line[x])));
		seed_line[x] = prev;

		// Other items, left to right.
		while (++x < w) {
			prev = mask_op(mask_line[x], spread_op(prev, spread_op(seed_line[x], prev_line[x])));
			seed_line[x] = prev;
		}

		prev_line = seed_line;
		seed_line += seed_stride;
		mask_line += mask_stride;
	}

	seed_line -= seed_stride;
	mask_line -= mask_stride;

	FastQueue<Position<T> > queue;
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

			T* const p_base_seed = seed_line + x;
			T const* const p_base_mask = mask_line + x;

			T* const p_east_seed = p_base_seed + ht.east_delta;
			T* const p_south_seed = p_base_seed + (seed_stride & vt.south_mask);

			T const new_val(
				mask_op(
					*p_base_mask,
					spread_op(*p_base_seed, spread_op(*p_east_seed, *p_south_seed))
				)
			);
			if (new_val == *p_base_seed) {
				continue;
			}

			*p_base_seed = new_val;

			Position<T> const pos(p_base_seed, p_base_mask, x, y);
			T const* p_east_mask = p_base_mask + ht.east_delta;
			T const* p_south_mask = p_base_mask + (mask_stride & vt.south_mask);

			// Eastern neighbor.
			processNeighbor(
				spread_op, mask_op, queue, new_val,
				p_east_seed, p_east_mask, pos, ht.east_delta, 0
			);

			// Southern neighbor.
			processNeighbor(
				spread_op, mask_op, queue, new_val,
				p_south_seed, p_south_mask, pos, 0, 1 & vt.south_mask
			);
		}

		seed_line -= seed_stride;
		mask_line -= mask_stride;
	}

	spread4(
		spread_op, mask_op, queue, &h_transitions[0],
		&v_transitions[0], seed_stride, mask_stride
	);
}

template<typename T, typename SpreadOp, typename MaskOp>
void seedFill8(
	SpreadOp spread_op, MaskOp mask_op,
	T* const seed, int const seed_stride, QSize const size,
	T const* const mask, int const mask_stride)
{
	int const w = size.width();
	int const h = size.height();

	// Some code below doesn't handle such cases.
	if (w == 1) {
		seedFillSingleLine(spread_op, mask_op, h, seed, seed_stride, mask, mask_stride);
		return;
	} else if (h == 1) {
		seedFillSingleLine(spread_op, mask_op, w, seed, 1, mask, 1);
		return;
	}

	T* seed_line = seed;
	T const* mask_line = mask;

	// Note: we usually process the first line by assigning
	// prev_line = seed_line, but in this case prev_line[x + 1]
	// won't be clipped by its mask when we use it to update seed_line[x].
	// The wrong value may propagate further from there, so clipping
	// we do on the anti-raster pass won't help.
	// That's why we process the first line separately.
	seed_line[0] = mask_op(seed_line[0], mask_line[0]);
	for (int x = 1; x < w; ++x) {
		seed_line[x] = mask_op(
			mask_line[x],
			spread_op(seed_line[x], seed_line[x - 1])
		);
	}

	T* prev_line = seed_line;

	// Top to bottom.
	for (int y = 1; y < h; ++y) {
		seed_line += seed_stride;
		mask_line += mask_stride;

		int x = 0;

		// Leftmost pixel.
		seed_line[x] = mask_op(
			mask_line[x],
			spread_op(
				seed_line[x],
				spread_op(prev_line[x], prev_line[x + 1])
			)
		);

		// Left to right.
		while (++x < w - 1) {
			seed_line[x] = mask_op(
				mask_line[x],
				spread_op(
					spread_op(
						spread_op(seed_line[x], seed_line[x - 1]),
						spread_op(prev_line[x], prev_line[x - 1])
					),
					prev_line[x + 1]
				)
			);
		}

		// Rightmost pixel.
		seed_line[x] = mask_op(
			mask_line[x],
			spread_op(
				spread_op(seed_line[x], seed_line[x - 1]),
				spread_op(prev_line[x], prev_line[x - 1])
			)
		);

		prev_line = seed_line;
	}

	FastQueue<Position<T> > queue;
	std::vector<HTransition> h_transitions;
	std::vector<VTransition> v_transitions;
	initHorTransitions(h_transitions, w);
	initVertTransitions(v_transitions, h);

	// Bottom to top.
	for (int y = h - 1; y >= 0; --y) {
		VTransition const vt(v_transitions[y]);

		for (int x = w - 1; x >= 0; --x) {
			HTransition const ht(h_transitions[x]);

			T* const p_base_seed = seed_line + x;
			T const* const p_base_mask = mask_line + x;

			T* const p_east_seed = p_base_seed + ht.east_delta;
			T* const p_south_seed = p_base_seed + (seed_stride & vt.south_mask);
			T* const p_south_west_seed = p_south_seed + ht.west_delta;
			T* const p_south_east_seed = p_south_seed + ht.east_delta;

			T const new_val = mask_op(
				*p_base_mask,
				spread_op(
					*p_base_seed,
					spread_op(
						spread_op(*p_east_seed, *p_south_east_seed),
						spread_op(*p_south_seed, *p_south_west_seed)
					)
				)
			);
			if (new_val == *p_base_seed) {
				continue;
			}

			*p_base_seed = new_val;

			Position<T> const pos(p_base_seed, p_base_mask, x, y);
			T const* p_east_mask = p_base_mask + ht.east_delta;
			T const* p_south_mask = p_base_mask + (mask_stride & vt.south_mask);
			T const* p_south_west_mask = p_south_mask + ht.west_delta;
			T const* p_south_east_mask = p_south_mask + ht.east_delta;

			// Eastern neighbor.
			processNeighbor(
				spread_op, mask_op, queue, new_val,
				p_east_seed, p_east_mask, pos, ht.east_delta, 0
			);

			// South-eastern neighbor.
			processNeighbor(
				spread_op, mask_op, queue, new_val,
				p_south_east_seed, p_south_east_mask, pos,
				ht.east_delta, 1 & vt.south_mask
			);

			// Southern neighbor.
			processNeighbor(
				spread_op, mask_op, queue, new_val,
				p_south_seed, p_south_mask, pos, 0, 1 & vt.south_mask
			);

			// South-western neighbor.
			processNeighbor(
				spread_op, mask_op, queue, new_val,
				p_south_west_seed, p_south_west_mask, pos,
				ht.west_delta, 1 & vt.south_mask
			);
		}

		seed_line -= seed_stride;
		mask_line -= mask_stride;
	}

	spread8(
		spread_op, mask_op, queue, &h_transitions[0],
		&v_transitions[0], seed_stride, mask_stride
	);
}

} // namespace seed_fill_generic

} // namespace detail


/**
 * The following pseudocode illustrates the principle of a seed-fill algorithm:
 * [code]
 * do {
 *   foreach (<point at x, y>) {
 *     val = mask_op(mask[x, y], seed[x, y]);
 *     foreach (<neighbor at nx, ny>) {
 *       seed[nx, ny] = mask_op(mask[nx, ny], spread_op(seed[nx, ny], val));
 *     }
 *   }
 * } while (<changes to seed were made on this iteration>);
 * [/code]
 *
 * \param spread_op A functor or a pointer to a free function that can be called with
 *        two arguments of type T and return the bigger or the smaller of the two.
 * \param mask_op Same as spread_op, but the opposite operation.
 * \param conn Determines whether to spread values to 4 or 8 eight immediate neighbors.
 * \param[in,out] seed Pointer to the seed buffer.
 * \param seed_stride The size of a row in the seed buffer, in terms of the number of T objects.
 * \param size Dimensions of the seed and the mask buffers.
 * \param mask Pointer to the mask data.
 * \param mask_stride The size of a row in the mask buffer, in terms of the number of T objects.
 *
 * This code is an implementation of the hybrid grayscale restoration algorithm described in:
 * Morphological Grayscale Reconstruction in Image Analysis:
 * Applications and Efficient Algorithms, technical report 91-16, Harvard Robotics Laboratory,
 * November 1991, IEEE Transactions on Image Processing, Vol. 2, No. 2, pp. 176-201, April 1993.\n
 */
template<typename T, typename SpreadOp, typename MaskOp>
void seedFillGenericInPlace(
	SpreadOp spread_op, MaskOp mask_op, Connectivity conn,
	T* seed, int seed_stride, QSize size,
	T const* mask, int mask_stride)
{
	if (size.isEmpty()) {
		return;
	}

	if (conn == CONN4) {
		detail::seed_fill_generic::seedFill4(
			spread_op, mask_op, seed, seed_stride, size, mask, mask_stride
		);
	} else {
		assert(conn == CONN8);
		detail::seed_fill_generic::seedFill8(
			spread_op, mask_op, seed, seed_stride, size, mask, mask_stride
		);
	}
}

} // namespace imageproc

#endif
