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

#include "Curve.h"
#include "XmlMarshaller.h"
#include "XmlUnmarshaller.h"
#include "VecNT.h"
#ifndef Q_MOC_RUN
#include <boost/foreach.hpp>
#endif
#include <QByteArray>
#include <QDataStream>
#include <QDomDocument>
#include <QDomElement>
#include <QString>
#include <algorithm>

namespace dewarping
{

struct Curve::CloseEnough
{
	bool operator()(QPointF const& p1, QPointF const& p2) {
		QPointF const d(p1 - p2);
		return d.x() * d.x() + d.y() * d.y() <= 0.01 * 0.01;
	}
};

Curve::Curve()
{
}

Curve::Curve(std::vector<QPointF> const& polyline)
:	m_polyline(polyline)
{
}

Curve::Curve(XSpline const& xspline)
:	m_xspline(xspline),
	m_polyline(xspline.toPolyline())
{
}

Curve::Curve(QDomElement const& el)
:	m_xspline(deserializeXSpline(el.namedItem("xspline").toElement())),
	m_polyline(deserializePolyline(el.namedItem("polyline").toElement()))
{
}

QDomElement
Curve::toXml(QDomDocument& doc, QString const& name) const
{
	if (!isValid()) {
		return QDomElement();
	}

	QDomElement el(doc.createElement(name));
	el.appendChild(serializeXSpline(m_xspline, doc, "xspline"));
	el.appendChild(serializePolyline(m_polyline, doc, "polyline"));
	return el;
}

bool
Curve::isValid() const
{
	return m_polyline.size() > 1 && m_polyline.front() != m_polyline.back();
}

bool
Curve::matches(Curve const& other) const
{
	if (!approxPolylineMatch(m_polyline, other.m_polyline)) {
		return false;
	}

	return true;
}

std::vector<QPointF>
Curve::deserializePolyline(QDomElement const& el)
{
	QByteArray ba(QByteArray::fromBase64(el.text().trimmed().toLatin1()));
	QDataStream strm(&ba, QIODevice::ReadOnly);
	strm.setVersion(QDataStream::Qt_4_4);
	strm.setByteOrder(QDataStream::LittleEndian);

	unsigned const num_points = ba.size() / 8;
	std::vector<QPointF> points;
	points.reserve(num_points);

	for (unsigned i = 0; i < num_points; ++i) {
		float x = 0, y = 0;
		strm >> x >> y;
		points.push_back(QPointF(x, y));
	}

	return points;
}

QDomElement
Curve::serializePolyline(
	std::vector<QPointF> const& polyline, QDomDocument& doc, QString const& name)
{
	if (polyline.empty()) {
		return QDomElement();
	}

	QByteArray ba;
	ba.reserve(8 * polyline.size());
	QDataStream strm(&ba, QIODevice::WriteOnly);
	strm.setVersion(QDataStream::Qt_4_4);
	strm.setByteOrder(QDataStream::LittleEndian);

	BOOST_FOREACH(QPointF const& pt, polyline) {
		strm << (float)pt.x() << (float)pt.y();
	}

	QDomElement el(doc.createElement(name));
	el.appendChild(doc.createTextNode(QString::fromLatin1(ba.toBase64())));

	return el;
}

bool
Curve::approxPolylineMatch(
	std::vector<QPointF> const& polyline1, std::vector<QPointF> const& polyline2)
{
	if (polyline1.size() != polyline2.size()) {
		return false;
	}

	if (!std::equal(polyline1.begin(), polyline1.end(), polyline2.begin(), CloseEnough())) {
		return false;
	}

	return true;
}

QDomElement
Curve::serializeXSpline(
	XSpline const& xspline, QDomDocument& doc, QString const& name)
{
	if (xspline.numControlPoints() == 0) {
		return QDomElement();
	}

	QDomElement el(doc.createElement(name));
	XmlMarshaller marshaller(doc);

	int const num_control_points = xspline.numControlPoints();
	for (int i = 0; i < num_control_points; ++i) {
		QPointF const pt(xspline.controlPointPosition(i));
		el.appendChild(marshaller.pointF(pt, "point"));
	}
	
	return el;
}


XSpline
Curve::deserializeXSpline(QDomElement const& el)
{
	XSpline xspline;

	QString const point_tag_name("point");
	QDomNode node(el.firstChild());
	for (; !node.isNull(); node = node.nextSibling()) {
		if (!node.isElement()) {
			continue;
		}
		if (node.nodeName() != point_tag_name) {
			continue;
		}
		xspline.appendControlPoint(XmlUnmarshaller::pointF(node.toElement()), 1);
	}

	if (xspline.numControlPoints() > 0) {
		xspline.setControlPointTension(0, 0);
		xspline.setControlPointTension(xspline.numControlPoints() - 1, 0);
	}

	return xspline;
}

bool
Curve::splineHasLoops(XSpline const& spline)
{
	int const num_control_points = spline.numControlPoints();
	Vec2d const main_direction(spline.pointAt(1) - spline.pointAt(0));

	for (int i = 1; i < num_control_points; ++i) {
		QPointF const cp1(spline.controlPointPosition(i - 1));
		QPointF const cp2(spline.controlPointPosition(i));
		if (Vec2d(cp2 - cp1).dot(main_direction) < 0) {
			return true;
		}
#if 0
		double const t1 = spline.controlPointIndexToT(i - 1);
		double const t2 = spline.controlPointIndexToT(i);
		if (Vec2d(spline.pointAt(t2) - spline.pointAt(t1)).dot(main_direction)) < 0) {
			return true;
		}
#endif
	}

	return false;
}

} // namespace dewarping

