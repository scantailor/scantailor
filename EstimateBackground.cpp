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

#include "EstimateBackground.h"
#include "ImageTransformation.h"
#include "TaskStatus.h"
#include "DebugImages.h"
#include "imageproc/GrayImage.h"
#include "imageproc/BinaryImage.h"
#include "imageproc/BWColor.h"
#include "imageproc/BitOps.h"
#include "imageproc/Transform.h"
#include "imageproc/Scale.h"
#include "imageproc/Morphology.h"
#include "imageproc/Connectivity.h"
#include "imageproc/PolynomialLine.h"
#include "imageproc/PolynomialSurface.h"
#include "imageproc/PolygonRasterizer.h"
#include "imageproc/GrayImage.h"
#include "imageproc/GrayRasterOp.h"
#include "imageproc/RasterOpGeneric.h"
#include "imageproc/SeedFill.h"
#include <QImage>
#include <QColor>
#include <QSize>
#include <QPolygonF>
#include <QTransform>
#include <QDebug>
#include <Qt>
#ifndef Q_MOC_RUN
#include <boost/lambda/lambda.hpp>
#include <boost/lambda/bind.hpp>
#include <boost/lambda/control_structures.hpp>
#endif
#include <vector>
#include <algorithm>
#include <stdint.h>
#include <assert.h>
#include <string.h>

using namespace imageproc;

struct AbsoluteDifference
{
	static uint8_t transform(uint8_t src, uint8_t dst) {
		return abs(int(src) - int(dst));
	}
};

/**
 * The same as seedFillGrayInPlace() with a seed of two black lines
 * at top and bottom, except here colors may only spread vertically.
 */
static void seedFillTopBottomInPlace(GrayImage& image)
{
	uint8_t* const data = image.data();
	int const stride = image.stride();
	
	int const width = image.width();
	int const height = image.height();
	
	std::vector<uint8_t> seed_line(height, 0xff);
	
	for (int x = 0; x < width; ++x) {
		uint8_t* p = data + x;
		
		uint8_t prev = 0; // black
		for (int y = 0; y < height; ++y) {
			seed_line[y] = prev = std::max(*p, prev);
			p += stride;
		}
		
		prev = 0; // black
		for (int y = height - 1; y >= 0; --y) {
			p -= stride;
			*p = prev = std::max(
				*p, std::min(seed_line[y], prev)
			);
		}
	}
}

static void morphologicalPreprocessingInPlace(GrayImage& image, DebugImages* dbg)
{
	using namespace boost::lambda;

	// We do morphological preprocessing with one of two methods.  The first
	// one is good for cases when the dark area is in the middle of the image,
	// touching at least one of the vertical edges and not touching the horizontal one.
	// The second method is good for pages that have pictures (partly) in the
	// shadow of the spine.  Most of the other cases can be handled by any of these
	// two methods.

	GrayImage method1(createFramedImage(image.size()));
	seedFillGrayInPlace(method1, image, CONN8);

	// This will get rid of the remnants of letters.  Note that since we know we
	// are working with at most 300x300 px images, we can just hardcode the size.
	method1 = openGray(method1, QSize(1, 20), 0x00);
	if (dbg) {
		dbg->add(method1, "preproc_method1");
	}

	seedFillTopBottomInPlace(image);
	if (dbg) {
		dbg->add(image, "preproc_method2");
	}

	// Now let's estimate, which of the methods is better for this case.

	// Take the difference between two methods.
	GrayImage diff(image);
	rasterOpGeneric(
		diff.data(), diff.stride(), diff.size(),
		method1.data(), method1.stride(), _1 -= _2
	);
	if (dbg) {
		dbg->add(diff, "raw_diff");
	}

	// Approximate the difference using a polynomial function.
	// If it fits well into our data set, we consider the difference
	// to be caused by a shadow rather than a picture, and use method1.
	GrayImage approximated(PolynomialSurface(3, 3, diff).render(diff.size()));
	if (dbg) {
		dbg->add(approximated, "approx_diff");
	}

	// Now let's take the difference between the original difference
	// and approximated difference.
	rasterOpGeneric(
		diff.data(), diff.stride(), diff.size(),
		approximated.data(), approximated.stride(),
		if_then_else(_1 > _2, _1 -= _2, _1 = _2 - _1)
	);
	approximated = GrayImage(); // save memory.
	if (dbg) {
		dbg->add(diff, "raw_vs_approx_diff");
	}

	// Our final decision is like this:
	// If we have at least 1% of pixels that are greater than 10,
	// we consider that we have a picture rather than a shadow,
	// and use method2.

	int sum = 0;
	GrayscaleHistogram hist(diff);
	for (int i = 255; i > 10; --i) {
		sum += hist[i];
	}

	//qDebug() << "% of pixels > 10: " << 100.0 * sum / (diff.width() * diff.height());

	if (sum < 0.01 * (diff.width() * diff.height())) {
		image = method1;
		if (dbg) {
			dbg->add(image, "use_method1");
		}
	} else {
		// image is already set to method2
		if (dbg) {
			dbg->add(image, "use_method2");
		}
	}
}

