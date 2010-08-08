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

#include "XmlUnmarshaller.h"
#include "Dpi.h"
#include "OrthogonalRotation.h"
#include "Margins.h"
#include <QString>
#include <QSize>
#include <QSizeF>
#include <QPointF>
#include <QLineF>
#include <QRect>
#include <QRectF>
#include <QPolygonF>
#include <QDomElement>

QString
XmlUnmarshaller::string(QDomElement const& el)
{
	return el.text(); // FIXME: this needs unescaping, but Qt doesn't provide such functionality
}

QSize
XmlUnmarshaller::size(QDomElement const& el)
{
	int const width = el.attribute("width").toInt();
	int const height = el.attribute("height").toInt();
	return QSize(width, height);
}

QSizeF
XmlUnmarshaller::sizeF(QDomElement const& el)
{
	double const width = el.attribute("width").toDouble();
	double const height = el.attribute("height").toDouble();
	return QSizeF(width, height);
}

Dpi
XmlUnmarshaller::dpi(QDomElement const& el)
{
	int const hor = el.attribute("horizontal").toInt();
	int const ver = el.attribute("vertical").toInt();
	return Dpi(hor, ver);
}

OrthogonalRotation
XmlUnmarshaller::rotation(QDomElement const& el)
{
	int const degrees = el.attribute("degrees").toInt();
	OrthogonalRotation rotation;
	for (int i = 0; i < 4; ++i) {
		if (rotation.toDegrees() == degrees) {
			break;
		}
		rotation.nextClockwiseDirection();
	}
	return rotation;
}

Margins
XmlUnmarshaller::margins(QDomElement const& el)
{
	Margins margins;
	margins.setLeft(el.attribute("left").toDouble());
	margins.setRight(el.attribute("right").toDouble());
	margins.setTop(el.attribute("top").toDouble());
	margins.setBottom(el.attribute("bottom").toDouble());
	return margins;
}

QPointF
XmlUnmarshaller::pointF(QDomElement const& el)
{
	double const x = el.attribute("x").toDouble();
	double const y = el.attribute("y").toDouble();
	return QPointF(x, y);
}

QLineF
XmlUnmarshaller::lineF(QDomElement const& el)
{
	QPointF const p1(pointF(el.namedItem("p1").toElement()));
	QPointF const p2(pointF(el.namedItem("p2").toElement()));
	return QLineF(p1, p2);
}

QRect
XmlUnmarshaller::rect(QDomElement const& el)
{
	int const x = el.attribute("x").toInt();
	int const y = el.attribute("y").toInt();
	int const width = el.attribute("width").toInt();
	int const height = el.attribute("height").toInt();
	return QRect(x, y, width, height);
}

QRectF
XmlUnmarshaller::rectF(QDomElement const& el)
{
	double const x = el.attribute("x").toDouble();
	double const y = el.attribute("y").toDouble();
	double const width = el.attribute("width").toDouble();
	double const height = el.attribute("height").toDouble();
	return QRectF(x, y, width, height);
}

QPolygonF
XmlUnmarshaller::polygonF(QDomElement const& el)
{
	QPolygonF poly;
	
	QString const point_tag_name("point");
	QDomNode node(el.firstChild());
	for (; !node.isNull(); node = node.nextSibling()) {
		if (!node.isElement()) {
			continue;
		}
		if (node.nodeName() != point_tag_name) {
			continue;
		}
		poly.push_back(pointF(node.toElement()));
	}
	
	return poly;
}
