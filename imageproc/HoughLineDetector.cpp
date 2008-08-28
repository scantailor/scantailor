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

#include "HoughLineDetector.h"
#include "BinaryImage.h"
#include "BWColor.h"
#include "ConnCompEraser.h"
#include "ConnComp.h"
#include "Connectivity.h"
#include "Constants.h"
#include "Morphology.h"
#include "RasterOp.h"
#include "SeedFill.h"
#include <QSize>
#include <QRect>
#include <QPoint>
#include <QPointF>
#include <QLineF>
#include <QDebug>
#include <boost/foreach.hpp>
#include <algorithm>
#include <math.h>
#include <stdint.h>
#include <assert.h>

namespace imageproc
{

class HoughLineDetector::GreaterQualityFirst
{
public:
	bool operator()(HoughLine const& lhs, HoughLine const& rhs) const {
		return lhs.quality() > rhs.quality();
	}
};


HoughLineDetector::HoughLineDetector(
	QSize const& input_dimensions, double const distance_resolution,
	double const start_angle, double const angle_delta, int const num_angles)
:	m_distanceResolution(distance_resolution),
	m_recipDistanceResolution(1.0 / distance_resolution)
{
	int const max_x = input_dimensions.width() + 1;
	int const max_y = input_dimensions.height() + 1;
	
	// distance = x * cos(angle) + y * cos(angle)
	double const max_distance = sqrt(max_x * max_x + max_y * max_y);
	
	// min_distance would normally be -max_distance, but as it turns out,
	// we can decrease the absolute value of min_distance (and thus the
	// histogram size) if we limit the angle to [-45 .. 135] degrees.
	// In fact we can easily do that, because angle and angle + 180 degrees
	// are equivalent for our purposes.  So, min_distance becomes:
	double const min_distance = std::max(max_x, max_y) / -sqrt(2.0);
	
	// We bias distances to make them non-negative.
	m_distanceBias = -min_distance;
	
	int const num_distance_bins = (int)ceil(
		(max_distance - min_distance + 1.0) * m_recipDistanceResolution
	);
	
	m_histWidth = num_distance_bins + 2;
	m_histHeight = num_angles + 2;
	m_histogram.resize(m_histWidth * m_histHeight, 0);
	
	m_angleUnitVectors.reserve(num_angles);
	for (int i = 0; i < num_angles; ++i) {
		double angle = start_angle + angle_delta * i;
		angle *= constants::DEG2RAD;
		
		double sine = sin(angle);
		double cosine = cos(angle);
		
		// See comments about min_distance above.
		double const max_abs = fabs(sine) > fabs(cosine) ? sine : cosine;
		if (max_abs < 0.0) {
			sine = -sine;
			cosine = -cosine;
		}
		
		m_angleUnitVectors.push_back(QPointF(cosine, sine));
	}
}

void
HoughLineDetector::process(int x, int y, unsigned weight)
{
	// + m_histWidth is required to skip the padding line.
	unsigned* hist_line = &m_histogram[0] + m_histWidth;
	
	BOOST_FOREACH (QPointF const& uv, m_angleUnitVectors) {
		double const distance = uv.x() * x + uv.y() * y;
		double const biased_distance = distance + m_distanceBias;
		
		int const bin = (int)(biased_distance * m_recipDistanceResolution + 0.5) + 1;
		assert(bin < m_histWidth - 1);
		hist_line[bin] += weight;
		
		hist_line += m_histWidth;
	}
}

std::vector<HoughLine>
HoughLineDetector::findLines(unsigned const quality_lower_bound) const
{
	BinaryImage peaks(
		findHistogramPeaks(
			m_histogram, m_histWidth, m_histHeight,
			quality_lower_bound
		)
	);
	
	std::vector<HoughLine> lines;
	
	QRect const peaks_rect(peaks.rect());
	ConnCompEraser eraser(peaks.release(), CONN8);
	ConnComp cc;
	while (!(cc = eraser.nextConnComp()).isNull()) {
		unsigned const level = m_histogram[
			cc.seed().y() * m_histWidth + cc.seed().x()
		];
		
		// Shifting by (1, 1) is required because our histogram
		// is padded by a line of bins on each side.
		QPoint const center(cc.rect().center() - QPoint(1, 1));
		
		QPointF const norm_uv(m_angleUnitVectors[center.y()]);
		double const distance = (center.x() + 0.5)
				* m_distanceResolution - m_distanceBias;
		lines.push_back(HoughLine(norm_uv, distance, level));
	}
	
	std::sort(lines.begin(), lines.end(), GreaterQualityFirst());
	
	return lines;
}

BinaryImage
HoughLineDetector::findHistogramPeaks(
	std::vector<unsigned> const& hist,
	int const width, int const height, unsigned const lower_bound)
{
	// Peak candidates are connected components of bins having the same
	// value.  Such a connected component may or may not be a peak.
	BinaryImage peak_candidates(
		findPeakCandidates(hist, width, height, lower_bound)
	);
	
	// To check if a peak candidate is really a peak, we have to check
	// if every bin it borders has a lower value than that candidate.
	BinaryImage border_mask(dilateBrick(peak_candidates, QSize(3, 3)));
	rasterOp<RopXor<RopSrc, RopDst> >(border_mask, peak_candidates);
	
	// Bins bordering a peak candidate fall into two categories:
	// 1. The bin is lower than the peak candidate it borders.
	// 2. The bin is has the same value as the peak candidate,
	//    but it borders with some other bin that has a greater value.
	// The second case indicates that our candidate is not relly a peak.
	// To test for the second case we are going to increment the values
	// of the border bins, find the peak candidates again and analize
	// the differences.
	std::vector<unsigned> new_hist(hist);
	incrementBinsMasked(new_hist, width, height, border_mask);
	
	border_mask.release();
	
	BinaryImage diff(findPeakCandidates(new_hist, width, height, lower_bound));
	rasterOp<RopXor<RopSrc, RopDst> >(diff, peak_candidates);
	
	// If a bin that has changed its state was a part of a peak candidate,
	// it means a neighboring border bin went from equal to a greater value,
	// which indicates that such candidate is not a peak.
	BinaryImage const not_peaks(seedFill(diff, peak_candidates, CONN8));
	
	rasterOp<RopXor<RopSrc, RopDst> >(peak_candidates, not_peaks);
	return peak_candidates;
}

/**
 * Build a binary image where a black pixel indicates that the corresponding
 * histogram bin meets the following conditions:
 * \li It doesn't have a greater neighbor.
 * \li It's not on en edge.
 * \li It's value is not below \p lower_bound.
 */
BinaryImage
HoughLineDetector::findPeakCandidates(
	std::vector<unsigned> const& hist,
	int const width, int const height, unsigned const lower_bound)
{
	std::vector<unsigned> maxed(hist.size(), 0);
	
	// Every bin becomes the maximum of itself and its 8 neighbors.
	// Edge bins are not changed.
	max3x3(hist, maxed, width, height);
	
	// Those that haven't changed didn't have a neighbor.
	BinaryImage equal_map(
		buildEqualMap(hist, maxed, width, height, lower_bound)
	);
	
	// Now deal with edges.
	equal_map.fillExcept(equal_map.rect().adjusted(1, 1, -1, -1), WHITE);
	
	return equal_map;
}

/**
 * Increment bins that correspond to black pixels in \p mask.
 */
void
HoughLineDetector::incrementBinsMasked(
	std::vector<unsigned>& hist,
	int const width, int const height, BinaryImage const& mask)
{
	uint32_t const* mask_line = mask.data();
	int const mask_wpl = mask.wordsPerLine();
	unsigned* hist_line = &hist[0];
	uint32_t const msb = uint32_t(1) << 31;
	
	for (int y = 0; y < height; ++y) {
		for (int x = 0; x < width; ++x) {
			if (mask_line[x >> 5] & (msb >> (x & 31))) {
				++hist_line[x];
			}
		}
		mask_line += mask_wpl;
		hist_line += width;
	}
}

/**
 * Every bin in \p dst is set to the maximum of the corresponding bin
 * in \p src and its 8 neighbors.  Bins on an edge are not modified.
 */
void
HoughLineDetector::max3x3(
	std::vector<unsigned> const& src, std::vector<unsigned>& dst,
	int const width, int const height)
{
	std::vector<unsigned> tmp(src.size(), 0);
	max3x1(src, tmp, width, height);
	max1x3(tmp, dst, width, height);
}

/**
 * Every bin in \p dst is set to the maximum of the corresponding bin
 * in \p src and its left and right neighbors.  Bins on an edge are not modified.
 */
void
HoughLineDetector::max3x1(
	std::vector<unsigned> const& src, std::vector<unsigned>& dst,
	int const width, int const height)
{
	maxPrevCurNext(src, dst, width, height, 1);
}

/**
 * Every bin in \p dst is set to the maximum of the corresponding bin
 * in \p src and its top and bottom neighbors.  Bins on an edge are not modified.
 */
void
HoughLineDetector::max1x3(
	std::vector<unsigned> const& src, std::vector<unsigned>& dst,
	int const width, int const height)
{
	maxPrevCurNext(src, dst, width, height, width);
}

/**
 * Every bin in \p dst is set to the maximum of the corresponding bin
 * in \p src and its neighbors at \p next_offset and \p -next_offset.
 * Bins on an edge are not modified.
 */
void
HoughLineDetector::maxPrevCurNext(
	std::vector<unsigned> const& src, std::vector<unsigned>& dst,
	int const width, int const height, int const next_offset)
{
	unsigned const* src_line = &src[0] + width;
	unsigned* dst_line = &dst[0] + width;
	
	for (int row = 1; row < height - 1; ++row) {
		for (int col = 1; col < width - 1; ++col) {
			unsigned const prev = src_line[col - next_offset];
			unsigned const cur = src_line[col];
			unsigned const next = src_line[col + next_offset];
			dst_line[col] = std::max(prev, std::max(cur, next));
		}
		
		src_line += width;
		dst_line += width;
	}
}

/**
 * Builds a binary image where a black pixel indicates that the corresponding
 * bins in two histograms have equal values, and those values are not below
 * \p lower_bound.
 */
BinaryImage
HoughLineDetector::buildEqualMap(
	std::vector<unsigned> const& src1, std::vector<unsigned> const& src2,
	int const width, int const height, unsigned const lower_bound)
{
	BinaryImage dst(width, height, WHITE);
	uint32_t* dst_line = dst.data();
	int const dst_wpl = dst.wordsPerLine();
	unsigned const* src1_line = &src1[0];
	unsigned const* src2_line = &src2[0];
	uint32_t const msb = uint32_t(1) << 31;
	
	for (int y = 0; y < height; ++y) {
		for (int x = 0; x < width; ++x) {
			if (src1_line[x] >= lower_bound &&
					src1_line[x] == src2_line[x]) {
				dst_line[x >> 5] |= msb >> (x & 31);
			}
		}
		dst_line += dst_wpl;
		src1_line += width;
		src2_line += width;
	}
	
	return dst;
}


/*=============================== HoughLine ================================*/

QLineF
HoughLine::unitSegment() const
{
	QPointF const line_point(m_normUnitVector * m_distance);
	QPointF const line_vector(m_normUnitVector.y(), -m_normUnitVector.x());
	return QLineF(line_point, line_point + line_vector);
}

} // namespace imageproc
