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

#include "SlicedHistogram.h"
#include "BinaryImage.h"
#include "BitOps.h"
#include <QRect>
#include <stdexcept>

namespace imageproc
{

SlicedHistogram::SlicedHistogram()
{
}

SlicedHistogram::SlicedHistogram(BinaryImage const& image, Type const type)
{
	switch (type) {
		case ROWS:
			processHorizontalLines(image, image.rect());
			break;
		case COLS:
			processVerticalLines(image, image.rect());
			break;
	}
}

SlicedHistogram::SlicedHistogram(
	BinaryImage const& image, QRect const& area, Type const type)
{
	if (!image.rect().contains(area)) {
		throw std::invalid_argument("SlicedHistogram: area exceeds the image");
	}
	
	switch (type) {
		case ROWS:
			processHorizontalLines(image, area);
			break;
		case COLS:
			processVerticalLines(image, area);
			break;
	}
}

void
SlicedHistogram::processHorizontalLines(BinaryImage const& image, QRect const& area)
{
	m_data.reserve(area.height());
	
	int const top = area.top();
	int const bottom = area.bottom();
	int const wpl = image.wordsPerLine();
	int const first_word_idx = area.left() >> 5;
	int const last_word_idx = area.right() >> 5; // area.right() is within area
	uint32_t const first_word_mask = ~uint32_t(0) >> (area.left() & 31);
	int const last_word_unused_bits = (last_word_idx << 5) + 31 - area.right();
	uint32_t const last_word_mask = ~uint32_t(0) << last_word_unused_bits;
	uint32_t const* line = image.data() + top * wpl;
	
	if (first_word_idx == last_word_idx) {
		uint32_t const mask = first_word_mask & last_word_mask;
		for (int y = top; y <= bottom; ++y, line += wpl) {
			int const count = countNonZeroBits(line[first_word_idx] & mask);
			m_data.push_back(count);
		}
	} else {
		for (int y = top; y <= bottom; ++y, line += wpl) {
			int idx = first_word_idx;
			int count = countNonZeroBits(line[idx] & first_word_mask);
			for (++idx; idx != last_word_idx; ++idx) {
				count += countNonZeroBits(line[idx]);
			}
			count += countNonZeroBits(line[idx] & last_word_mask);
			m_data.push_back(count);
		}
	}
}

void
SlicedHistogram::processVerticalLines(BinaryImage const& image, QRect const& area)
{
	m_data.reserve(area.width());
	
	int const right = area.right();
	int const height = area.height();
	int const wpl = image.wordsPerLine();
	uint32_t const* const top_line = image.data() + area.top() * wpl;
	
	for (int x = area.left(); x <= right; ++x) {
		uint32_t const* pword = top_line + (x >> 5);
		int const least_significant_zeroes = 31 - (x & 31);
		int count = 0;
		for (int i = 0; i < height; ++i, pword += wpl) {
			count += (*pword >> least_significant_zeroes) & 1;
		}
		m_data.push_back(count);
	}
}

} // namespace imageproc
