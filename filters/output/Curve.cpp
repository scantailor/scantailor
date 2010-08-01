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
#include <boost/foreach.hpp>
#include <QByteArray>
#include <QDataStream>
#include <QDomDocument>
#include <QDomElement>
#include <QString>
#include <algorithm>

namespace output
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

Curve::Curve(imageproc::CubicBSpline const& bspline)
:	m_bspline(bspline),
	m_polyline(bspline.toPolyline())
{
}

Curve::Curve(QDomElement const& el)
:	m_bspline(XmlUnmarshaller::bspline(el.namedItem("bspline").toElement())),
	m_polyline(deserializePolyline(el.namedItem("polyline").toElement()))
{
}

QDomElement
Curve::toXml(QDomDocument& doc, QString const& name) const
{
	if (!isValid()) {
		return QDomElement();
	}

	XmlMarshaller marshaller(doc);

	QDomElement el(doc.createElement(name));
	el.appendChild(marshaller.bspline(m_bspline, "bspline"));
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

} // namespace output

