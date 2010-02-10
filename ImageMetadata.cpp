/*
    Scan Tailor - Interactive post-processing tool for scanned pages.
    Copyright (C) 2007-2009  Joseph Artsimovich <joseph_a@mail.ru>

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

#include "ImageMetadata.h"
#include "imageproc/Constants.h"

using namespace imageproc::constants;

bool
ImageMetadata::operator==(ImageMetadata const& other) const
{
	if (m_size != other.m_size) {
		return false;
	} else if (m_dpi.isNull() && other.m_dpi.isNull()) {
		return true;
	} else {
		return m_dpi == other.m_dpi;
	}
}

bool
ImageMetadata::isDpiOK() const
{
	return horizontalDpiStatus() == DPI_OK && verticalDpiStatus() == DPI_OK;
}

ImageMetadata::DpiStatus
ImageMetadata::horizontalDpiStatus() const
{
	return dpiStatus(m_size.width(), m_dpi.horizontal());
}

ImageMetadata::DpiStatus
ImageMetadata::verticalDpiStatus() const
{
	return dpiStatus(m_size.height(), m_dpi.vertical());
}

ImageMetadata::DpiStatus
ImageMetadata::dpiStatus(int pixel_size, int dpi)
{
	if (dpi <= 1) {
		return DPI_UNDEFINED;
	}
	
	if (dpi < 150) {
		return DPI_TOO_SMALL;
	}

	if (dpi > 9999) {
		return DPI_TOO_LARGE;
	}
	
	double const mm = INCH2MM * pixel_size / dpi;
	if (mm > 500) {
		// This may indicate we are working with very large printed materials,
		// but most likely it indicates the DPI is wrong (too low).
		// DPIs that are too low may easily cause crashes due to out of memory
		// conditions.  The memory consumption is proportional to:
		// (real_hor_dpi / provided_hor_dpi) * (real_vert_dpi / provided_vert_dpi).
		// For example, if the real DPI is 600x600 but 200x200 is specified,
		// memory consumption is increased 9 times.
		return DPI_TOO_SMALL_FOR_THIS_PIXEL_SIZE;
	}
	
	return DPI_OK;
}
