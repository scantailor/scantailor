/*
	Scan Tailor - Interactive post-processing tool for scanned pages.
	Copyright (C) 2007-2009  Joseph Artsimovich <joseph_a@mail.ru>

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

#include "BasicSplineVisualizer.h"
#include "EditableZoneSet.h"
#include <QPainter>
#include <QBrush>
#include <Qt>
#ifndef Q_MOC_RUN
#include <boost/foreach.hpp>
#endif

BasicSplineVisualizer::BasicSplineVisualizer()
:	m_solidColor(0xcc1420),
	m_highlightBrightColor(0xfffe00),
	m_highlightDarkColor(0xffa90e),
	m_pen(m_solidColor)
{
	m_pen.setCosmetic(true);
	m_pen.setWidthF(1.5);
}

void
BasicSplineVisualizer::drawSplines(
	QPainter& painter, QTransform const& to_screen,
	EditableZoneSet const& zones)
{
	BOOST_FOREACH(EditableZoneSet::Zone const& zone, zones) {
		drawSpline(painter, to_screen, zone.spline());
	}
}

void
BasicSplineVisualizer::drawSpline(
	QPainter& painter, QTransform const& to_screen, EditableSpline::Ptr const& spline)
{
	prepareForSpline(painter, spline);
	painter.drawPolygon(to_screen.map(spline->toPolygon()), Qt::WindingFill);
}

void
BasicSplineVisualizer::drawVertex(QPainter& painter, QPointF const& pt, QColor const& color)
{
	painter.setPen(Qt::NoPen);
	painter.setBrush(color);

	QRectF rect(0, 0, 4, 4);
	rect.moveCenter(pt);
	painter.drawEllipse(rect);
}

void
BasicSplineVisualizer::prepareForSpline(
	QPainter& painter, EditableSpline::Ptr const&)
{
	painter.setPen(m_pen);
	painter.setBrush(Qt::NoBrush);
}
