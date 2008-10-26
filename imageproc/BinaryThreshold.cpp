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

#include "BinaryThreshold.h"
#include "Grayscale.h"
#include "Morphology.h"
#include <QImage>
#include <QDebug>
#include <stdexcept>
#include <stdint.h>
#include <string.h>
#include <assert.h>

namespace imageproc
{

BinaryThreshold
BinaryThreshold::otsuThreshold(QImage const& image)
{
	return otsuThreshold(GrayscaleHistogram(image));
}

BinaryThreshold
BinaryThreshold::otsuThreshold(GrayscaleHistogram const& pixels_by_color)
{
	int32_t pixels_by_threshold[256];
	int64_t moment_by_threshold[256];
	
	// Note that although BinaryThreshold is defined in such a way
	// that everything below the threshold is considered black,
	// this algorithm assumes that everything below *or equal* to
	// the threshold is considered black.
	// That is, pixels_by_threshold[10] holds the number of pixels
	// in the image that have a gray_level <= 10
	
	pixels_by_threshold[0] = pixels_by_color[0];
	moment_by_threshold[0] = 0;
	for (int i = 1; i < 256; ++i) {
		pixels_by_threshold[i] = pixels_by_threshold[i - 1]
			+ pixels_by_color[i];
		moment_by_threshold[i] = moment_by_threshold[i - 1]
			+ int64_t(pixels_by_color[i]) * i;
	}
	
	int const total_pixels = pixels_by_threshold[255];
	int64_t const total_moment = moment_by_threshold[255];
	double max_variance = 0.0;
	int first_best_threshold = -1;
	int last_best_threshold = -1;
	for (int i = 0; i < 256; ++i) {
		int const pixels_below = pixels_by_threshold[i];
		int const pixels_above = total_pixels - pixels_below;
		if (pixels_below > 0 && pixels_above > 0) { // prevent division by zero
			int64_t const moment_below = moment_by_threshold[i];
			int64_t const moment_above = total_moment - moment_below;
			double const mean_below = (double)moment_below / pixels_below;
			double const mean_above = (double)moment_above / pixels_above;
			double const mean_diff = mean_below - mean_above;
			double const variance = mean_diff * mean_diff * pixels_below * pixels_above;
			if (variance > max_variance) {
				max_variance = variance;
				first_best_threshold = i;
				last_best_threshold = i;
			} else if (variance == max_variance) {
				last_best_threshold = i;
			}
		}
	}
	
	// Compensate the "< threshold" vs "<= threshold" difference.
	++first_best_threshold;
	++last_best_threshold;
	
	// The middle between the two.
	return BinaryThreshold((first_best_threshold + last_best_threshold) >> 1);
}

BinaryThreshold
BinaryThreshold::mokjiThreshold(
	QImage const& image, unsigned const max_edge_width,
	unsigned const min_edge_magnitude)
{
	if (max_edge_width < 1) {
		throw std::invalid_argument("mokjiThreshold: invalud max_edge_width");
	}
	if (min_edge_magnitude < 1) {
		throw std::invalid_argument("mokjiThreshold: invalid min_edge_magnitude");
	}
	
	QImage const gray(toGrayscale(image));
	
	int const dilate_size = (max_edge_width + 1) * 2 - 1;
	QImage dilated(dilateGray(gray, QSize(dilate_size, dilate_size)));
	
	unsigned matrix[256][256];
	memset(matrix, 0, sizeof(matrix));
	
	int const w = image.width();
	int const h = image.height();
	unsigned char const* src_line = gray.bits();
	int const src_bpl = gray.bytesPerLine();
	unsigned char const* dilated_line = dilated.bits();
	int const dilated_bpl = dilated.bytesPerLine();
	
	src_line += max_edge_width * src_bpl;
	dilated_line += max_edge_width * dilated_bpl;
	for (int y = max_edge_width; y < h - (int)max_edge_width; ++y) {
		for (int x = max_edge_width; x < w - (int)max_edge_width; ++x) {
			unsigned const pixel = src_line[x];
			unsigned const darkest_neighbor = dilated_line[x];
			assert(darkest_neighbor <= pixel);
			
			++matrix[darkest_neighbor][pixel];
		}
		src_line += src_bpl;
		dilated_line += dilated_bpl;
	}
	
	unsigned nominator = 0;
	unsigned denominator = 0;
	for (unsigned m = 0; m < 256 - min_edge_magnitude; ++m) {
		for (unsigned n = m + min_edge_magnitude; n < 256; ++n) {
			assert(n >= m);
			
			unsigned const val = matrix[m][n];
			nominator += (m + n) * val;
			denominator += val;
		}
	}
	
	if (denominator == 0) {
		return BinaryThreshold(128);
	}
	
	double const threshold = 0.5 * nominator / denominator;
	return BinaryThreshold((int)(threshold + 0.5));
}

} // namespace imageproc
