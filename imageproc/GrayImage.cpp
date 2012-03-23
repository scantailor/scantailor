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

#include "GrayImage.h"
#include "Grayscale.h"
#include <new>

namespace imageproc
{

GrayImage::GrayImage(QSize size)
{
	if (size.isEmpty()) {
		return;
	}

	m_image = QImage(size, QImage::Format_Indexed8);
	m_image.setColorTable(createGrayscalePalette());
	if (m_image.isNull()) {
		throw std::bad_alloc();
	}
}

GrayImage::GrayImage(QImage const& image)
:	m_image(toGrayscale(image))
{
}

} // namespace imageproc
