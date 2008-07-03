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

#include "PolygonUtils.h"
#include <boost/foreach.hpp>
#include <QPolygonF>
#include <QPointF>
#include <math.h>

namespace imageproc
{

QPolygonF
PolygonUtils::round(QPolygonF const& poly)
{
	QPolygonF rounded;
	rounded.reserve(poly.size());
	
	BOOST_FOREACH(QPointF const& p, poly) {
		rounded.push_back(roundPoint(p));
	}
	
	return rounded;
}

QPointF
PolygonUtils::roundPoint(QPointF const& p)
{
	return QPointF(roundValue(p.x()), roundValue(p.y()));
}

double
PolygonUtils::roundValue(double const val)
{
	double const multiplier = 1 << 16;
	double const r_multiplier = 1.0 / multiplier;
	return floor(val * multiplier + 0.5) * r_multiplier;
}


} // namespace imageproc
