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

#ifndef DPI_H_
#define DPI_H_

#include <QSize>

class Dpm;

/**
 * \brief Dots per inch (horizontal and vertical).
 */
class Dpi
{
public:
	Dpi() : m_xDpi(0), m_yDpi(0) {}
	
	Dpi(int horizontal, int vertical) : m_xDpi(horizontal), m_yDpi(vertical) {}
	
	Dpi(Dpm dpm);
	
	explicit Dpi(QSize size);
	
	int horizontal() const { return m_xDpi; }
	
	int vertical() const { return m_yDpi; }
	
	QSize toSize() const;
	
	bool isNull() const { return m_xDpi <= 1 || m_yDpi <= 1; }
	
	bool operator==(Dpi const& other) const;
	
	bool operator!=(Dpi const& other) const { return !(*this == other); }
private:
	int m_xDpi;
	int m_yDpi;
};

#endif
