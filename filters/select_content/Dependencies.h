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

#ifndef SELECT_CONTENT_DEPENDENCIES_H_
#define SELECT_CONTENT_DEPENDENCIES_H_

#include <QPolygonF>

class QDomDocument;
class QDomElement;
class QString;

namespace select_content
{

/**
 * \brief Dependencies of the content box.
 *
 * Once dependencies change, the content box is no longer valid.
 */
class Dependencies
{
public:
	// Member-wise copying is OK.
	
	Dependencies();
	
	Dependencies(QPolygonF const& rotated_page_outline);
	
	Dependencies(QDomElement const& deps_el);
	
	~Dependencies();
	
	QPolygonF const& rotatedPageOutline() const { return m_rotatedPageOutline; }
	
	bool matches(Dependencies const& other) const;
	
	QDomElement toXml(QDomDocument& doc, QString const& name) const;
private:
	QPolygonF m_rotatedPageOutline;
};

} // namespace select_content

#endif
