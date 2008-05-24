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

#include "SkewFinder.h"
#include "BinaryImage.h"
#include "BWColor.h"
#include "BitOps.h"
#include "Shear.h"
#include "ReduceThreshold.h"
#include "Constants.h"
#include <QDebug>
#include <stdexcept>
#include <stdint.h>
#include <math.h>

namespace imageproc
{

double const Skew::GOOD_CONFIDENCE = 2.0;

double const SkewFinder::DEFAULT_MAX_ANGLE = 7.0;

double const SkewFinder::DEFAULT_ACCURACY = 0.1;

int const SkewFinder::DEFAULT_COARSE_REDUCTION = 2;

int const SkewFinder::DEFAULT_FINE_REDUCTION = 1;

double const SkewFinder::LOW_SCORE = 1000.0;

SkewFinder::SkewFinder()
:	m_maxAngle(DEFAULT_MAX_ANGLE),
	m_accuracy(DEFAULT_ACCURACY),
	m_resolutionRatio(1.0),
	m_coarseReduction(DEFAULT_COARSE_REDUCTION),
	m_fineReduction(DEFAULT_FINE_REDUCTION)
{
}

void
SkewFinder::setMaxAngle(double const max_angle)
{
	if (max_angle < 0.0 || max_angle > 45.0) {
		throw std::invalid_argument("SkewFinder: max skew angle is invalid");
	}
	m_maxAngle = max_angle;
}

void
SkewFinder::setDesiredAccuracy(double const accuracy)
{
	m_accuracy = accuracy;
}

void
SkewFinder::setCoarseReduction(int const reduction)
{
	if (reduction < 0) {
		throw std::invalid_argument("SkewFinder: coarse reduction is invalid");
	}
	m_coarseReduction = reduction;
}

void
SkewFinder::setFineReduction(int const reduction)
{
	if (reduction < 0) {
		throw std::invalid_argument("SkewFinder: fine reduction is invalid");
	}
	m_fineReduction = reduction;
}

void
SkewFinder::setResolutionRatio(double const ratio)
{
	if (ratio <= 0.0) {
		throw std::invalid_argument("SkewFinder: resolution ratio is invalid");
	}
	m_resolutionRatio = ratio;
}

Skew
SkewFinder::findSkew(BinaryImage const& image) const
{
	if (image.isNull()) {
		throw std::invalid_argument("SkewFinder: null image was provided");
	}
	
	ReduceThreshold coarse_reduced(image);
	int const min_reduction = std::min(m_coarseReduction, m_fineReduction);
	for (int i = 0; i < min_reduction; ++i) {
		coarse_reduced.reduce(i == 0 ? 1 : 2);
	}
	
	ReduceThreshold fine_reduced(coarse_reduced.image());
	
	for (int i = min_reduction; i < m_coarseReduction; ++i) {
		coarse_reduced.reduce(i == 0 ? 1 : 2);
	}
	
	BinaryImage skewed(coarse_reduced.image().size());
	double const coarse_step = 1.0; // degrees
	
	// Coarse linear search.
	int num_coarse_scores = 0;
	double sum_coarse_scores = 0.0;
	double best_coarse_score = 0.0;
	double best_coarse_angle = -m_maxAngle;
	for (double angle = -m_maxAngle; angle <= m_maxAngle; angle += coarse_step) {
		double const score = process(coarse_reduced, skewed, angle);
		sum_coarse_scores += score;
		++num_coarse_scores;
		if (score > best_coarse_score) {
			best_coarse_angle = angle;
			best_coarse_score = score;
		}
	}
	
	if (m_accuracy >= coarse_step) {
		double confidence = 0.0;
		if (num_coarse_scores > 1) {
			confidence = best_coarse_score /
				sum_coarse_scores * num_coarse_scores;
		}
		return Skew(-best_coarse_angle, confidence - 1.0);
	}
	
	for (int i = min_reduction; i < m_fineReduction; ++i) {
		fine_reduced.reduce(i == 0 ? 1 : 2);
	}
	
	if (m_coarseReduction != m_fineReduction) {
		skewed = BinaryImage(fine_reduced.image().size());
	}
	
	// Fine binary search.
	double angle_plus = best_coarse_angle + 0.5 * coarse_step;
	double angle_minus = best_coarse_angle - 0.5 * coarse_step;
	double score_plus = process(fine_reduced, skewed, angle_plus);
	double score_minus = process(fine_reduced, skewed, angle_minus);
	double const fine_score1 = score_plus;
	double const fine_score2 = score_minus;
	while (angle_plus - angle_minus > m_accuracy) {
		if (score_plus > score_minus) {
			angle_minus = 0.5 * (angle_plus + angle_minus);
			score_minus = process(fine_reduced, skewed, angle_minus);
		} else if (score_plus < score_minus) {
			angle_plus = 0.5 * (angle_plus + angle_minus);
			score_plus = process(fine_reduced, skewed, angle_plus);
		} else {
			// This protects us from unreasonably low m_accuracy.
			break;
		}
	}
	
	double best_angle;
	double best_score;
	if (score_plus > score_minus) {
		best_angle = angle_plus;
		best_score = score_plus;
	} else {
		best_angle = angle_minus;
		best_score = score_minus;
	}
	
	if (best_score <= LOW_SCORE) {
		return Skew(-best_angle, 0.0); // Zero confidence.
	}
	
	double confidence = 0.0;
	if (num_coarse_scores > 1) {
		confidence = best_score / sum_coarse_scores * num_coarse_scores;
	} else {
		int num_scores = num_coarse_scores;
		double sum_scores = sum_coarse_scores;
		num_scores += 2;
		sum_scores += fine_score1;
		sum_scores += fine_score2;
		confidence = best_score / sum_scores * num_scores;
	}
	return Skew(-best_angle, confidence - 1.0);
}

double
SkewFinder::process(BinaryImage const& src, BinaryImage& dst, double const angle) const
{
	double const tg = tan(angle * constants::DEG2RAD);
	double const x_center = 0.5 * dst.width();
	vShearFromTo(src, dst, tg / m_resolutionRatio, x_center, WHITE);
	return calcScore(dst);
}

double
SkewFinder::calcScore(BinaryImage const& image)
{
	int const width = image.width();
	int const height = image.height();
	uint32_t const* line = image.data();
	int const wpl = image.wordsPerLine();
	int const last_word_idx = (width - 1) >> 5;
	uint32_t const last_word_mask = ~uint32_t(0) << (31 - ((width - 1) & 31));
	
	double score = 0.0;
	int last_line_black_pixels = 0;
	for (int y = 0; y < height; ++y, line += wpl) {
		int num_black_pixels = 0;
		int i = 0;
		for (; i != last_word_idx; ++i) {
			num_black_pixels += countNonZeroBits(line[i]);
		}
		num_black_pixels += countNonZeroBits(line[i] & last_word_mask);
		
		if (y != 0) {
			double const diff = num_black_pixels - last_line_black_pixels;
			score += diff * diff;
		}
		last_line_black_pixels = num_black_pixels;
	}
	
	return score;
}

} // namespace imageproc
