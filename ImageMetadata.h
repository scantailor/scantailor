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

#ifndef IMAGEMETADATA_H_
#define IMAGEMETADATA_H_

#include <QSize>
#include "Dpi.h"

class ImageMetadata
{
	// Member-wise copying is OK.
public:
	enum DpiStatus {
		DPI_OK,
		DPI_UNDEFINED,
		DPI_TOO_LARGE,
		DPI_TOO_SMALL,
		DPI_TOO_SMALL_FOR_THIS_PIXEL_SIZE
	};
	
	ImageMetadata() {}
	
	ImageMetadata(QSize size, Dpi dpi) : m_size(size), m_dpi(dpi) {}
	
	QSize const& size() const { return m_size; }
	
	void setSize(QSize const& size) { m_size = size; }
	
	Dpi const& dpi() const { return m_dpi; }
	
	void setDpi(Dpi const& dpi) { m_dpi = dpi; }
	
	bool isDpiOK() const;
	
	DpiStatus horizontalDpiStatus() const;
	
	DpiStatus verticalDpiStatus() const;
	
	bool operator==(ImageMetadata const& other) const;
	
	bool operator!=(ImageMetadata const& other) const {
		return !(*this == other);
	}
private:
	static DpiStatus dpiStatus(int pixel_size, int dpi);
	
	QSize m_size;
	Dpi m_dpi;
};

#endif
