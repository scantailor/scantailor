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

#include "PictureLayerProperty.h"
#include "PropertyFactory.h"
#include <QDomDocument>
#include <QDomDocument>
#include <QString>

namespace output
{

char const PictureLayerProperty::m_propertyName[] = "PictureZoneProperty";

PictureLayerProperty::PictureLayerProperty(QDomElement const& el)
:	m_layer(layerFromString(el.attribute("layer")))
{
}

void
PictureLayerProperty::registerIn(PropertyFactory& factory)
{
	factory.registerProperty(m_propertyName, &PictureLayerProperty::construct);
}

IntrusivePtr<Property>
PictureLayerProperty::clone() const
{
	return IntrusivePtr<Property>(new PictureLayerProperty(*this));
}

QDomElement
PictureLayerProperty::toXml(QDomDocument& doc, QString const& name) const
{
	QDomElement el(doc.createElement(name));
	el.setAttribute("type", m_propertyName);
	el.setAttribute("layer", layerToString(m_layer));
	return el;
}

IntrusivePtr<Property>
PictureLayerProperty::construct(QDomElement const& el)
{
	return IntrusivePtr<Property>(new PictureLayerProperty(el));
}

PictureLayerProperty::Layer
PictureLayerProperty::layerFromString(QString const& str)
{
	if (str == "eraser1") {
		return ERASER1;
	} else if (str == "painter2") {
		return PAINTER2;
	} else if (str == "eraser3") {
		return ERASER3;
	} else {
		return NO_OP;
	}
}

QString
PictureLayerProperty::layerToString(Layer layer)
{
	char const* str = 0;

	switch (layer) {
		case ERASER1:
			str = "eraser1";
			break;
		case PAINTER2:
			str = "painter2";
			break;
		case ERASER3:
			str = "eraser3";
			break;
		default:
			str = "";
			break;
	}

	return str;
}

} // namespace output
