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

#ifndef PAGE_LAYOUT_ALIGNMENT_H_
#define PAGE_LAYOUT_ALIGNMENT_H_

namespace page_layout
{

class Alignment
{
public:
	enum Vertical { TOP, VCENTER, BOTTOM };
	
	enum Horizontal { LEFT, HCENTER, RIGHT };
	
	/**
	 * \brief Constructs a null alignment.
	 */
	Alignment()
	: m_vert(VCENTER), m_hor(HCENTER), m_isNull(true) {}
	
	Alignment(Vertical vert, Horizontal hor)
	: m_vert(vert), m_hor(hor), m_isNull(false) {}
	
	Vertical vertical() const { return m_vert; }
	
	Horizontal horizontal() const { return m_hor; }
	
	bool isNull() const { return m_isNull; }
private:
	Vertical m_vert;
	Horizontal m_hor;
	bool m_isNull;
};

} // namespace page_layout

#endif
