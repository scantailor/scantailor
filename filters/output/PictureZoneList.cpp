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

#include "PictureZoneList.h"
#include <QDomDocument>
#include <QDomElement>
#include <QString>
#include <boost/foreach.hpp>

namespace output
{

PictureZoneList::PictureZoneList(QDomElement const& el)
{
	QString const zone_tag_name("zone");
	QDomNode node(el.firstChild());
	for (; !node.isNull(); node = node.nextSibling()) {
		if (!node.isElement()) {
			continue;
		}
		if (node.nodeName() != zone_tag_name) {
			continue;
		}

		PictureZone const zone(node.toElement());
		if (zone.isValid()) {
			push_back(zone);
		}
	}
}

QDomElement
PictureZoneList::toXml(QDomDocument& doc, QString const& name) const
{
	if (empty()) {
		return QDomElement();
	}

	QDomElement el(doc.createElement(name));
	BOOST_FOREACH(PictureZone const& zone, *this) {
		el.appendChild(zone.toXml(doc, "zone"));
	}
	return el;
}

} // namespace output

