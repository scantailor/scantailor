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

#include "Dependencies.h"
#include "XmlMarshaller.h"
#include "XmlUnmarshaller.h"
#include <QString>
#include <QDomDocument>
#include <QDomElement>
#include <QDomText>

namespace page_split
{

Dependencies::Dependencies()
{
}

Dependencies::Dependencies(QDomElement const& el)
:	m_imageSize(XmlUnmarshaller::size(el.namedItem("size").toElement())),
	m_rotation(XmlUnmarshaller::rotation(el.namedItem("rotation").toElement()))
{
}

Dependencies::Dependencies(
	QSize const& image_size, OrthogonalRotation const rotation)
:	m_imageSize(image_size),
	m_rotation(rotation)
{
}

bool
Dependencies::matches(Dependencies const& other) const
{
	if (m_imageSize != other.m_imageSize) {
		return false;
	}
	if (m_rotation != other.m_rotation) {
		return false;
	}
	return true;
}

QDomElement
Dependencies::toXml(QDomDocument& doc, QString const& tag_name) const
{
	if (isNull()) {
		return QDomElement();
	}
	
	XmlMarshaller marshaller(doc);
	
	QDomElement el(doc.createElement(tag_name));
	el.appendChild(marshaller.rotation(m_rotation, "rotation"));
	el.appendChild(marshaller.size(m_imageSize, "size"));
	
	return el;
}

} // namespace page_split
