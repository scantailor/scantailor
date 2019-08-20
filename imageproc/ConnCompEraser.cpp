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

/* The code is based on Paul Heckbert's stack-based seed fill algorithm
 * from "Graphic Gems", ed. Andrew Glassner, Academic Press, 1990.
 * This version is optimized to eliminate all multiplications. */

#include "ConnCompEraser.h"
#include "ConnComp.h"
#include "BitOps.h"
#include <QRect>
#include <algorithm>
#include <stdexcept>
#include <assert.h>
#include <limits.h>

namespace imageproc
{

struct ConnCompEraser::BBox
{
	int xmin;
	int xmax;
	int ymin;
	int ymax;
	
	BBox(int x, int y) : xmin(x), xmax(x), ymin(y), ymax(y) {}
	
	int width() const { return xmax - xmin + 1; }
	
	int height() const { return ymax - ymin + 1; }
};


inline uint32_t
ConnCompEraser::getBit(uint32_t const* const line, int const x)
{
	uint32_t const mask = (uint32_t(1) << 31) >> (x & 31);
	return line[x >> 5] & mask;
}

inline void
ConnCompEraser::clearBit(uint32_t* const line, int const x)
{
	uint32_t const mask = (uint32_t(1) << 31) >> (x & 31);
	line[x >> 5] &= ~mask;
}

ConnCompEraser::ConnCompEraser(BinaryImage const& image, Connectivity conn)
:	m_image(image),
	m_pLine(0),
	m_width(m_image.width()),
	m_height(m_image.height()),
	m_wpl(m_image.wordsPerLine()),
	m_connectivity(conn),
	m_x(0),
	m_y(0)
{
	// By initializing m_pLine with 0 instead of m_image.data(),
	// we avoid copy-on-write, provided that the caller used image.release().
}

ConnComp
ConnCompEraser::nextConnComp()
{
	if (!moveToNextBlackPixel()) {
		return ConnComp();
	}
	
	if (m_connectivity == CONN4) {
		return eraseConnComp4();
	} else {
		return eraseConnComp8();
	}
}

void
ConnCompEraser::pushSegSameDir(Segment const& seg, int xleft, int xright, BBox& bbox)
{
	bbox.xmin = std::min(bbox.xmin, xleft);
	bbox.xmax = std::max(bbox.xmax, xright);
	bbox.ymin = std::min(bbox.ymin, seg.y);
	bbox.ymax = std::max(bbox.ymax, seg.y);
	
	int const new_dy = seg.dy;
	int const new_dy_wpl = seg.dy_wpl;
	int const new_y = seg.y + new_dy;
	if (new_y >= 0 && new_y < m_height) {
		Segment new_seg;
		new_seg.line = seg.line + new_dy_wpl;
		new_seg.xleft = xleft;
		new_seg.xright = xright;
		new_seg.y = new_y;
		new_seg.dy = new_dy;
		new_seg.dy_wpl = new_dy_wpl;
		m_segStack.push(new_seg);
	}
}

void
ConnCompEraser::pushSegInvDir(Segment const& seg, int xleft, int xright, BBox& bbox)
{
	bbox.xmin = std::min(bbox.xmin, xleft);
	bbox.xmax = std::max(bbox.xmax, xright);
	bbox.ymin = std::min(bbox.ymin, seg.y);
	bbox.ymax = std::max(bbox.ymax, seg.y);
	
	int const new_dy = -seg.dy;
	int const new_dy_wpl = -seg.dy_wpl;
	int const new_y = seg.y + new_dy;
	if (new_y >= 0 && new_y < m_height) {
		Segment new_seg;
		new_seg.line = seg.line + new_dy_wpl;
		new_seg.xleft = xleft;
		new_seg.xright = xright;
		new_seg.y = new_y;
		new_seg.dy = new_dy;
		new_seg.dy_wpl = new_dy_wpl;
		m_segStack.push(new_seg);
	}
}

void
ConnCompEraser::pushInitialSegments()
{
	assert(m_x >= 0 && m_x < m_width);
	assert(m_y >= 0 && m_y < m_height);
	
	if (m_y + 1 < m_height) {
		Segment seg1;
		seg1.line = m_pLine + m_wpl;
		seg1.xleft = m_x;
		seg1.xright = m_x;
		seg1.y = m_y + 1;
		seg1.dy = 1;
		seg1.dy_wpl = m_wpl;
		m_segStack.push(seg1);
	}
	
	Segment seg2;
	seg2.line = m_pLine;
	seg2.xleft = m_x;
	seg2.xright = m_x;
	seg2.y = m_y;
	seg2.dy = -1;
	seg2.dy_wpl = -m_wpl;
	m_segStack.push(seg2);
}

bool
ConnCompEraser::moveToNextBlackPixel()
{
	if (m_image.isNull()) {
		return false;
	}
	
	if (!m_pLine) {
		// By initializing m_pLine with 0 instead of m_image.data(),
		// we allow the caller to delete his copy of the image
		// to avoid copy-on-write.
		// We could also try to avoid copy-on-write in the case of
		// a completely white image, but I don't think it's worth it.
		m_pLine = m_image.data();
	}
	
	uint32_t* line = m_pLine;
	uint32_t const* pword = line + (m_x >> 5);
	
	// Stop word is a last word in line that holds data.
	int const last_bit_idx = m_width - 1;
	uint32_t const* p_stop_word = line + (last_bit_idx >> 5);
	uint32_t const stop_word_mask = ~uint32_t(0) << (31 - (last_bit_idx & 31));
	
	uint32_t word = *pword;
	if (pword == p_stop_word) {
		word &= stop_word_mask;
	}
	word <<= (m_x & 31);
	if (word) {
		int const shift = countMostSignificantZeroes(word);
		m_x += shift;
		assert(m_x < m_width);
		return true;
	}
	
	int y = m_y;
	if (pword != p_stop_word) {
		++pword;
	} else {
		++y;
		line += m_wpl;
		p_stop_word += m_wpl;
		pword = line;
	}
	
	for (; y < m_height; ++y) {
		for (; pword != p_stop_word; ++pword) {
			word = *pword;
			if (word) {
				int const shift = countMostSignificantZeroes(word);
				m_x = ((pword - line) << 5) + shift;
				assert(m_x < m_width);
				m_y = y;
				m_pLine = line;
				return true;
			}
		}
		
		// Handle the stop word (some bits need to be ignored).
		assert(pword == p_stop_word);
		word = *pword & stop_word_mask;
		if (word) {
			int const shift = countMostSignificantZeroes(word);
			m_x = ((pword - line) << 5) + shift;
			assert(m_x < m_width);
			m_y = y;
			m_pLine = line;
			return true;
		}
		
		line += m_wpl;
		p_stop_word += m_wpl;
		pword = line;
	}
	
	return false;
}

ConnComp
ConnCompEraser::eraseConnComp4()
{
	pushInitialSegments();
	
	BBox bbox(m_x, m_y);
	int pix_count = 0;
	
	while (!m_segStack.empty()) {
		// Pop a segment off the stack.
		Segment const seg(m_segStack.top());
		m_segStack.pop();
		
		int const xmax = std::min(seg.xright, m_width - 1);
		
		int x = seg.xleft;
		for (; x >= 0 && getBit(seg.line, x); --x) {
			clearBit(seg.line, x);
			++pix_count;
		}
		
		int xstart = x + 1;
		
		if (x >= seg.xleft) {
			// Pixel at seg.xleft was off and was not cleared.
			goto skip;
		}
		
		if (xstart < seg.xleft - 1) {
			// Leak on left.
			pushSegInvDir(seg, xstart, seg.xleft - 1, bbox);
		}
		
		x = seg.xleft + 1;
		
		do {
			for (; x < m_width && getBit(seg.line, x); ++x) {
				clearBit(seg.line, x);
				++pix_count;
			}
			pushSegSameDir(seg, xstart, x - 1, bbox);
			if (x > seg.xright + 1) {
				// Leak on right.
				pushSegInvDir(seg, seg.xright + 1, x - 1, bbox);
			}
			
			skip:
			for (++x; x <= xmax && !getBit(seg.line, x); ++x) {
				// Skip white pixels.
			}
			xstart = x;
		} while (x <= xmax);
	}
	
	QRect rect(bbox.xmin, bbox.ymin, bbox.width(), bbox.height());
	return ConnComp(QPoint(m_x, m_y), rect, pix_count);
}

ConnComp
ConnCompEraser::eraseConnComp8()
{
	pushInitialSegments();
	
	BBox bbox(m_x, m_y);
	int pix_count = 0;
	
	while (!m_segStack.empty()) {
		// Pop a segment off the stack.
		Segment const seg(m_segStack.top());
		m_segStack.pop();
		
		int const xmax = std::min(seg.xright + 1, m_width - 1);
		
		int x = seg.xleft - 1;
		for (; x >= 0 && getBit(seg.line, x); --x) {
			clearBit(seg.line,x);
			++pix_count;
		}
		
		int xstart = x + 1;
		
		if (x >= seg.xleft - 1) {
			// Pixel at seg.xleft - 1 was off and was not cleared.
			goto skip;
		}
		
		if (xstart < seg.xleft) {
			// Leak on left.
			pushSegInvDir(seg, xstart, seg.xleft - 1, bbox);
		}
		
		x = seg.xleft;
		do {
			for (; x < m_width && getBit(seg.line, x); ++x) {
				clearBit(seg.line, x);
				++pix_count;
			}
			pushSegSameDir(seg, xstart, x - 1, bbox);
			if (x > seg.xright) {
				// Leak on right.
				pushSegInvDir(seg, seg.xright + 1, x - 1, bbox);
			}
			
			skip:
			for (++x; x <= xmax && !getBit(seg.line, x); ++x) {
				// Skip white pixels.
			}
			xstart = x;
		} while (x <= xmax);
	}
	
	QRect rect(bbox.xmin, bbox.ymin, bbox.width(), bbox.height());
	return ConnComp(QPoint(m_x, m_y), rect, pix_count);
}

} // namespace imageproc
