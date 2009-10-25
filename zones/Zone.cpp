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

#include "Zone.h"
#include <QDomDocument>
#include <QDomElement>
#include <QString>

Zone::Zone(SerializableSpline const& spline, PropertySet const& props)
:	m_spline(spline),
	m_props(props)
{
}

Zone::Zone(QDomElement const& el, PropertyFactory const& prop_factory)
:	m_spline(el.namedItem("spline").toElement()),
	m_props(el.namedItem("properties").toElement(), prop_factory)
{
}

QDomElement
Zone::toXml(QDomDocument& doc, QString const& name) const
{
	QDomElement el(doc.createElement(name));
	el.appendChild(m_spline.toXml(doc, "spline"));
	el.appendChild(m_props.toXml(doc, "properties"));
	return el;
}

bool
Zone::isValid() const
{
	QPolygonF const& shape = m_spline.toPolygon();

	switch (shape.size()) {
		case 0:
		case 1:
		case 2:
			return false;
		case 3:
			if (shape.front() == shape.back()) {
				return false;
			}
			// fall through
		default:
			return true;
	}
}
