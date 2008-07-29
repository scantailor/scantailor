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

#include "kFill.h"
#include "BinaryImage.h"
#include <vector>
#include <stdexcept>

namespace imageproc
{

namespace
{

bool kFillIteration(
	BinaryImage const& src, BinaryImage& dst,
	int const k, int* const nhood)
{
	int const w = src.width();
	int const h = dst.width();
	uint32_t const* src_line = src.data();
	uint32_t* dst_line = dst.data();
	int const src_wpl = src.wordsPerLine();
	int const dst_wpl = dst.wordsPerLine(); // TODO: merge src_wpl and dst_wpl into one
	uint32_t const msb = uint32_t(1) << 31;
	int const n_threshold = k * 3 - 4;
	
	bool modified = false;
	
	for (int y = 0; y < h - k; ++y) {
		for (int x = 0; x < w - k; ++x) {
			// bit 0: have white pixels, bit 1: have black pixels
			int core_colors = 0;
			
			uint32_t const* aux_src_line = src_line;
			for (int i = k - 2; i > 0; --i) {
				aux_src_line += src_wpl;
				for (int cx = x + 1; cx < x + k - 1; ++cx) {
					uint32_t const word = aux_src_line[cx >> 5];
					int const bit = (word >> (31 - (cx & 31))) & 1;
					core_colors |= bit << 1; // have black pixels
					core_colors |= ~bit & 1; // have white pixels
				}
			}
			
			// Index into nhood.  Then, number of items there.
			int ni = 0;
			
			// Top neighborhood line from left to right.
			aux_src_line = src_line;
			for (int nx = x; nx < x + k - 1; ++nx, ++ni) {
				uint32_t const word = aux_src_line[nx >> 5];
				nhood[ni] = (word >> (31 - (nx & 31))) & 1;
			}
			
			// Right neighborhood line from top to bottom.
			int word_idx = (x + k - 1) >> 5;
			int shift = 31 - ((x + k - 1) & 31);
			for (int i = k - 1; i > 0; --i, ++ni) {
				uint32_t const word = aux_src_line[word_idx];
				nhood[ni] = (word >> shift) & 1;
				aux_src_line += src_wpl;
			}
			
			// Bottom neighborhood line from right to left.
			for (int nx = x + k - 1; nx > x; --nx, ++ni) {
				uint32_t const word = aux_src_line[nx >> 5];
				nhood[ni] = (word >> (31 - (nx & 31))) & 1;
			}
			
			// Left neighborhood line from bottom to top.
			word_idx = x >> 5;
			shift = 31 - (x & 31);
			for (int i = k - 1; i > 0; --i, ++ni) {
				uint32_t const word = aux_src_line[word_idx];
				nhood[ni] = (word >> shift) & 1;
				aux_src_line -= src_wpl;
			}
			
			if (core_colors == 1) { // only white pixels in core
				// Number of black pixels in the neighborhood.
				int n = 0;
				
				// Number of black pixels in neighborhood corners.
				int r = 0;
				
				// Number of black connected components in the
				// neighborhood, except for 1 component it may
				// be either 0 or 1.
				int c = 0;
				
				n = nhood[0];
				for (int i = 1; i < ni; ++i) {
					n += nhood[i];
					c += nhood[i] ^ nhood[i - 1];
				}
				c += nhood[0] ^ nhood[ni - 1];
				c >>= 1;
				
				int const* p = &nhood[0];
				for (int i = 0; i < 4; ++i, p += k - 1) {
					r += *p;
				}
				
				if (c <= 1 && (n > n_threshold ||
					(n == n_threshold && r == 2))) {
					
					// Set core pixels to black
					uint32_t* aux_dst_line = dst_line;
					for (int i = k - 2; i > 0; --i) {
						aux_dst_line += dst_wpl;
						for (int cx = x + 1; cx < x + k - 1; ++cx) {
							uint32_t& word = aux_dst_line[cx >> 5];
							word |= msb >> (cx & 31);
						}
					}
					
					modified = true;
				}
			} else if (core_colors == 2) { // only black pixels in core
				// Number of white pixels in the neighborhood.
				int n = 0;
				
				// Number of white pixels in neighborhood corners.
				int r = 0;
				
				// Number of white connected components in the
				// neighborhood, except for 1 component it may
				// be either 0 or 1.
				int c = 0;
				
				n = ~nhood[0] & 1;
				for (int i = 1; i < ni; ++i) {
					n += ~nhood[i] & 1;
					c += nhood[i] ^ nhood[i - 1];
				}
				c += nhood[0] ^ nhood[ni - 1];
				c >>= 1;
				
				int const* p = &nhood[0];
				for (int i = 0; i < 4; ++i, p += k - 1) {
					r += ~*p & 1;
				}
				
				if (c <= 1 && (n > n_threshold ||
					(n == n_threshold && r == 2))) {
					
					// Set core pixels to white
					uint32_t* aux_dst_line = dst_line;
					for (int i = k - 2; i > 0; --i) {
						aux_dst_line += dst_wpl;
						for (int cx = x + 1; cx < x + k - 1; ++cx) {
							uint32_t& word = aux_dst_line[cx >> 5];
							word &= ~(msb >> (cx & 31));
						}
					}
					
					modified = true;
				}
			}
		}
		src_line += src_wpl;
		dst_line += dst_wpl;
	}
	
	return modified;
}

} // anonymous namespace

BinaryImage kFill(BinaryImage src, int const k)
{
	if (k < 3) {
		throw std::invalid_argument("kFill: k must be >= 3");
	}
	
	if (src.width() < k || src.height() < k) {
		// This also handles the null image case.
		return src;
	}
	
	// Neighborhood (window perimeter).
	std::vector<int> nhood(4 * (k - 1), 0); 
	
	BinaryImage dst(src);
	while (kFillIteration(src, dst, k, &nhood[0])) {
		src = dst;
	}
	
	return dst; // or src, they are the same if kFillInteration() returns false.
}

} // namespace imageproc