imageproc::PolynomialSurface estimateBackground(
	GrayImage const& input, QPolygonF const& area_to_consider,
	TaskStatus const& status, DebugImages* dbg)
{
	QSize reduced_size(input.size());
	reduced_size.scale(300, 300, Qt::KeepAspectRatio);
	GrayImage background(scaleToGray(GrayImage(input), reduced_size));
	if (dbg) {
		dbg->add(background, "downscaled");
	}
	
	status.throwIfCancelled();

	morphologicalPreprocessingInPlace(background, dbg);

	status.throwIfCancelled();
	
	int const width = background.width();
	int const height = background.height();
	
	uint8_t const* const bg_data = background.data();
	int const bg_stride = background.stride();
	
	BinaryImage mask(background.size(), BLACK);
	
	if (!area_to_consider.isEmpty()) {
		QTransform xform;
		xform.scale(
			(double)reduced_size.width() / input.width(),
			(double)reduced_size.height() / input.height()
		);
		PolygonRasterizer::fillExcept(
			mask, WHITE, xform.map(area_to_consider), Qt::WindingFill
		);
	}
	
	if (dbg) {
		dbg->add(mask, "area_to_consider");
	}
	
	uint32_t* mask_data = mask.data();
	int mask_stride = mask.wordsPerLine();
	
	std::vector<uint8_t> line(std::max(width, height), 0);
	uint32_t const msb = uint32_t(1) << 31;
	
	status.throwIfCancelled();
	
	// Smooth every horizontal line with a polynomial,
	// then mask pixels that became significantly lighter.
	for (int x = 0; x < width; ++x) {
		uint32_t const mask = ~(msb >> (x & 31));
		
		int const degree = 2;
		PolynomialLine pl(degree, bg_data + x, height, bg_stride);
		pl.output(&line[0], height, 1);
		
		uint8_t const* p_bg = bg_data + x;
		uint32_t* p_mask = mask_data + (x >> 5);
		for (int y = 0; y < height; ++y) {
			if (*p_bg + 30 < line[y]) {
				*p_mask &= mask;
			}
			p_bg += bg_stride;
			p_mask += mask_stride;
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
		
		bg_line += bg_stride;
		mask_line += mask_stride;
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
	mask_stride = mask.wordsPerLine();
	
	// Check each horizontal line.  If it's mostly
	// white (ignored), then make it completely white.
	int const last_word_idx = (width - 1) >> 5;
	uint32_t const last_word_mask = ~uint32_t(0) << (
		32 - width - (last_word_idx << 5)
	);
	mask_line = mask_data;
	for (int y = 0; y < height; ++y, mask_line += mask_stride) {
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
			p_mask += mask_stride;
		}
		if (black_count < height / 4) {
			for (int y = height - 1; y >= 0; --y) {
				p_mask -= mask_stride;
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
