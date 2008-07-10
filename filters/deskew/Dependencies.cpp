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
#include "imageproc/PolygonUtils.h"
#include <QDomDocument>
#include <QDomElement>

using namespace imageproc;

namespace deskew
{

Dependencies::Dependencies()
{
}

Dependencies::Dependencies(
	QPolygonF const& page_outline, OrthogonalRotation const rotation)
:	m_pageOutline(page_outline),
	m_rotation(rotation)
{
}

Dependencies::Dependencies(QDomElement const& deps_el)
:	m_pageOutline(
		XmlUnmarshaller::polygonF(
			deps_el.namedItem("page-outline").toElement()
		)
	),
	m_rotation(
		XmlUnmarshaller::rotation(
			deps_el.namedItem("rotation").toElement()
		)
	)
{
}

Dependencies::~Dependencies()
{
}

bool
Dependencies::matches(Dependencies const& other) const
{
	if (m_rotation != other.m_rotation) {
		return false;
	}
	if (!PolygonUtils::fuzzyCompare(m_pageOutline, other.m_pageOutline)) {
		return false;
	}
	return true;
}

QDomElement
Dependencies::toXml(QDomDocument& doc, QString const& name) const
{
	XmlMarshaller marshaller(doc);
	
	QDomElement el(doc.createElement(name));
	el.appendChild(marshaller.rotation(m_rotation, "rotation"));
	el.appendChild(marshaller.polygonF(m_pageOutline, "page-outline"));
	
	return el;
}

} // namespace deskew
