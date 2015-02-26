/*
	Scan Tailor - Interactive post-processing tool for scanned pages.
	Copyright (C) 2007-2009  Joseph Artsimovich <joseph_a@mail.ru>

	ZoneCategoryProperty: made by monday2000 <monday2000@yandex.ru>

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

#include "ZoneCategoryProperty.h"
#include "PropertyFactory.h"
#include <QDomDocument>
#include <QDomDocument>
#include <QString>

namespace output
{

char const ZoneCategoryProperty::m_propertyName[] = "ZoneCategoryProperty";

ZoneCategoryProperty::ZoneCategoryProperty(QDomElement const& el)
:	m_zone_category(zoneCategoryFromString(el.attribute("zone_category")))
{
}

void
ZoneCategoryProperty::registerIn(PropertyFactory& factory)
{
	factory.registerProperty(m_propertyName, &ZoneCategoryProperty::construct);
}

IntrusivePtr<Property>
ZoneCategoryProperty::clone() const
{
	return IntrusivePtr<Property>(new ZoneCategoryProperty(*this));
}

QDomElement
ZoneCategoryProperty::toXml(QDomDocument& doc, QString const& name) const
{
	QDomElement el(doc.createElement(name));
	el.setAttribute("type", m_propertyName);
	el.setAttribute("zone_category", zoneCategoryToString(m_zone_category));
	return el;
}

IntrusivePtr<Property>
ZoneCategoryProperty::construct(QDomElement const& el)
{
	return IntrusivePtr<Property>(new ZoneCategoryProperty(el));
}

ZoneCategoryProperty::ZoneCategory
ZoneCategoryProperty::zoneCategoryFromString(QString const& str)
{
	if (str == "manual") {
		return MANUAL;
	} else if (str == "rectangular_outline") {
		return RECTANGULAR_OUTLINE;
	}else 
		return MANUAL;
}

QString
ZoneCategoryProperty::zoneCategoryToString(ZoneCategory zone_category)
{
	char const* str = 0;

	switch (zone_category) {
		case MANUAL:
			str = "manual";
			break;
		case RECTANGULAR_OUTLINE:
			str = "rectangular_outline";
			break;
		default:
			str = "";
			break;
	}

	return str;
}

} // namespace output
