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

#include "DespeckleVisualization.h"
#include "ImageViewBase.h"
#include "Dpi.h"
#include "imageproc/BinaryImage.h"
#include "imageproc/SEDM.h"
#include <QPainter>
#include <stdint.h>

using namespace imageproc;

namespace output
{

DespeckleVisualization::DespeckleVisualization(
	QImage const& output, imageproc::BinaryImage const& speckles, Dpi const& dpi)
{
	if (output.isNull()) {
		// This can happen in batch processing mode.
		return;
	}

	m_image = output.convertToFormat(QImage::Format_RGB32);

	if (!speckles.isNull()) {
		colorizeSpeckles(m_image, speckles, dpi);
	}

	m_downscaledImage = ImageViewBase::createDownscaledImage(m_image);
}

void
DespeckleVisualization::colorizeSpeckles(
	QImage& image, imageproc::BinaryImage const& speckles, Dpi const& dpi)
{
	int const w = image.width();
	int const h = image.height();
	uint32_t* image_line = (uint32_t*)image.bits();
	int const image_stride = image.bytesPerLine() / 4;

	SEDM const sedm(speckles, SEDM::DIST_TO_BLACK, SEDM::DIST_TO_NO_BORDERS);
	uint32_t const* sedm_line = sedm.data();
	int const sedm_stride = sedm.stride();

	float const radius = 15.0 * std::max(dpi.horizontal(), dpi.vertical()) / 600;
	float const sq_radius = radius * radius;

	for (int y = 0; y < h; ++y) {
		for (int x = 0; x < w; ++x) {
			uint32_t const sq_dist = sedm_line[x];
			if (sq_dist == 0) {
				// Speckle pixel.
				image_line[x] = 0xffff0000; // opaque red
				continue;
			} else if ((image_line[x] & 0x00ffffff) == 0x0) {
				// Non-speckle black pixel.
				continue;
			}

			float const alpha_upper_bound = 0.8f;
			float const scale = alpha_upper_bound / sq_radius;
			float const alpha = alpha_upper_bound - scale * sq_dist;
			if (alpha > 0) {
				float const alpha2 = 1.0f - alpha;
				float const overlay_r = 255;
				float const overlay_g = 0;
				float const overlay_b = 0;
				float const r = overlay_r * alpha + qRed(image_line[x]) * alpha2;
				float const g = overlay_g * alpha + qGreen(image_line[x]) * alpha2;
				float const b = overlay_b * alpha + qBlue(image_line[x]) * alpha2;
				image_line[x] = qRgb(int(r), int(g), int(b));
			}
		}
		sedm_line += sedm_stride;
		image_line += image_stride;
	}
}

} // namespace output
