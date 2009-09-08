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

#include "PictureZone.h"
#include <QString>
#include <QDomDocument>
#include <QDomElement>

namespace output
{

PictureZone::PictureZone(QPolygonF const& shape, Type type)
:	Zone(shape),
	m_type(type)
{
}

PictureZone::PictureZone(QDomElement const& el)
:	Zone(el),
	m_type(typeFromString(el.attribute("type")))
{
}

QDomElement
PictureZone::toXml(QDomDocument& doc, QString const& name) const
{
	QDomElement el(Zone::toXml(doc, name));
	el.setAttribute("type", typeToString(m_type));
	return el;
}

QString
PictureZone::typeToString(Type type)
{
	switch (type) {
		case NO_OP:
			return "0";
		case ERASER1:
			return "1";
		case PAINTER2:
			return "2";
		case ERASER3:
			return "3";
	}

	return "0";
}

PictureZone::Type
PictureZone::typeFromString(QString const& str)
{
	if (str == "1") {
		return ERASER1;
	} else if (str == "2") {
		return PAINTER2;
	} else if (str == "3") {
		return ERASER3;
	} else {
		return NO_OP;
	}
}

} // namespace output
