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

#ifndef PAGE_LAYOUT_DEPENDENCIES_H_
#define PAGE_LAYOUT_DEPENDENCIES_H_

#include <QPolygonF>

class QDomDocument;
class QDomElement;
class QString;

namespace page_layout
{

class Dependencies
{
public:
	// Member-wise copying is OK.
	
	Dependencies();
	
	Dependencies(QPolygonF const& physical_content_box);
	
	Dependencies(QDomElement const& deps_el);
	
	~Dependencies();
	
	bool matches(Dependencies const& other) const;
	
	QDomElement toXml(QDomDocument& doc, QString const& name) const;
private:
	QPolygonF m_physicalContentBox;
};

} // namespace page_layout

#endif
