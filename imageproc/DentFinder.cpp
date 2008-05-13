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

#include "DentFinder.h"
#include "BinaryImage.h"
#include "BWColor.h"
#include <assert.h>

namespace imageproc
{

struct DentFinder::ImgInfo
{
	ImgInfo(BinaryImage const& src, BinaryImage& dst);
	
	uint32_t const* src_data;
	uint32_t* dst_data;
	int src_wpl;
	int dst_wpl;
	int width;
	int height;
};

DentFinder::ImgInfo::ImgInfo(BinaryImage const& src, BinaryImage& dst)
:	src_data(src.data()),
	dst_data(dst.data()),
	src_wpl(src.wordsPerLine()),
	dst_wpl(dst.wordsPerLine()),
	width(src.width()),
	height(src.height())
{
}

imageproc::BinaryImage
DentFinder::findDentsAndHoles(imageproc::BinaryImage const& src)
{
	if (src.isNull()) {
		return BinaryImage();
	}
	
	BinaryImage dst(src.size(), WHITE);
	ImgInfo info(src, dst);
	
	scanHorizontalLines(info);
	scanVerticalLines(info);
	scanSlashDiagonals(info);
	scanBackslashDiagonals(info);
	
	return dst;
}

uint32_t
DentFinder::getPixel(uint32_t const* const src_line, int const x)
{
	int const offset = x >> 5;
	int const shift = x & 31;
	uint32_t const msb = uint32_t(1) << 31;
	return src_line[offset] & (msb >> shift);
}

void
DentFinder::transferPixel(
	uint32_t const* const src_line,
	uint32_t* const dst_line, int const x)
{
	int const offset = x >> 5;
	int const shift = x & 31;
	uint32_t const msb = uint32_t(1) << 31;
	dst_line[offset] |= ~src_line[offset] & (msb >> shift);
}

void
DentFinder::scanHorizontalLines(ImgInfo const info)
{
	uint32_t const* src_line = info.src_data;
	uint32_t* dst_line = info.dst_data;
	for (int y = 0; y < info.height; ++y,
			src_line += info.src_wpl, dst_line += info.dst_wpl) {
		// Find the first black pixel.
		int first_black = -1; // TODO: rename to first_black_x
		for (int x = 0; x < info.width; ++x) {
			if (getPixel(src_line, x)) {
				first_black = x;
				break;
			}
		}
		
		if (first_black == -1) {
			// Only one black span on this line.
			continue;
		}
		
		// Continue until we encounter a white pixel.
		int first_black_end = -1;
		for (int x = first_black + 1; x < info.width; ++x) {
			if (!getPixel(src_line, x)) {
				first_black_end = x;
				break;
			}
		}
		
		if (first_black_end == -1) {
			// Only one black span on this line.
			continue;
		}
		
		// Find the last black pixel.
		int last_black = -1;
		for (int x = info.width - 1; x >= 0; --x) {
			if (getPixel(src_line, x)) {
				last_black = x;
				break;
			}
		}
		
		// We know we have at least one black pixel.
		assert(last_black != -1);
		
		if (first_black_end > last_black) {
			// Only one black span on this line.
			continue;
		}
		
		// Continue until we encounter a white pixel.
		int last_black_end = -1;
		for (int x = last_black - 1; x >= 0; --x) {
			if (!getPixel(src_line, x)) {
				last_black_end = x;
				break;
			}
		}
		assert(last_black_end != -1);
		
		// Every white pixel between the first and the last black span
		// becomes black in the destination image.
		for (int x = first_black_end; x <= last_black_end; ++x) {
			transferPixel(src_line, dst_line, x);
		}
	}
}

void
DentFinder::scanVerticalLines(ImgInfo const info)
{
	for (int x = 0; x < info.width; ++x) {
		int const offset = x >> 5;
		uint32_t const msb = uint32_t(1) << 31;
		uint32_t const mask = msb >> (x & 31);
		
		// Find the first black pixel.
		int first_black = -1;
		uint32_t const* p_src_first = info.src_data + offset;
		for (int y = 0; y < info.height; ++y, p_src_first += info.src_wpl) {
			if (*p_src_first & mask) {
				first_black = y;
				break;
			}
		}
		
		if (first_black == -1) {
			// Only one black span on this line.
			continue;
		}
		
		// Continue until we encounter a white pixel.
		int first_black_end = -1;
		for (int y = first_black + 1; y < info.height; ++y) {
			p_src_first += info.src_wpl;
			if (!(*p_src_first & mask)) {
				first_black_end = y;
				break;
			}
		}
		
		if (first_black_end == -1) {
			// Only one black span on this line.
			continue;
		}
		
		// Find the last black pixel.
		int last_black  = -1;
		uint32_t const* p_src_last = info.src_data + info.height * info.src_wpl + offset;
		for (int y = info.height - 1; y >= 0; --y) {
			p_src_last -= info.src_wpl;
			if (*p_src_last & mask) {
				last_black = y;
				break;
			}
		}
		
		// We know we have at least one black pixel.
		assert(last_black != -1);
		
		if (first_black_end > last_black) {
			// Only one black span on this line.
			continue;
		}
		
		// Continue until we encounter a white pixel.
		int last_black_end = -1;
		for (int y = last_black - 1; y >= 0; --y) {
			p_src_last -= info.src_wpl;
			if (!(*p_src_first & mask)) {
				last_black_end = y;
				break;
			}
		}
		assert(last_black_end != -1);
		
		// Every white pixel between the first and the last black span
		// becomes black in the destination image.
		uint32_t* p_dst = info.dst_data + first_black_end * info.dst_wpl + offset;
		for (int y = first_black_end; y <= last_black_end; ++y) {
			*p_dst |= ~*p_src_first & mask;
			p_src_first += info.src_wpl;
			p_dst += info.dst_wpl;
		}
	}
}

void
DentFinder::scanSlashDiagonals(ImgInfo const info)
{
	// These are endpoint coordinates of our slash diagonals.
	int x0 = 0, y0 = 0;
	int x1 = 0, y1 = 0;
	
	/*
	+------------+
	|(x1, y1)--+ |
	|(x0, y0)  | |
	| |        | |
	| |        v |
	| +------->  |
	+------------+
	*/
	
	while (x0 < info.width /*&& y1 < info.height*/) {
		do { // just to be able to break from it.
			// Find the first black pixel.
			int first_black_x = -1;
			int x = x0;
			int y = y0;
			uint32_t const* src_line = info.src_data + info.src_wpl * y;
			do {
				if (getPixel(src_line, x)) {
					first_black_x = x;
					break;
				}
				++x;
				--y;
				src_line -= info.src_wpl;
			} while (x < x1);
			
			if (first_black_x == -1) {
				// No black pixels on this line.
				break;
			}
			
			// Continue until we encounter a white pixel.
			int first_black_end_x = -1;
			int first_black_end_y = -1;
			do {
				if (!getPixel(src_line, x)) {
					first_black_end_x = x;
					first_black_end_y = y;
					break;
				}
				++x;
				--y;
				src_line -= info.src_wpl;
			} while (x < x1);
			
			if (first_black_end_x == -1) {
				// Only one black span on this line.
				break;
			}
			
			// Find the last black pixel.
			int last_black_x = -1;
			x = x1;
			y = y1;
			src_line = info.src_data + info.src_wpl * y;
			do {
				if (getPixel(src_line, x)) {
					last_black_x = x;
					break;
				}
				--x;
				++y;
				src_line += info.src_wpl;
			} while (x >= x0);
			
			// We know we have at least one black pixel.
			assert(last_black_x != -1);
			
			if (first_black_end_x > last_black_x) {
				// Only one black span on this line.
				break;
			}
			
			// Continue until we encounter a white pixel.
			int last_black_end_x = -1;
			int last_black_end_y = -1;
			do {
				if (!getPixel(src_line, x)) {
					last_black_end_x = x;
					last_black_end_y = y;
					break;
				}
				--x;
				++y;
				src_line += info.src_wpl;
			} while (x >= x0);
			assert(last_black_end_x != -1);
			
			// Every white pixel between the first and the last black span
			// becomes black in the destination image.
			x = first_black_end_x;
			y = first_black_end_y;
			src_line = info.src_data + info.src_wpl * y;
			uint32_t* dst_line = info.dst_data + info.dst_wpl * y;
			do {
				transferPixel(src_line, dst_line, x);
				++x;
				--y;
				src_line -= info.src_wpl;
				dst_line -= info.dst_wpl;
			} while (x <= last_black_end_x);
		} while (false);
		
		if (y0 + 1 < info.height) {
			++y0;
		} else {
			++x0;
		}
		if (x1 + 1 < info.width) {
			++x1;
		} else {
			++y1;
		}
	}
}

void
DentFinder::scanBackslashDiagonals(ImgInfo const info)
{
	// These are endpoint coordinates of our backslash diagonals.
	int x0 = 0, y0 = info.height - 1;
	int x1 = 0, y1 = y0;
	
	/*
	+------------+
	| +------->  |
	| |        ^ |
	| |        | |
	|(x0, y0)  | |
	|(x1, y1)--+ |
	+------------+
	*/
	
	while (x0 < info.width) {
		do { // just to be able to break from it.
			// Find the first black pixel.
			int first_black_x = -1;
			int x = x0;
			int y = y0;
			uint32_t const* src_line = info.src_data + info.src_wpl * y;
			do {
				if (getPixel(src_line, x)) {
					first_black_x = x;
					break;
				}
				++x;
				++y;
				src_line += info.src_wpl;
			} while (x < x1);
			
			if (first_black_x == -1) {
				// No black pixels on this line.
				break;
			}
			
			// Continue until we encounter a white pixel.
			int first_black_end_x = -1;
			int first_black_end_y = -1;
			do {
				if (!getPixel(src_line, x)) {
					first_black_end_x = x;
					first_black_end_y = y;
					break;
				}
				++x;
				++y;
				src_line += info.src_wpl;
			} while (x < x1);
			
			if (first_black_end_x == -1) {
				// Only one black span on this line.
				break;
			}
			
			// Find the last black pixel.
			int last_black_x = -1;
			x = x1;
			y = y1;
			src_line = info.src_data + info.src_wpl * y;
			do {
				if (getPixel(src_line, x)) {
					last_black_x = x;
					break;
				}
				--x;
				--y;
				src_line -= info.src_wpl;
			} while (x >= x0);
			
			// We know we have at least one black pixel.
			assert(last_black_x != -1);
			
			if (first_black_end_x > last_black_x) {
				// Only one black span on this line.
				break;
			}
			
			// Continue until we encounter a white pixel.
			int last_black_end_x = -1;
			int last_black_end_y = -1;
			do {
				if (!getPixel(src_line, x)) {
					last_black_end_x = x;
					last_black_end_y = y;
					break;
				}
				--x;
				--y;
				src_line -= info.src_wpl;
			} while (x >= x0);
			assert(last_black_end_x != -1);
			
			// Every white pixel between the first and the last black span
			// becomes black in the destination image.
			x = first_black_end_x;
			y = first_black_end_y;
			src_line = info.src_data + info.src_wpl * y;
			uint32_t* dst_line = info.dst_data + info.dst_wpl * y;
			do {
				transferPixel(src_line, dst_line, x);
				++x;
				++y;
				src_line += info.src_wpl;
				dst_line += info.dst_wpl;
			} while (x <= last_black_end_x);
		} while (false);
		
		if (y0 > 0) {
			--y0;
		} else {
			++x0;
		}
		if (x1 + 1 < info.width) {
			++x1;
		} else {
			--y1;
		}
	}
}

} // namespace imageproc
