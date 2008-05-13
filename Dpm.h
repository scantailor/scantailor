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

#ifndef DPM_H_
#define DPM_H_

#include <QSize>

class Dpi;
class QImage;

/**
 * \brief Dots per meter (horizontal and vertical).
 */
class Dpm
{
	// Member-wise copying is OK.
public:
	Dpm() : m_xDpm(0), m_yDpm(0) {}
	
	Dpm(int horizontal, int vertical) : m_xDpm(horizontal), m_yDpm(vertical) {}
	
	Dpm(Dpi dpi);
	
	explicit Dpm(QSize size);
	
	explicit Dpm(QImage const& image);
	
	int horizontal() const { return m_xDpm; }
	
	int vertical() const { return m_yDpm; }
	
	QSize toSize() const;
	
	bool isNull() const;
	
	bool operator==(Dpm const& other) const;
	
	bool operator!=(Dpm const& other) const { return !(*this == other); }
private:
	int m_xDpm;
	int m_yDpm;
};

#endif
