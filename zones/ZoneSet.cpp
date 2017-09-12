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

#include "ZoneSet.h"
#include <QDomNode>
#include <QDomDocument>
#include <QDomElement>
#include <QString>
#ifndef Q_MOC_RUN
#include <boost/foreach.hpp>
#endif

ZoneSet::ZoneSet(QDomElement const& el, PropertyFactory const& prop_factory)
{
	QString const zone_str("zone");

	QDomNode node(el.firstChild());
	for (; !node.isNull(); node = node.nextSibling()) {
		if (!node.isElement()) {
			continue;
		}
		if (node.nodeName() != zone_str) {
			continue;
		}

		Zone const zone(node.toElement(), prop_factory);
		if (zone.isValid()) {
			m_zones.push_back(zone);
		}
	}
}

QDomElement
ZoneSet::toXml(QDomDocument& doc, QString const& name) const
{
	QString const zone_str("zone");

	QDomElement el(doc.createElement(name));
	BOOST_FOREACH(Zone const& zone, m_zones) {
		el.appendChild(zone.toXml(doc, zone_str));
	}
	return el;
}
