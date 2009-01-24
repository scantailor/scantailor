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

#include "Binarize.h"
#include "BinaryImage.h"
#include "BinaryThreshold.h"
#include "Grayscale.h"
#include "IntegralImage.h"
#include <QImage>
#include <QRect>
#include <QDebug>
#include <vector>
#include <algorithm>
#include <stdexcept>
#include <math.h>
#include <stdint.h>
#include <string.h>
#include <assert.h>

namespace imageproc
{

BinaryImage binarizeOtsu(QImage const& src)
{
	return BinaryImage(src, BinaryThreshold::otsuThreshold(src));
}

BinaryImage binarizeMokji(
	QImage const& src, unsigned const max_edge_width,
	unsigned const min_edge_magnitude)
{
	BinaryThreshold const threshold(
		BinaryThreshold::mokjiThreshold(
			src, max_edge_width, min_edge_magnitude
		)
	);
	return BinaryImage(src, threshold);
}

BinaryImage binarizeSauvola(QImage const& src, QSize const window_size)
{
	if (window_size.isEmpty()) {
		throw std::invalid_argument("binarizeSauvola: invalid window_size");
	}
	
	if (src.isNull()) {
		return BinaryImage();
	}
	
	QImage const gray(toGrayscale(src));
	int const w = gray.width();
	int const h = gray.height();
	
	IntegralImage<uint32_t> integral_image(w, h);
	IntegralImage<uint64_t> integral_sqimage(w, h);
	
	uint8_t const* gray_line = gray.bits();
	int const gray_bpl = gray.bytesPerLine();
	
	for (int y = 0; y < h; ++y, gray_line += gray_bpl) {
		integral_image.beginRow();
		integral_sqimage.beginRow();
		for (int x = 0; x < w; ++x) {
			uint32_t const pixel = gray_line[x];
			integral_image.push(pixel);
			integral_sqimage.push(pixel * pixel);
		}
	}
	
	int const window_lower_half = window_size.height() >> 1;
	int const window_upper_half = window_size.height() - window_lower_half;
	int const window_left_half = window_size.width() >> 1;
	int const window_right_half = window_size.width() - window_left_half;
	
	BinaryImage bw_img(w, h);
	uint32_t* bw_line = bw_img.data();
	int const bw_wpl = bw_img.wordsPerLine();
	
	gray_line = gray.bits();
	for (int y = 0; y < h; ++y) {
		int const top = std::max(0, y - window_lower_half);
		int const bottom = std::min(h, y + window_upper_half); // exclusive
		
		for (int x = 0; x < w; ++x) {
			int const left = std::max(0, x - window_left_half);
			int const right = std::min(w, x + window_right_half); // exclusive
			int const area = (bottom - top) * (right - left);
			assert(area > 0); // because window_size > 0 and w > 0 and h > 0
			
			QRect const rect(left, top, right - left, bottom - top);
			double const window_sum = integral_image.sum(rect);
			double const window_sqsum = integral_sqimage.sum(rect);
			
			double const r_area = 1.0 / area;
			double const mean = window_sum * r_area;
			double const sqmean = window_sqsum * r_area;
			
			double const variance = sqmean - mean * mean;
			double const deviation = sqrt(fabs(variance));
			
			double const k = 0.34;
			double const threshold = mean * (1.0 + k * (deviation / 128.0 - 1.0));
			
			uint32_t const msb = uint32_t(1) << 31;
			uint32_t const mask = msb >> (x & 31);
			if (int(gray_line[x]) < threshold) {
				// black
				bw_line[x >> 5] |= mask;
			} else {
				// white
				bw_line[x >> 5] &= ~mask;
			}
		}
		
		gray_line += gray_bpl;
		bw_line += bw_wpl;
	}
	
	return bw_img;
}

BinaryImage binarizeWolf(
	QImage const& src, QSize const window_size,
	unsigned char const lower_bound, unsigned char const upper_bound)
{
	if (window_size.isEmpty()) {
		throw std::invalid_argument("binarizeWolf: invalid window_size");
	}
	
	if (src.isNull()) {
		return BinaryImage();
	}
	
	QImage const gray(toGrayscale(src));
	int const w = gray.width();
	int const h = gray.height();
	
	IntegralImage<uint32_t> integral_image(w, h);
	IntegralImage<uint64_t> integral_sqimage(w, h);
	
	uint8_t const* gray_line = gray.bits();
	int const gray_bpl = gray.bytesPerLine();
	
	uint32_t min_gray_level = 255;
	
	for (int y = 0; y < h; ++y, gray_line += gray_bpl) {
		integral_image.beginRow();
		integral_sqimage.beginRow();
		for (int x = 0; x < w; ++x) {
			uint32_t const pixel = gray_line[x];
			integral_image.push(pixel);
			integral_sqimage.push(pixel * pixel);
			min_gray_level = std::min(min_gray_level, pixel);
		}
	}
	
	int const window_lower_half = window_size.height() >> 1;
	int const window_upper_half = window_size.height() - window_lower_half;
	int const window_left_half = window_size.width() >> 1;
	int const window_right_half = window_size.width() - window_left_half;
	
	std::vector<float> means(w * h, 0);
	std::vector<float> deviations(w * h, 0);
	
	double max_deviation = 0;
	
	for (int y = 0; y < h; ++y) {
		int const top = std::max(0, y - window_lower_half);
		int const bottom = std::min(h, y + window_upper_half); // exclusive
		
		for (int x = 0; x < w; ++x) {
			int const left = std::max(0, x - window_left_half);
			int const right = std::min(w, x + window_right_half); // exclusive
			int const area = (bottom - top) * (right - left);
			assert(area > 0); // because window_size > 0 and w > 0 and h > 0
			
			QRect const rect(left, top, right - left, bottom - top);
			double const window_sum = integral_image.sum(rect);
			double const window_sqsum = integral_sqimage.sum(rect);
			
			double const r_area = 1.0 / area;
			double const mean = window_sum * r_area;
			double const sqmean = window_sqsum * r_area;
			
			double const variance = sqmean - mean * mean;
			double const deviation = sqrt(fabs(variance));
			max_deviation = std::max(max_deviation, deviation);
			means[w * y + x] = mean;
			deviations[w * y + x] = deviation;
		}
	}
	
	// TODO: integral images can be disposed at this point.
	
	BinaryImage bw_img(w, h);
	uint32_t* bw_line = bw_img.data();
	int const bw_wpl = bw_img.wordsPerLine();
	
	gray_line = gray.bits();
	for (int y = 0; y < h; ++y, gray_line += gray_bpl, bw_line += bw_wpl) {
		for (int x = 0; x < w; ++x) {
			float const mean = means[y * w + x];
			float const deviation = deviations[y * w + x];
			double const k = 0.3;
			double const a = 1.0 - deviation / max_deviation;
			double const threshold = mean - k * a * (mean - min_gray_level);
			
			uint32_t const msb = uint32_t(1) << 31;
			uint32_t const mask = msb >> (x & 31);
			if (gray_line[x] < lower_bound ||
					(gray_line[x] <= upper_bound &&
					int(gray_line[x]) < threshold)) {
				// black
				bw_line[x >> 5] |= mask;
			} else {
				// white
				bw_line[x >> 5] &= ~mask;
			}
		}
	}
	
	return bw_img;
}

} // namespace imageproc
