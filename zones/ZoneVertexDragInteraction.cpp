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

#include "ZoneVertexDragInteraction.h"
#include "ZoneInteractionContext.h"
#include "EditableZoneSet.h"
#include "ImageViewBase.h"
#include <QTransform>
#include <QMouseEvent>
#include <QPainter>
#include <QPen>
#include <QLinearGradient>
#include <Qt>
#include <QLineF>
#include <boost/foreach.hpp>

ZoneVertexDragInteraction::ZoneVertexDragInteraction(
	ZoneInteractionContext& context, InteractionState& interaction,
	EditableSpline::Ptr const& spline, SplineVertex::Ptr const& vertex)
:	m_rContext(context),
	m_ptrSpline(spline),
	m_ptrVertex(vertex)
{
	QPointF const screen_mouse_pos(
		m_rContext.imageView().mapFromGlobal(QCursor::pos()) + QPointF(0.5, 0.5)
	);
	QTransform const to_screen(m_rContext.imageView().imageToWidget());
	m_dragOffset = to_screen.map(vertex->point()) - screen_mouse_pos;

	interaction.capture(m_interaction);
	checkProximity(interaction);
}

void
ZoneVertexDragInteraction::onPaint(QPainter& painter, InteractionState const& interaction)
{
	painter.setWorldMatrixEnabled(false);
	painter.setRenderHint(QPainter::Antialiasing);

	QTransform const to_screen(m_rContext.imageView().imageToWidget());

	BOOST_FOREACH(EditableZoneSet::Zone const& zone, m_rContext.zones()) {
		EditableSpline::Ptr const& spline = zone.spline();

		if (spline != m_ptrSpline) {
			// Draw the whole spline in solid color.
			m_visualizer.drawSpline(painter, to_screen, spline);
			continue;
		}

		// Draw the solid part of the spline.
		QPolygonF points;
		SplineVertex::Ptr vertex(m_ptrVertex->next(SplineVertex::LOOP));
		for (; vertex != m_ptrVertex; vertex = vertex->next(SplineVertex::LOOP)) {
			points.push_back(to_screen.map(vertex->point()));
		}

		m_visualizer.prepareForSpline(painter, spline);
		painter.drawPolyline(points);
	}

	QLinearGradient gradient; // From remote to selected point.
	gradient.setColorAt(0.0, m_visualizer.solidColor());
	gradient.setColorAt(1.0, m_visualizer.highlightDarkColor());

	QPen gradient_pen;
	gradient_pen.setCosmetic(true);
	gradient_pen.setWidthF(1.5);

	painter.setBrush(Qt::NoBrush);

	QPointF const pt(to_screen.map(m_ptrVertex->point()));
	QPointF const prev(to_screen.map(m_ptrVertex->prev(SplineVertex::LOOP)->point()));
	QPointF const next(to_screen.map(m_ptrVertex->next(SplineVertex::LOOP)->point()));

	gradient.setStart(prev);
	gradient.setFinalStop(pt);
	gradient_pen.setBrush(gradient);
	painter.setPen(gradient_pen);
	painter.drawLine(prev, pt);

	gradient.setStart(next);
	gradient_pen.setBrush(gradient);
	painter.setPen(gradient_pen);
	painter.drawLine(next, pt);

	m_visualizer.drawVertex(
		painter, to_screen.map(m_ptrVertex->point()),
		m_visualizer.highlightBrightColor()
	);
}

void
ZoneVertexDragInteraction::onMouseReleaseEvent(
	QMouseEvent* event, InteractionState& interaction)
{
	if (event->button() == Qt::LeftButton) {
		if (m_ptrVertex->point() == m_ptrVertex->next(SplineVertex::LOOP)->point() ||
				m_ptrVertex->point() == m_ptrVertex->prev(SplineVertex::LOOP)->point()) {
			if (m_ptrVertex->hasAtLeastSiblings(3)) {
				m_ptrVertex->remove();
			}
		}

		m_rContext.zones().commit();
		makePeerPreceeder(*m_rContext.createDefaultInteraction());
		delete this;
	}
}

void
ZoneVertexDragInteraction::onMouseMoveEvent(QMouseEvent* event, InteractionState& interaction)
{
	QTransform const from_screen(m_rContext.imageView().widgetToImage());
	m_ptrVertex->setPoint(from_screen.map(event->pos() + QPointF(0.5, 0.5) + m_dragOffset));

//begin of modified by monday2000
//Ortho_Corner_Move_Square_Picture_Zones
//added:

	Qt::KeyboardModifiers mask = event->modifiers();

	if (mask == Qt::ControlModifier) 
	{
		QTransform const to_screen(m_rContext.imageView().imageToWidget());

		QPointF current = event->pos() + QPointF(0.5, 0.5) + m_dragOffset;
		QPointF prev = to_screen.map(m_ptrVertex->prev(SplineVertex::LOOP)->point());
		QPointF next = to_screen.map(m_ptrVertex->next(SplineVertex::LOOP)->point());

		int dx = next.x() - prev.x();
		int dy = next.y() - prev.y();

		if ((dx > 0 && dy > 0) || (dx < 0 && dy < 0))
		{
			prev.setX(current.x());
			next.setY(current.y());
		}
		else
		{
			next.setX(current.x());	
			prev.setY(current.y());
		}		

		m_ptrVertex->prev(SplineVertex::LOOP)->setPoint(from_screen.map(prev));
		m_ptrVertex->next(SplineVertex::LOOP)->setPoint(from_screen.map(next));
	}
//end of modified by monday2000

	checkProximity(interaction);
	m_rContext.imageView().update();
}

void
ZoneVertexDragInteraction::checkProximity(InteractionState const& interaction)
{
	bool can_merge = false;

	if (m_ptrVertex->hasAtLeastSiblings(3)) {
		QTransform const to_screen(m_rContext.imageView().imageToWidget());
		QPointF const origin(to_screen.map(m_ptrVertex->point()));

		QPointF const prev(m_ptrVertex->prev(SplineVertex::LOOP)->point());
		Proximity const prox_prev(origin, to_screen.map(prev));

		QPointF const next(m_ptrVertex->next(SplineVertex::LOOP)->point());
		Proximity const prox_next(origin, to_screen.map(next));

		if (prox_prev <= interaction.proximityThreshold() && prox_prev < prox_next) {
			m_ptrVertex->setPoint(prev);
			can_merge = true;
		} else if (prox_next <= interaction.proximityThreshold()) {
			m_ptrVertex->setPoint(next);
			can_merge = true;
		}
	}

	if (can_merge) {
		m_interaction.setInteractionStatusTip(tr("Merge these two vertices."));
	} else {
		m_interaction.setInteractionStatusTip(tr("Move the vertex to one of its neighbors to merge them."));
	}
}
