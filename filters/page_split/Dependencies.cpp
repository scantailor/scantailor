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
#include "Params.h"
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
	m_rotation(XmlUnmarshaller::rotation(el.namedItem("rotation").toElement())),
	m_layoutType(
		layoutTypeFromString(
			XmlUnmarshaller::string(el.namedItem("layoutType").toElement())
		)
	)
{
}

Dependencies::Dependencies(
	QSize const& image_size, OrthogonalRotation const rotation,
	LayoutType const layout_type)
:	m_imageSize(image_size),
	m_rotation(rotation),
	m_layoutType(layout_type)
{
}

bool
Dependencies::compatibleWith(Params const& params) const
{
	Dependencies const& deps = params.dependencies();
	
	if (m_imageSize != deps.m_imageSize) {
		return false;
	}
	if (m_rotation != deps.m_rotation) {
		return false;
	}
	if (m_layoutType == deps.m_layoutType) {
		return true;
	}
	if (m_layoutType == SINGLE_PAGE_UNCUT) {
		// The split line doesn't matter here.
		return true;
	}
	if (m_layoutType == TWO_PAGES && params.splitLineMode() == MODE_MANUAL) {
		// Two pages and a specified split line means we have all the data.
		// Note that if layout type was PAGE_PLUS_OFFCUT, we would
		// not know if that page is to the left or to the right of the
		// split line.
		return true;
	}
	
	return false;
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
	el.appendChild(marshaller.string(layoutTypeToString(m_layoutType), "layoutType"));
	
	return el;
}

} // namespace page_split
