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

#include "AdjustBrightness.h"
#include <QImage>
#include <stdexcept>
#include <stdint.h>

namespace imageproc
{

void adjustBrightness(
	QImage& rgb_image, QImage const& brightness,
	double const wr, double const wb)
{
	switch (rgb_image.format()) {
		case QImage::Format_RGB32:
		case QImage::Format_ARGB32:
			break;
		default:
			throw std::invalid_argument("adjustBrightness: not (A)RGB32");
	}
	
	if (brightness.format() != QImage::Format_Indexed8
			|| !brightness.allGray()) {
		throw std::invalid_argument("adjustBrightness: brightness not grayscale");
	}
	
	if (rgb_image.size() != brightness.size()) {
		throw std::invalid_argument("adjustBrightness: image and brightness have different sizes");
	}
	
	uint32_t* rgb_line = reinterpret_cast<uint32_t*>(rgb_image.bits());
	int const rgb_wpl = rgb_image.bytesPerLine() / 4;
	
	uint8_t const* br_line = brightness.bits();
	int const br_bpl = brightness.bytesPerLine();
	
	int const width = rgb_image.width();
	int const height = rgb_image.height();
	
	double const wg = 1.0 - wr - wb;
	double const wu = (1.0 - wb);
	double const wv = (1.0 - wr);
	double const r_wg = 1.0 / wg;
	double const r_wu = 1.0 / wu;
	double const r_wv = 1.0 / wv;
	
	for (int y = 0; y < height; ++y) {
		for (int x = 0; x < width; ++x) {
			uint32_t RGB = rgb_line[x];
			double const R = (RGB >> 16) & 0xFF;
			double const G = (RGB >> 8) & 0xFF;
			double const B = RGB & 0xFF;
			
			double const Y = wr * R + wg * G + wb * B;
			double const U = (B - Y) * r_wu;
			double const V = (R - Y) * r_wv;
			
			double new_Y = br_line[x];
			double new_R = new_Y + V * wv;
			double new_B = new_Y + U * wu;
			double new_G = (new_Y - new_R * wr - new_B * wb) * r_wg;
			
			RGB &= 0xFF000000; // preserve alpha
			RGB |= uint32_t(qBound(0, int(new_R + 0.5), 255)) << 16;
			RGB |= uint32_t(qBound(0, int(new_G + 0.5), 255)) << 8;
			RGB |= uint32_t(qBound(0, int(new_B + 0.5), 255));
			rgb_line[x] = RGB;
		}
		rgb_line += rgb_wpl;
		br_line += br_bpl;
	}
}

void adjustBrightnessYUV(QImage& rgb_image, QImage const& brightness)
{
	adjustBrightness(rgb_image, brightness, 0.299, 0.114);
}

void adjustBrightnessGrayscale(QImage& rgb_image, QImage const& brightness)
{
	adjustBrightness(rgb_image, brightness, 11.0/32.0, 5.0/32.0);
}

} // namespace imageproc
