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

#include "ObjectDragHandler.h"
#include <QMouseEvent>
#include <QCursor>
#include <Qt>

ObjectDragHandler::ObjectDragHandler(DraggableObject* obj)
:	m_pObj(obj)
{
	setProximityCursor(Qt::OpenHandCursor);
	setInteractionCursor(Qt::ClosedHandCursor);
}

void
ObjectDragHandler::setProximityCursor(QCursor const& cursor)
{
	m_interaction.setProximityCursor(cursor);
}

void
ObjectDragHandler::setInteractionCursor(QCursor const& cursor)
{
	m_interaction.setInteractionCursor(cursor);
}

void
ObjectDragHandler::setProximityStatusTip(QString const& tip)
{
	m_interaction.setProximityStatusTip(tip);
}

void
ObjectDragHandler::setInteractionStatusTip(QString const& tip)
{
	m_interaction.setInteractionStatusTip(tip);
}

bool
ObjectDragHandler::interactionInProgress(
	InteractionState const& interaction) const
{
	return interaction.capturedBy(m_interaction);
}

bool
ObjectDragHandler::proximityLeader(InteractionState const& interaction) const
{
	return interaction.proximityLeader(m_interaction);
}

void
ObjectDragHandler::forceEnterDragState(
	InteractionState& interaction, QPoint widget_mouse_pos)
{
	interaction.capture(m_interaction);
	m_pObj->dragInitiated(QPointF(0.5, 0.5) + widget_mouse_pos);
}

void
ObjectDragHandler::onPaint(
	QPainter& painter, InteractionState const& interaction)
{
	m_pObj->paint(painter, interaction);
}

void
ObjectDragHandler::onProximityUpdate(
	QPointF const& screen_mouse_pos, InteractionState& interaction)
{
	interaction.updateProximity(
		m_interaction, m_pObj->proximity(screen_mouse_pos),
		m_pObj->proximityPriority(), m_pObj->proximityThreshold(interaction)
	);
}

void
ObjectDragHandler::onMousePressEvent(
	QMouseEvent* event, InteractionState& interaction)
{
	if (interaction.captured() || event->button() != Qt::LeftButton) {
		return;
	}

	if (interaction.proximityLeader(m_interaction)) {
		interaction.capture(m_interaction);
		m_pObj->dragInitiated(QPointF(0.5, 0.5) + event->pos());
	}
}

void
ObjectDragHandler::onMouseReleaseEvent(
	QMouseEvent* event, InteractionState& interaction)
{
	if (event->button() == Qt::LeftButton && interaction.capturedBy(m_interaction)) {
		m_interaction.release();
		m_pObj->dragFinished(QPointF(0.5, 0.5) + event->pos());
	}
}

void
ObjectDragHandler::onMouseMoveEvent(
	QMouseEvent* event, InteractionState& interaction)
{
	if (interaction.capturedBy(m_interaction)) {
		m_pObj->dragContinuation(QPointF(0.5, 0.5) + event->pos());
	}
}
