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

#ifndef OUTPUT_ZONE_CATEGORY_PROPERTY_H_
#define OUTPUT_ZONE_CATEGORY_PROPERTY_H_

#include "Property.h"
#include "IntrusivePtr.h"

class PropertyFactory;
class QDomDocument;
class QDomElement;
class QString;

namespace output
{

class ZoneCategoryProperty : public Property
{
public:
	enum ZoneCategory { MANUAL, RECTANGULAR_OUTLINE };

	ZoneCategoryProperty(ZoneCategory zone_category = MANUAL) : m_zone_category(zone_category) {}

	ZoneCategoryProperty(QDomElement const& el);

	static void registerIn(PropertyFactory& factory);

	virtual IntrusivePtr<Property> clone() const;

	virtual QDomElement toXml(QDomDocument& doc, QString const& name) const;

	ZoneCategory zone_category() const { return m_zone_category; }

	void setZoneCategory(ZoneCategory zone_category) { m_zone_category = zone_category; }
private:
	static IntrusivePtr<Property> construct(QDomElement const& el);

	static ZoneCategory zoneCategoryFromString(QString const& str);

	static QString zoneCategoryToString(ZoneCategory zone_category);

	static char const m_propertyName[];
	ZoneCategory m_zone_category;
};

} // namespace output

#endif
