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

#include "ZoneCreationInteraction.h"
#include "ZoneInteractionContext.h"
#include "EditableZoneSet.h"
#include "ImageViewBase.h"
#include <QCursor>
#include <QTransform>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QPainter>
#include <QPen>
#include <QLinearGradient>
#include <Qt>
#include <QLineF>
#include <QDebug>
#ifndef Q_MOC_RUN
#include <boost/lambda/lambda.hpp>
#endif

ZoneCreationInteraction::ZoneCreationInteraction(
	ZoneInteractionContext& context, InteractionState& interaction)
:	m_rContext(context),
	m_dragHandler(context.imageView(), boost::lambda::constant(true)),
	m_dragWatcher(m_dragHandler),
	m_zoomHandler(context.imageView(), boost::lambda::constant(true)),
	m_ptrSpline(new EditableSpline)
{
	QPointF const screen_mouse_pos(
		m_rContext.imageView().mapFromGlobal(QCursor::pos()) + QPointF(0.5, 0.5)
	);
	QTransform const from_screen(m_rContext.imageView().widgetToImage());
	m_nextVertexImagePos = from_screen.map(screen_mouse_pos);

	makeLastFollower(m_dragHandler);
	m_dragHandler.makeFirstFollower(m_dragWatcher);

	makeLastFollower(m_zoomHandler);

	interaction.capture(m_interaction);
	m_ptrSpline->appendVertex(m_nextVertexImagePos);

	updateStatusTip();
}

void
ZoneCreationInteraction::onPaint(QPainter& painter, InteractionState const& interaction)
{
	painter.setWorldMatrixEnabled(false);
	painter.setRenderHint(QPainter::Antialiasing);

	QTransform const to_screen(m_rContext.imageView().imageToWidget());
	QTransform const from_screen(m_rContext.imageView().widgetToImage());

	m_visualizer.drawSplines(painter, to_screen, m_rContext.zones());

	QPen solid_line_pen(m_visualizer.solidColor());
	solid_line_pen.setCosmetic(true);
	solid_line_pen.setWidthF(1.5);

	QLinearGradient gradient; // From inactive to active point.
	gradient.setColorAt(0.0, m_visualizer.solidColor());
	gradient.setColorAt(1.0, m_visualizer.highlightDarkColor());

	QPen gradient_pen;
	gradient_pen.setCosmetic(true);
	gradient_pen.setWidthF(1.5);

	painter.setPen(solid_line_pen);
	painter.setBrush(Qt::NoBrush);

	for (EditableSpline::SegmentIterator it(*m_ptrSpline); it.hasNext(); ) {
		SplineSegment const segment(it.next());
		QLineF const line(to_screen.map(segment.toLine()));

		if (segment.prev == m_ptrSpline->firstVertex() &&
				segment.prev->point() == m_nextVertexImagePos) {
			gradient.setStart(line.p2());
			gradient.setFinalStop(line.p1());
			gradient_pen.setBrush(gradient);
			painter.setPen(gradient_pen);
			painter.drawLine(line);
			painter.setPen(solid_line_pen);
		} else {
			painter.drawLine(line);
		}
	}

	QLineF const line(
		to_screen.map(QLineF(m_ptrSpline->lastVertex()->point(), m_nextVertexImagePos))
	);
	gradient.setStart(line.p1());
	gradient.setFinalStop(line.p2());
	gradient_pen.setBrush(gradient);
	painter.setPen(gradient_pen);
	painter.drawLine(line);

	m_visualizer.drawVertex(
		painter, to_screen.map(m_nextVertexImagePos), m_visualizer.highlightBrightColor()
	);
}

void
ZoneCreationInteraction::onKeyPressEvent(QKeyEvent* event, InteractionState& interaction)
{
	if (event->key() == Qt::Key_Escape) {
		makePeerPreceeder(*m_rContext.createDefaultInteraction());
		m_rContext.imageView().update();
		delete this;
		event->accept();
	}
}

void
ZoneCreationInteraction::onMouseReleaseEvent(QMouseEvent* event, InteractionState& interaction)
{
	if (event->button() != Qt::LeftButton) {
		return;
	}

	if (m_dragWatcher.haveSignificantDrag()) {
		return;
	}

	QTransform const to_screen(m_rContext.imageView().imageToWidget());
	QTransform const from_screen(m_rContext.imageView().widgetToImage());
	QPointF const screen_mouse_pos(event->pos() + QPointF(0.5, 0.5));
	QPointF const image_mouse_pos(from_screen.map(screen_mouse_pos));

	if (m_ptrSpline->hasAtLeastSegments(2) &&
			m_nextVertexImagePos == m_ptrSpline->firstVertex()->point()) {
		// Finishing the spline.  Bridging the first and the last points
		// will create another segment.
		m_ptrSpline->setBridged(true);
		m_rContext.zones().addZone(m_ptrSpline);
		m_rContext.zones().commit();

		makePeerPreceeder(*m_rContext.createDefaultInteraction());
		m_rContext.imageView().update();
		delete this;
	} else if (m_nextVertexImagePos == m_ptrSpline->lastVertex()->point()) {
		// Removing the last vertex.
		m_ptrSpline->lastVertex()->remove();
		if (!m_ptrSpline->firstVertex()) {
			// If it was the only vertex, cancelling spline creation.
			makePeerPreceeder(*m_rContext.createDefaultInteraction());
			m_rContext.imageView().update();
			delete this;
		}
	} else {
		// Adding a new vertex, provided we are not to close to the previous one.
		Proximity const prox(screen_mouse_pos, m_ptrSpline->lastVertex()->point());
		if (prox > interaction.proximityThreshold()) {
			m_ptrSpline->appendVertex(image_mouse_pos);
			updateStatusTip();
		}
	}

	event->accept();
}

void
ZoneCreationInteraction::onMouseMoveEvent(QMouseEvent* event, InteractionState& interaction)
{
	QPointF const screen_mouse_pos(event->pos() + QPointF(0.5, 0.5));
	QTransform const to_screen(m_rContext.imageView().imageToWidget());
	QTransform const from_screen(m_rContext.imageView().widgetToImage());

	m_nextVertexImagePos = from_screen.map(screen_mouse_pos);

	QPointF const last(to_screen.map(m_ptrSpline->lastVertex()->point()));
	if (Proximity(last, screen_mouse_pos) <= interaction.proximityThreshold()) {
		m_nextVertexImagePos = m_ptrSpline->lastVertex()->point();
	} else if (m_ptrSpline->hasAtLeastSegments(2)) {
		QPointF const first(to_screen.map(m_ptrSpline->firstVertex()->point()));
		if (Proximity(first, screen_mouse_pos) <= interaction.proximityThreshold()) {
			m_nextVertexImagePos = m_ptrSpline->firstVertex()->point();
			updateStatusTip();
		}
	}

	m_rContext.imageView().update();
}

void
ZoneCreationInteraction::updateStatusTip()
{
	QString tip;

	if (m_ptrSpline->hasAtLeastSegments(2)) {
		if (m_nextVertexImagePos == m_ptrSpline->firstVertex()->point()) {
			tip = tr("Click to finish this zone.  ESC to cancel.");
		} else {
			tip = tr("Connect first and last points to finish this zone.  ESC to cancel.");
		}
	} else {
		tip = tr("Zones need to have at least 3 points.  ESC to cancel.");
	}

	m_interaction.setInteractionStatusTip(tip);
}
