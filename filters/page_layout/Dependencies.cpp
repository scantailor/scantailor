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

namespace page_layout
{

Dependencies::Dependencies()
{
}

Dependencies::Dependencies(QPolygonF const& physical_content_box)
:	m_physicalContentBox(physical_content_box)
{
}

Dependencies::Dependencies(QDomElement const& deps_el)
:	m_physicalContentBox(
		XmlUnmarshaller::polygonF(
			deps_el.namedItem("physical-content-box").toElement()
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
	return PolygonUtils::fuzzyCompare(
		m_physicalContentBox, other.m_physicalContentBox
	);
}

QDomElement
Dependencies::toXml(QDomDocument& doc, QString const& name) const
{
	XmlMarshaller marshaller(doc);
	
	QDomElement el(doc.createElement(name));
	el.appendChild(
		marshaller.polygonF(
			m_physicalContentBox, "physical-content-box"
		)
	);
	
	return el;
}

} // namespace page_layout
