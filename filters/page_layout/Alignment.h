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

class QDomDocument;
class QDomElement;
class QString;

#include <iostream>

class CommandLine;

namespace page_layout
{

const double DEFAULT_TOLERANCE=0.2;

class Alignment
{
public:
	enum Vertical { TOP, VCENTER, BOTTOM, VAUTO, VORIGINAL };
	
	enum Horizontal { LEFT, HCENTER, RIGHT, HAUTO, HORIGINAL };
	
	/**
	 * \brief Constructs a null alignment.
	 */
	Alignment();
    
    Alignment(Vertical vert, Horizontal hor);
	
	Alignment(QDomElement const& el);
	
	Vertical vertical() const { return m_vert; }
	
	void setVertical(Vertical vert) { m_vert = vert; }
	
	Horizontal horizontal() const { return m_hor; }
	
	void setHorizontal(Horizontal hor) { m_hor = hor; }
	
	bool isNull() const { return m_isNull; }
	
	void setNull(bool is_null) { m_isNull = is_null; }

	double tolerance() const { return m_tolerance; };
	void setTolerance(double t) { m_tolerance = t; };

	bool isAutoMarginsEnabled() const { return m_autoMargins; };
	void setAutoMargins(bool state) { m_autoMargins = state; };
	
	bool operator==(Alignment const& other) const {
		return m_vert == other.m_vert && m_hor == other.m_hor
				&& m_isNull == other.m_isNull && m_autoMargins == other.m_autoMargins;
	}
	
	bool operator!=(Alignment const& other) const {
		return !(*this == other);
	}
	
	QDomElement toXml(QDomDocument& doc, QString const& name) const;
private:
	Vertical m_vert;
	Horizontal m_hor;
	bool m_isNull;
	double m_tolerance;
	bool m_autoMargins;
};

} // namespace page_layout

#endif
