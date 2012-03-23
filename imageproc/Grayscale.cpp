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

#include "Grayscale.h"
#include "GrayImage.h"
#include "BinaryImage.h"
#include "BitOps.h"
#include <QImage>
#include <QColor>
#include <QtGlobal>
#include <stdexcept>
#include <algorithm>
#include <new>
#include <string.h>
#include <stdint.h>

namespace imageproc
{

static QImage monoMsbToGrayscale(QImage const& src)
{
	int const width = src.width();
	int const height = src.height();
	
	QImage dst(width, height, QImage::Format_Indexed8);
	dst.setColorTable(createGrayscalePalette());
	if (width > 0 && height > 0 && dst.isNull()) {
		throw std::bad_alloc();
	}
	
	uint8_t const* src_line = src.bits();
	uint8_t* dst_line = dst.bits();
	int const src_bpl = src.bytesPerLine();
	int const dst_bpl = dst.bytesPerLine();
	
	uint8_t bin2gray[2] = { 0, 0xff };
	if (src.numColors() >= 2) {
		if (qGray(src.color(0)) > qGray(src.color(1))) {
			// if color 0 is lighter than color 1
			bin2gray[0] = 0xff;
			bin2gray[1] = 0;
		}
	}
	
	for (int y = 0; y < height; ++y) {
		for (int x = 0; x < width;) {
			uint8_t const b = src_line[x / 8];
			for (int i = 7; i >= 0 && x < width; --i, ++x) {
				dst_line[x] = bin2gray[(b >> i) & 1];
			}
		}
		
		src_line += src_bpl;
		dst_line += dst_bpl;
	}
	
	dst.setDotsPerMeterX(src.dotsPerMeterX());
	dst.setDotsPerMeterY(src.dotsPerMeterY());
	
	return dst;
}

static QImage monoLsbToGrayscale(QImage const& src)
{
	int const width = src.width();
	int const height = src.height();
	
	QImage dst(width, height, QImage::Format_Indexed8);
	dst.setColorTable(createGrayscalePalette());
	if (width > 0 && height > 0 && dst.isNull()) {
		throw std::bad_alloc();
	}
	
	uint8_t const* src_line = src.bits();
	uint8_t* dst_line = dst.bits();
	int const src_bpl = src.bytesPerLine();
	int const dst_bpl = dst.bytesPerLine();
	
	uint8_t bin2gray[2] = { 0, 0xff };
	if (src.numColors() >= 2) {
		if (qGray(src.color(0)) > qGray(src.color(1))) {
			// if color 0 is lighter than color 1
			bin2gray[0] = 0xff;
			bin2gray[1] = 0;
		}
	}
	
	for (int y = 0; y < height; ++y) {
		for (int x = 0; x < width;) {
			uint8_t const b = src_line[x / 8];
			for (int i = 0; i < 8 && x < width; ++i, ++x) {
				dst_line[x] = bin2gray[(b >> i) & 1];
			}
		}
		
		src_line += src_bpl;
		dst_line += dst_bpl;
	}
	
	dst.setDotsPerMeterX(src.dotsPerMeterX());
	dst.setDotsPerMeterY(src.dotsPerMeterY());
	
	return dst;
}

static QImage anyToGrayscale(QImage const& src)
{
	int const width = src.width();
	int const height = src.height();
	
	QImage dst(width, height, QImage::Format_Indexed8);
	dst.setColorTable(createGrayscalePalette());
	if (width > 0 && height > 0 && dst.isNull()) {
		throw std::bad_alloc();
	}
	
	uint8_t* dst_line = dst.bits();
	int const dst_bpl = dst.bytesPerLine();
	
	for (int y = 0; y < height; ++y) {
		for (int x = 0; x < width; ++x) {
			dst_line[x] = static_cast<uint8_t>(qGray(src.pixel(x, y)));
		}
		dst_line += dst_bpl;
	}
	
	dst.setDotsPerMeterX(src.dotsPerMeterX());
	dst.setDotsPerMeterY(src.dotsPerMeterY());
	
	return dst;
}

QVector<QRgb> createGrayscalePalette()
{
	QVector<QRgb> palette(256);
	for (int i = 0; i < 256; ++i) {
		palette[i] = qRgb(i, i, i);
	}
	return palette;
}

QImage toGrayscale(QImage const& src)
{
	if (src.isNull()) {
		return src;
	}
	
	switch (src.format()) {
	case QImage::Format_Mono:
		return monoMsbToGrayscale(src);
	case QImage::Format_MonoLSB:
		return monoLsbToGrayscale(src);
	case QImage::Format_Indexed8:
		if (src.isGrayscale()) {
			if (src.numColors() == 256) {
				return src;
			} else {
				QImage dst(src);
				dst.setColorTable(createGrayscalePalette());
				if (!src.isNull() && dst.isNull()) {
					throw std::bad_alloc();
				}
				return dst;
			}
		}
		// fall though
	default:
		return anyToGrayscale(src);
	}
}

GrayImage stretchGrayRange(
	GrayImage const& src,
	double const black_clip_fraction, double const white_clip_fraction)
{
	if (src.isNull()) {
		return src;
	}
	
	GrayImage dst(src);
	
	int const width = dst.width();
	int const height = dst.height();
	
	int const num_pixels = width * height;
	int black_clip_pixels = qRound(black_clip_fraction * num_pixels);
	int white_clip_pixels = qRound(white_clip_fraction * num_pixels);
	
	GrayscaleHistogram const hist(dst);
	
	int min = 0;
	for (; min <= 255; ++min) {
		if (black_clip_pixels < hist[min]) {
			break;
		}
		black_clip_pixels -= hist[min];
	}
	
	int max = 255;
	for (; max >= 0; --max) {
		if (white_clip_pixels < hist[max]) {
			break;
		}
		white_clip_pixels -= hist[max];
	}
	
	uint8_t gray_mapping[256];
	
	if (min >= max) {
		int const avg = (min + max) / 2;
		for (int i = 0; i <= avg; ++i) {
			gray_mapping[i] = 0;
		}
		for (int i = avg + 1; i < 256; ++i) {
			gray_mapping[i] = 255;
		}
	} else {
		for (int i = 0; i < 256; ++i) {
			int const src_level = qBound(min, i, max);
			int const num = 255 * (src_level - min);
			int const denom = max - min;
			int const dst_level = (num + denom / 2) / denom;
			gray_mapping[i] = static_cast<uint8_t>(dst_level);
		}
	}
	
	uint8_t* line = dst.data();
	int const stride = dst.stride();
	
	for (int y = 0; y < height; ++y, line += stride) {
		for (int x = 0; x < width; ++x) {
			line[x] = gray_mapping[line[x]];
		}
	}
	
	return dst;
}

GrayImage createFramedImage(QSize const& size,
	unsigned char const inner_color, unsigned char const frame_color)
{
	GrayImage image(size);
	image.fill(inner_color);
	
	int const width = size.width();
	int const height = size.height();
	
	unsigned char* line = image.data();
	int const stride = image.stride();
	
	memset(line, frame_color, width);
	
	for (int y = 0; y < height; ++y, line += stride) {
		line[0] = frame_color;
		line[width - 1] = frame_color;
	}
	
	memset(line - stride, frame_color, width);
	
	return image;
}

unsigned char darkestGrayLevel(QImage const& image)
{
	QImage const gray(toGrayscale(image));
	
	int const width = image.width();
	int const height = image.height();
	
	unsigned char const* line = image.bits();
	int const bpl = image.bytesPerLine();
	
	unsigned char darkest = 0xff;
	
	for (int y = 0; y < height; ++y) {
		for (int x = 0; x < width; ++x) {
			darkest = std::min(darkest, line[x]);
		}
		line += bpl;
	}
	
	return darkest;
}

GrayscaleHistogram::GrayscaleHistogram(QImage const& img)
{
	memset(m_pixels, 0, sizeof(m_pixels));
	
	if (img.isNull()) {
		return;
	}
	
	switch (img.format()) {
	case QImage::Format_Mono:
	case QImage::Format_MonoLSB:
		fromMonoImage(img);
		break;
	case QImage::Format_Indexed8:
		if (img.isGrayscale()) {
			fromGrayscaleImage(img);
			break;
		}
		// fall though
	default:
		fromAnyImage(img);
	}
}

GrayscaleHistogram::GrayscaleHistogram(
	QImage const& img, BinaryImage const& mask)
{
	memset(m_pixels, 0, sizeof(m_pixels));
	
	if (img.isNull()) {
		return;
	}
	
	if (img.size() != mask.size()) {
		throw std::invalid_argument(
			"GrayscaleHistogram: img and mask have different sizes"
		);
	}
	
	switch (img.format()) {
	case QImage::Format_Mono:
		fromMonoMSBImage(img, mask);
		break;
	case QImage::Format_MonoLSB:
		fromMonoMSBImage(img.convertToFormat(QImage::Format_Mono), mask);
		break;
	case QImage::Format_Indexed8:
		if (img.isGrayscale()) {
			fromGrayscaleImage(img, mask);
			break;
		}
		// fall though
	default:
		fromAnyImage(img, mask);
	}
}

void
GrayscaleHistogram::fromMonoImage(QImage const& img)
{
	int const w = img.width();
	int const h = img.height();
	int const bpl = img.bytesPerLine();
	int const last_byte_idx = (w - 1) >> 3;
	int const last_byte_unused_bits = (((last_byte_idx + 1) << 3) - w);
	uint8_t last_byte_mask = ~uint8_t(0);
	if (img.format() == QImage::Format_MonoLSB) {
		last_byte_mask >>= last_byte_unused_bits;
	} else {
		last_byte_mask <<= last_byte_unused_bits;
	}
	uint8_t const* line = img.bits();
	
	int num_bits_1 = 0;
	for (int y = 0; y < h; ++y, line += bpl) {
		int i = 0;
		for (; i < last_byte_idx; ++i) {
			num_bits_1 += countNonZeroBits(line[i]);
		}
		
		// The last (possibly incomplete) byte.
		num_bits_1 += countNonZeroBits(line[i] & last_byte_mask);
	}
	int const num_bits_0 = w * h - num_bits_1;
	
	QRgb color0 = 0xffffffff;
	QRgb color1 = 0xff000000;
	if (img.numColors() >= 2) {
		color0 = img.color(0);
		color1 = img.color(1);
	}
	
	m_pixels[qGray(color0)] = num_bits_0;
	m_pixels[qGray(color1)] = num_bits_1;
}

void
GrayscaleHistogram::fromMonoMSBImage(QImage const& img, BinaryImage const& mask)
{
	int const w = img.width();
	int const h = img.height();
	int const wpl = img.bytesPerLine() >> 2;
	int const last_word_idx = (w - 1) >> 5;
	int const last_word_unused_bits = (((last_word_idx + 1) << 5) - w);
	uint32_t last_word_mask = ~uint32_t(0) << last_word_unused_bits;
	uint32_t const* line = (uint32_t const*)img.bits();
	uint32_t const* mask_line = mask.data();
	int const mask_wpl = mask.wordsPerLine();
	
	int num_bits_0 = 0;
	int num_bits_1 = 0;
	for (int y = 0; y < h; ++y, line += wpl, mask_line += mask_wpl) {
		int i = 0;
		for (; i < last_word_idx; ++i) {
			uint32_t const mask = mask_line[i];
			num_bits_1 += countNonZeroBits(line[i] & mask);
			num_bits_0 += countNonZeroBits(~line[i] & mask);
		}
		
		// The last (possibly incomplete) word.
		uint32_t const mask = mask_line[i] & last_word_mask;
		num_bits_1 += countNonZeroBits(line[i] & mask);
		num_bits_0 += countNonZeroBits(~line[i] & mask);
	}
	
	QRgb color0 = 0xffffffff;
	QRgb color1 = 0xff000000;
	if (img.numColors() >= 2) {
		color0 = img.color(0);
		color1 = img.color(1);
	}
	
	m_pixels[qGray(color0)] = num_bits_0;
	m_pixels[qGray(color1)] = num_bits_1;
}

void
GrayscaleHistogram::fromGrayscaleImage(QImage const& img)
{
	int const w = img.width();
	int const h = img.height();
	int const bpl = img.bytesPerLine();
	uint8_t const* line = img.bits();
	
	for (int y = 0; y < h; ++y, line += bpl) {
		for (int x = 0; x < w; ++x) {
			++m_pixels[line[x]];
		}
	}
}

void
GrayscaleHistogram::fromGrayscaleImage(QImage const& img, BinaryImage const& mask)
{
	int const w = img.width();
	int const h = img.height();
	int const bpl = img.bytesPerLine();
	uint8_t const* line = img.bits();
	uint32_t const* mask_line = mask.data();
	int const mask_wpl = mask.wordsPerLine();
	uint32_t const msb = uint32_t(1) << 31;
	
	for (int y = 0; y < h; ++y, line += bpl, mask_line += mask_wpl) {
		for (int x = 0; x < w; ++x) {
			if (mask_line[x >> 5] & (msb >> (x & 31))) {
				++m_pixels[line[x]];
			}
		}
	}
}

void
GrayscaleHistogram::fromAnyImage(QImage const& img)
{
	int const w = img.width();
	int const h = img.height();
	
	for (int y = 0; y < h; ++y) {
		for (int x = 0; x < w; ++x) {
			++m_pixels[qGray(img.pixel(x, y))];
		}
	}
}

void
GrayscaleHistogram::fromAnyImage(QImage const& img, BinaryImage const& mask)
{
	int const w = img.width();
	int const h = img.height();
	uint32_t const* mask_line = mask.data();
	int const mask_wpl = mask.wordsPerLine();
	uint32_t const msb = uint32_t(1) << 31;
	
	for (int y = 0; y < h; ++y, mask_line += mask_wpl) {
		for (int x = 0; x < w; ++x) {
			if (mask_line[x >> 5] & (msb >> (x & 31))) {
				++m_pixels[qGray(img.pixel(x, y))];
			}
		}
	}
}

} // namespace imageproc
