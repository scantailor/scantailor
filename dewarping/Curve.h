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

#ifndef DEWARPING_CURVE_H_
#define DEWARPING_CURVE_H_

#include <QPointF>
#include "XSpline.h"
#include <vector>

class QDomDocument;
class QDomElement;
class QString;

namespace dewarping
{

class Curve
{
public:
	Curve();

	Curve(std::vector<QPointF> const& polyline);

	Curve(XSpline const& xspline);

	Curve(QDomElement const& el);

	QDomElement toXml(QDomDocument& doc, QString const& name) const;

	bool isValid() const;

	bool matches(Curve const& other) const;

	XSpline const& xspline() const { return m_xspline; }

	std::vector<QPointF> const& polyline() const { return m_polyline; }

	static bool splineHasLoops(XSpline const& spline);
private:
	struct CloseEnough;

	static std::vector<QPointF> deserializePolyline(QDomElement const& el);

	static QDomElement serializePolyline(
		std::vector<QPointF> const& polyline, QDomDocument& doc, QString const& name);

	static XSpline deserializeXSpline(QDomElement const& el);

	static QDomElement serializeXSpline(
		XSpline const& xspline, QDomDocument& doc, QString const& name);

	static bool approxPolylineMatch(
		std::vector<QPointF> const& polyline1,
		std::vector<QPointF> const& polyline2);

	XSpline m_xspline;
	std::vector<QPointF> m_polyline;
};

} // namespace dewarping

#endif
