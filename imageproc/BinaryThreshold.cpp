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
#include <QImage>
#include <stdexcept>
#include <stdint.h>

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

} // namespace imageproc
