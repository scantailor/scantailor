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

#include "EstimateBackground.h"
#include "ImageTransformation.h"
#include "TaskStatus.h"
#include "DebugImages.h"
#include "imageproc/BinaryImage.h"
#include "imageproc/BWColor.h"
#include "imageproc/BitOps.h"
#include "imageproc/Transform.h"
#include "imageproc/Scale.h"
#include "imageproc/Morphology.h"
#include "imageproc/Connectivity.h"
#include "imageproc/PolynomialLine.h"
#include "imageproc/PolynomialSurface.h"
#include <QImage>
#include <QColor>
#include <QSize>
#include <vector>
#include <algorithm>
#include <stdint.h>
#include <assert.h>
#include <string.h>

using namespace imageproc;

/**
 * The same as seedFillGrayInPlace() with a seed of two black lines
 * at top and bottom, except here colors may only spread vertically.
 */
static void seedFillTopBottomInPlace(QImage& image)
{
	uint8_t* const data = image.bits();
	int const bpl = image.bytesPerLine();
	
	int const width = image.width();
	int const height = image.height();
	
	std::vector<uint8_t> seed_line(height, 0xff);
	
	for (int x = 0; x < width; ++x) {
		uint8_t* p = data + x;
		
		uint8_t prev = 0; // black
		for (int y = 0; y < height; ++y) {
			seed_line[y] = prev = std::max(*p, prev);
			p += bpl;
		}
		
		prev = 0; // black
		for (int y = height - 1; y >= 0; --y) {
			p -= bpl;
			*p = prev = std::max(
				*p, std::min(seed_line[y], prev)
			);
		}
	}
}

imageproc::PolynomialSurface estimateBackground(
	QImage const& input, TaskStatus const& status, DebugImages* dbg)
{
	QSize reduced_size(input.size());
	reduced_size.scale(300, 300, Qt::KeepAspectRatio);
	QImage background(scaleToGray(input, reduced_size));
	if (dbg) {
		dbg->add(background, "downscaled");
	}
	
	status.throwIfCancelled();
	
	seedFillTopBottomInPlace(background);
	if (dbg) {
		dbg->add(background, "seedfill_topbottom");
	}
	
	int const width = background.width();
	int const height = background.height();
	
	uint8_t const* const bg_data = background.bits();
	int const bg_bpl = background.bytesPerLine();
	
	BinaryImage mask(background.size(), BLACK);
	uint32_t* mask_data = mask.data();
	int mask_wpl = mask.wordsPerLine();
	
	std::vector<uint8_t> line(std::max(width, height), 0);
	uint32_t const msb = uint32_t(1) << 31;
	
	status.throwIfCancelled();
	
	// Smooth every horizontal line with a polynomial,
	// then mask pixels that became significantly lighter.
	for (int x = 0; x < width; ++x) {
		uint32_t const mask = ~(msb >> (x & 31));
		
		int const degree = 2;
		PolynomialLine pl(degree, bg_data + x, height, bg_bpl);
		pl.output(&line[0], height, 1);
		
		uint8_t const* p_bg = bg_data + x;
		uint32_t* p_mask = mask_data + (x >> 5);
		for (int y = 0; y < height; ++y) {
			if (*p_bg + 30 < line[y]) {
				*p_mask &= mask;
			}
			p_bg += bg_bpl;
			p_mask += mask_wpl;
		}
	}
	
	status.throwIfCancelled();
	
	// Smooth every vertical line with a polynomial,
	// then mask pixels that became significantly lighter.
	uint8_t const* bg_line = bg_data;
	uint32_t* mask_line = mask_data;
	for (int y = 0; y < height; ++y) {
		int const degree = 4;
		PolynomialLine pl(degree, bg_line, width, 1);
		pl.output(&line[0], width, 1);
		
		for (int x = 0; x < width; ++x) {
			if (bg_line[x] + 30 < line[x]) {
				mask_line[x >> 5] &= ~(msb >> (x & 31));
			}
		}
		
		bg_line += bg_bpl;
		mask_line += mask_wpl;
	}
	
	if (dbg) {
		dbg->add(mask, "mask");
	}
	
	status.throwIfCancelled();
	
	mask = erodeBrick(mask, QSize(3, 3));
	if (dbg) {
		dbg->add(mask, "eroded");
	}
	
	status.throwIfCancelled();
	
	// Update those because mask was overwritten.
	mask_data = mask.data();
	mask_wpl = mask.wordsPerLine();
	
	// Check each horizontal line.  If it's mostly
	// white (ignored), then make it completely white.
	int const last_word_idx = (width - 1) >> 5;
	uint32_t const last_word_mask = ~uint32_t(0) << (
		32 - width - (last_word_idx << 5)
	);
	mask_line = mask_data;
	for (int y = 0; y < height; ++y, mask_line += mask_wpl) {
		int black_count = 0;
		int i = 0;
		
		// Complete words.
		for (; i < last_word_idx; ++i) {
			black_count += countNonZeroBits(mask_line[i]);
		}
		
		// The last (possible incomplete) word.
		black_count += countNonZeroBits(mask_line[i] & last_word_mask);
		
		if (black_count < width / 4) {
			memset(mask_line, 0,
				(last_word_idx + 1) * sizeof(*mask_line));
		}
	}
	
	status.throwIfCancelled();
	
	// Check each vertical line.  If it's mostly
	// white (ignored), then make it completely white.
	for (int x = 0; x < width; ++x) {
		uint32_t const mask = msb >> (x & 31);
		uint32_t* p_mask = mask_data + (x >> 5);
		int black_count = 0;
		for (int y = 0; y < height; ++y) {
			if (*p_mask & mask) {
				++black_count;
			}
			p_mask += mask_wpl;
		}
		if (black_count < height / 4) {
			for (int y = height - 1; y >= 0; --y) {
				p_mask -= mask_wpl;
				*p_mask &= ~mask;
			}
		}
	}
	
	if (dbg) {
		dbg->add(mask, "lines_extended");
	}
	
	status.throwIfCancelled();
	
	return PolynomialSurface(8, 5, background, mask);
}
