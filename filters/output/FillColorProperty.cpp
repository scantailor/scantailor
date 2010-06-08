/*
	Scan Tailor - Interactive post-processing tool for scanned pages.
	Copyright (C)  Joseph Artsimovich <joseph.artsimovich@gmail.com>

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

#include "FillColorProperty.h"
#include "PropertyFactory.h"
#include <QDomDocument>
#include <QDomDocument>
#include <QString>

namespace output
{

char const FillColorProperty::m_propertyName[] = "FillColorProperty";

FillColorProperty::FillColorProperty(QDomElement const& el)
:	m_rgb(rgbFromString(el.attribute("color")))
{
}

void
FillColorProperty::registerIn(PropertyFactory& factory)
{
	factory.registerProperty(m_propertyName, &FillColorProperty::construct);
}

IntrusivePtr<Property>
FillColorProperty::clone() const
{
	return IntrusivePtr<Property>(new FillColorProperty(*this));
}

QDomElement
FillColorProperty::toXml(QDomDocument& doc, QString const& name) const
{
	QDomElement el(doc.createElement(name));
	el.setAttribute("type", m_propertyName);
	el.setAttribute("color", rgbToString(m_rgb));
	return el;
}

IntrusivePtr<Property>
FillColorProperty::construct(QDomElement const& el)
{
	return IntrusivePtr<Property>(new FillColorProperty(el));
}

QRgb
FillColorProperty::rgbFromString(QString const& str)
{
	return QColor(str).rgb();
}

QString
FillColorProperty::rgbToString(QRgb rgb)
{
	return QColor(rgb).name();
}

} // namespace output
