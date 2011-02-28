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

#include "Alignment.h"
#include <QDomDocument>
#include <QDomElement>
#include <QString>

namespace page_layout
{

Alignment::Alignment(QDomElement const& el)
{
	QString const vert(el.attribute("vert"));
	QString const hor(el.attribute("hor"));
	m_isNull = el.attribute("null").toInt() != 0;
	
	if (vert == "top") {
		m_vert = TOP;
	} else if (vert == "bottom") {
		m_vert = BOTTOM;
	} else {
		m_vert = VCENTER;
	}
	
	if (hor == "left") {
		m_hor = LEFT;
	} else if (hor == "right") {
		m_hor = RIGHT;
	} else {
		m_hor = HCENTER;
	}
}

QDomElement
Alignment::toXml(QDomDocument& doc, QString const& name) const
{
	char const* vert = 0;
	switch (m_vert) {
		case TOP:
			vert = "top";
			break;
		case VCENTER:
			vert = "vcenter";
			break;
		case BOTTOM:
			vert = "bottom";
			break;
	}
	
	char const* hor = 0;
	switch (m_hor) {
		case LEFT:
			hor = "left";
			break;
		case HCENTER:
			hor = "hcenter";
			break;
		case RIGHT:
			hor = "right";
			break;
	}
	
	QDomElement el(doc.createElement(name));
	el.setAttribute("vert", QString::fromAscii(vert));
	el.setAttribute("hor", QString::fromAscii(hor));
	el.setAttribute("null", m_isNull ? 1 : 0);
	return el;
}

} // namespace page_layout

