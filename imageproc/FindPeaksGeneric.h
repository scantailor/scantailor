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

#ifndef IMAGEPROC_FIND_PEAKS_H_
#define IMAGEPROC_FIND_PEAKS_H_

#include "BinaryImage.h"
#include "Connectivity.h"
#include "SeedFillGeneric.h"
#include "LocalMinMaxGeneric.h"
#include <QSize>
#include <QPoint>
#include <QRect>
#include <QDebug>
#include <vector>
#include <limits>
#include <algorithm>
#include <stdint.h>

namespace imageproc
{

namespace detail
{

namespace find_peaks
{

template<typename T, typename MostSignificantSelector,
		 typename LeastSignificantSelector, typename IncreaseSignificance>
void raiseAllButPeaks(
	MostSignificantSelector most_significant,
	LeastSignificantSelector least_significant,
	IncreaseSignificance increase_significance,
	QSize peak_neighborhood, T outside_values,
	T const* input, int input_stride, QSize size,
	T* to_be_raised, int to_be_raised_stride)
{
	if (peak_neighborhood.isEmpty()) {
		peak_neighborhood.setWidth(1);
		peak_neighborhood.setHeight(1);
	}

	// Dilate the peaks and write results to seed.
	QRect neighborhood(QPoint(0, 0), peak_neighborhood);
	neighborhood.moveCenter(QPoint(0, 0));
	localMinMaxGeneric(
		most_significant, neighborhood, outside_values,
		input, input_stride, size, to_be_raised, to_be_raised_stride
	);

	std::vector<T> mask(to_be_raised, to_be_raised + to_be_raised_stride * size.height());
	int const mask_stride = to_be_raised_stride;

	// Slightly raise the mask relative to to_be_raised.
	std::transform(mask.begin(), mask.end(), mask.begin(), increase_significance);

	seedFillGenericInPlace(
		most_significant, least_significant, CONN8,
		&to_be_raised[0], to_be_raised_stride, size, &mask[0], mask_stride
	);
}

} // namespace find_peaks

} // namespace detail

/**
 * \brief Finds positive or negative peaks on a 2D grid.
 *
 * A peak is defined as a cell or an 8-connected group of cells that is more
 * significant than any of its neighbor cells.  More significant means either
 * greater or less, depending on the kind of peaks we want to locate.
 * In addition, we provide functionality to suppress peaks that are
 * in a specified neighborhood of a more significant peak.
 *
 * \param most_significant A functor or a pointer to a free function that
 *        can be called with two arguments of type T and return the bigger
 *        or the smaller of the two.
 * \param least_significant Same as most_significant, but the opposite operation.
 * \param increase_significance A functor or a pointer to a free function that
 *        takes one argument and returns the next most significant value next
 *        to it.  Hint: for floating point data, use the nextafter() family of
 *        functions.  Their generic versions are available in Boost.
 * \param peak_mutator A functor or a pointer to a free function that will
 *        transform a peak value.  Two typical cases would be returning
 *        the value as is and returning a fixed value.
 * \param non_peak_mutator Same as peak_mutator, but for non-peak values.
 * \param neighborhood The area around a peak (centered at width/2, height/2)
 *        in which less significant peaks will be suppressed.  Passing an empty
 *        neighborhood is equivalent of passing a 1x1 neighborhood.
 * \param outside_values Values that are assumed to be outside of the grid bounds.
 *        This will affect peak detection at the edges of grid.
 * \param[in,out] data Pointer to the data buffer.
 * \param stride The size of a row in the data buffer, in terms of the number of T objects.
 * \param size Grid dimensions.
 */
template<typename T, typename MostSignificantSelector,
		 typename LeastSignificantSelector, typename IncreaseSignificance,
		 typename PeakMutator, typename NonPeakMutator>
void findPeaksInPlaceGeneric(
	MostSignificantSelector most_significant,
	LeastSignificantSelector least_significant,
	IncreaseSignificance increase_significance,
	PeakMutator peak_mutator,
	NonPeakMutator non_peak_mutator,
	QSize peak_neighborhood, T outside_values,
	T* data, int stride, QSize size)
{
	if (size.isEmpty()) {
		return;
	}

	std::vector<T> raised(size.width() * size.height());
	int const raised_stride = size.width();

	detail::find_peaks::raiseAllButPeaks(
		most_significant, least_significant, increase_significance,
		peak_neighborhood, outside_values, data, stride, size,
		&raised[0], raised_stride
	);

	T* data_line = data;
	T* raised_line = &raised[0];
	int const w = size.width();
	int const h = size.height();

	for (int y = 0; y < h; ++y) {
		for (int x = 0; x < w; ++x) {
			if (data_line[x] == raised_line[x]) {
				data_line[x] = peak_mutator(data_line[x]);
			} else {
				data_line[x] = non_peak_mutator(data_line[x]);
			}
		}
		raised_line += raised_stride;
		data_line += stride;
	}
}

/**
 * \brief Same as findPeaksInPlaceGeneric(), but returning a binary image
 *        rather than mutating the input data.
 */
template<typename T, typename MostSignificantSelector,
		 typename LeastSignificantSelector, typename IncreaseSignificance>
BinaryImage findPeaksGeneric(
	MostSignificantSelector most_significant,
	LeastSignificantSelector least_significant,
	IncreaseSignificance increase_significance,
	QSize peak_neighborhood, T outside_values,
	T const* data, int stride, QSize size)
{
	if (size.isEmpty()) {
		return BinaryImage();
	}

	std::vector<T> raised(size.width() * size.height());
	int const raised_stride = size.width();

	detail::find_peaks::raiseAllButPeaks(
		most_significant, least_significant, increase_significance,
		peak_neighborhood, outside_values, data, stride, size,
		&raised[0], raised_stride
	);

	BinaryImage peaks(size, WHITE);
	uint32_t* peaks_line = peaks.data();
	int const peaks_stride = peaks.wordsPerLine();
	T const* data_line = data;
	T const* raised_line = &raised[0];
	int const w = size.width();
	int const h = size.height();
	uint32_t const msb = uint32_t(1) << 31;

	for (int y = 0; y < h; ++y) {
		for (int x = 0; x < w; ++x) {
			if (data_line[x] == raised_line[x]) {
				peaks_line[x >> 5] |= msb >> (x & 31);
			}
		}
		peaks_line += peaks_stride;
		raised_line += raised_stride;
		data_line += stride;
	}

	return peaks;
}

} // namespace imageproc

#endif
