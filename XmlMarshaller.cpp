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

#include "XmlMarshaller.h"
#include "OrthogonalRotation.h"
#include "Margins.h"
#include "Dpi.h"
#include "Utils.h"
#include <QPointF>
#include <QLineF>
#include <QRectF>
#include <QPolygonF>
#include <QSize>
#include <QString>

QDomElement
XmlMarshaller::size(QSize const& size, QString const& name)
{
	if (size.isNull()) {
		return QDomElement();
	}
	
	QDomElement el(m_doc.createElement(name));
	el.setAttribute("width", size.width());
	el.setAttribute("height", size.height());
	return el;
}


QDomElement
XmlMarshaller::sizeF(QSizeF const& size, QString const& name)
{
	if (size.isNull()) {
		return QDomElement();
	}
	
	QDomElement el(m_doc.createElement(name));
	el.setAttribute("width", Utils::doubleToString(size.width()));
	el.setAttribute("height", Utils::doubleToString(size.height()));
	return el;
}

QDomElement
XmlMarshaller::dpi(Dpi const& dpi, QString const& name)
{
	if (dpi.isNull()) {
		return QDomElement();
	}
	
	QDomElement el(m_doc.createElement(name));
	el.setAttribute("horizontal", dpi.horizontal());
	el.setAttribute("vertical", dpi.vertical());
	return el;
}

QDomElement
XmlMarshaller::rotation(OrthogonalRotation const& rotation, QString const& name)
{
	QDomElement el(m_doc.createElement(name));
	el.setAttribute("degrees", rotation.toDegrees());
	return el;
}

QDomElement
XmlMarshaller::pointF(QPointF const& p, QString const& name)
{
	QDomElement el(m_doc.createElement(name));
	el.setAttribute("x", Utils::doubleToString(p.x()));
	el.setAttribute("y", Utils::doubleToString(p.y()));
	return el;
}

QDomElement
XmlMarshaller::lineF(QLineF const& line, QString const& name)
{
	QDomElement el(m_doc.createElement(name));
	el.appendChild(pointF(line.p1(), "p1"));
	el.appendChild(pointF(line.p2(), "p2"));
	return el;
}

QDomElement
XmlMarshaller::rectF(QRectF const& rect, QString const& name)
{
	QDomElement el(m_doc.createElement(name));
	el.setAttribute("x", Utils::doubleToString(rect.x()));
	el.setAttribute("y", Utils::doubleToString(rect.y()));
	el.setAttribute("width", Utils::doubleToString(rect.width()));
	el.setAttribute("height", Utils::doubleToString(rect.height()));
	return el;
}

QDomElement
XmlMarshaller::polygonF(QPolygonF const& poly, QString const& name)
{
	QDomElement el(m_doc.createElement(name));
	
	QPolygonF::const_iterator it(poly.begin());
	QPolygonF::const_iterator const end(poly.end());
	for (; it != end; ++it) {
		el.appendChild(pointF(*it, "point"));
	}
	
	return el;
}

QDomElement
XmlMarshaller::margins(Margins const& margins, QString const& name)
{
	QDomElement el(m_doc.createElement(name));
	el.setAttribute("left", Utils::doubleToString(margins.left()));
	el.setAttribute("right", Utils::doubleToString(margins.right()));
	el.setAttribute("top", Utils::doubleToString(margins.top()));
	el.setAttribute("bottom", Utils::doubleToString(margins.bottom()));
	return el;
}
