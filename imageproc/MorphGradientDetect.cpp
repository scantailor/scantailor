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

#include "MorphGradientDetect.h"
#include "Morphology.h"
#include "Grayscale.h"
#include <QImage>
#include <QSize>

namespace imageproc
{

QImage morphGradientDetect(QImage const& image, QSize const& window)
{
	QImage const gray(toGrayscale(image));
	QImage const eroded(erodeGray(gray, window, 0x00));
	
	QImage gradient(image.size(), QImage::Format_Indexed8);
	gradient.setColorTable(createGrayscalePalette());
	
	uint8_t const* orig_line = gray.bits();
	int const orig_bpl = gray.bytesPerLine();
	
	uint8_t const* lighter_line = eroded.bits();
	int const lighter_bpl = eroded.bytesPerLine();
	
	uint8_t* gradient_line = gradient.bits();
	int const gradient_bpl = gradient.bytesPerLine();
	
	int const width = image.width();
	int const height = image.height();
	
	for (int y = 0; y < height; ++y) {
		for (int x = 0; x < width; ++x) {
			gradient_line[x] = lighter_line[x] - orig_line[x];
		}
		orig_line += orig_bpl;
		lighter_line += lighter_bpl;
		gradient_line += gradient_bpl;
	}
	
	return gradient;
}

} // namespace imageproc

