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

#include "BoxResizeHandler.h"
#include <QMouseEvent>
#include <QCursor>
#include <Qt>
#include <algorithm>
#include <assert.h>

BoxResizeHandler::BoxResizeHandler(int base_proximity_priority)
:	DraggablePoint(base_proximity_priority + 1),
	DraggableLineSegment(base_proximity_priority),
	m_minBoxSize(1, 1)
{
	for (int i = 0; i < 4; ++i) {
		m_cornerHandlers[i].setObject(static_cast<DraggablePoint*>(this));
		m_edgeHandlers[i].setObject(static_cast<DraggableLineSegment*>(this));

		if ((i & 1) == 0) {
			m_cornerHandlers[i].setProximityCursor(Qt::SizeFDiagCursor);
			m_cornerHandlers[i].setInteractionCursor(Qt::SizeFDiagCursor);
			m_edgeHandlers[i].setProximityCursor(Qt::SizeVerCursor);
			m_edgeHandlers[i].setInteractionCursor(Qt::SizeVerCursor);
		} else {
			m_cornerHandlers[i].setProximityCursor(Qt::SizeBDiagCursor);
			m_cornerHandlers[i].setInteractionCursor(Qt::SizeBDiagCursor);
			m_edgeHandlers[i].setProximityCursor(Qt::SizeHorCursor);
			m_edgeHandlers[i].setInteractionCursor(Qt::SizeHorCursor);
		}

		makeLastPreceeder(m_cornerHandlers[i]);
		makeLastPreceeder(m_edgeHandlers[i]);
	}
}

void
BoxResizeHandler::setProximityStatusTip(QString const& tip)
{
	for (int i = 0; i < 4; ++i) {
		m_cornerHandlers[i].setProximityStatusTip(tip);
		m_edgeHandlers[i].setProximityStatusTip(tip);
	}
}

void
BoxResizeHandler::setInteractionStatusTip(QString const& tip)
{
	for (int i = 0; i < 4; ++i) {
		m_cornerHandlers[i].setInteractionStatusTip(tip);
		m_edgeHandlers[i].setInteractionStatusTip(tip);
	}
}

bool
BoxResizeHandler::interactionInProgress(
	InteractionState const& interaction) const
{
	for (int i = 0; i < 4; ++i) {
		if (m_cornerHandlers[i].interactionInProgress(interaction)) {
			return true;
		}
		if (m_edgeHandlers[i].interactionInProgress(interaction)) {
			return true;
		}
	}
	return false;
}

bool
BoxResizeHandler::proximityLeader(InteractionState const& interaction) const
{
	for (int i = 0; i < 4; ++i) {
		if (m_cornerHandlers[i].proximityLeader(interaction)) {
			return true;
		}
		if (m_edgeHandlers[i].proximityLeader(interaction)) {
			return true;
		}
	}
	return false;
}

QPointF
BoxResizeHandler::pointPosition(
	ObjectDragHandler const* handler, InteractionState const& interaction) const
{
	QRectF const box(boxPosition(this, interaction));

	if (&m_cornerHandlers[0] == handler) {
		return box.topLeft();
	} else if (&m_cornerHandlers[1] == handler) {
		return box.topRight();
	} else if (&m_cornerHandlers[2] == handler) {
		return box.bottomRight();
	} else {
		assert(&m_cornerHandlers[3] == handler);
		return box.bottomLeft();
	}
}

void
BoxResizeHandler::pointMoveRequest(
	ObjectDragHandler const* handler, QPointF const& widget_pos,
	InteractionState const& interaction)
{
	QRectF box(boxPosition(this, interaction));

	double const minw = m_minBoxSize.width();
	double const minh = m_minBoxSize.height();

	if (&m_cornerHandlers[0] == handler) {
		box.setTop(std::min(widget_pos.y(), box.bottom() - minh));
		box.setLeft(std::min(widget_pos.x(), box.right() - minw));
	} else if (&m_cornerHandlers[1] == handler) {
		box.setTop(std::min(widget_pos.y(), box.bottom() - minh));
		box.setRight(std::max(widget_pos.x(), box.left() + minw));
	} else if (&m_cornerHandlers[2] == handler) {
		box.setBottom(std::max(widget_pos.y(), box.top() + minh));
		box.setRight(std::max(widget_pos.x(), box.left() + minw));
	} else {
		assert(&m_cornerHandlers[3] == handler);
		box.setBottom(std::max(widget_pos.y(), box.top() + minh));
		box.setLeft(std::min(widget_pos.x(), box.right() - minw));
	}

	boxResizeRequest(this, box, interaction);
}

QLineF
BoxResizeHandler::lineSegment(
	ObjectDragHandler const* handler, InteractionState const& interaction) const
{
	QRectF const box(boxPosition(this, interaction));

	if (&m_edgeHandlers[0] == handler) {
		return QLineF(box.topLeft(), box.topRight());
	} else if (&m_edgeHandlers[1] == handler) {
		return QLineF(box.topRight(), box.bottomRight());
	} else if (&m_edgeHandlers[2] == handler) {
		return QLineF(box.bottomRight(), box.bottomLeft());
	} else {
		assert(&m_edgeHandlers[3] == handler);
		return QLineF(box.bottomLeft(), box.topLeft());
	}
}

QPointF
BoxResizeHandler::lineSegmentPosition(
	ObjectDragHandler const* handler, InteractionState const& interaction) const
{
	return lineSegment(handler, interaction).p1();
}

void
BoxResizeHandler::lineSegmentMoveRequest(
	ObjectDragHandler const* handler, QPointF const& widget_pos,
	InteractionState const& interaction)
{
	QRectF box(boxPosition(this, interaction));

	double const minw = m_minBoxSize.width();
	double const minh = m_minBoxSize.height();

	if (&m_edgeHandlers[0] == handler) {
		box.setTop(std::min(widget_pos.y(), box.bottom() - minh));
	} else if (&m_edgeHandlers[1] == handler) {
		box.setRight(std::max(widget_pos.x(), box.left() + minw));
	} else if (&m_edgeHandlers[2] == handler) {
		box.setBottom(std::max(widget_pos.y(), box.top() + minh));
	} else {
		assert(&m_edgeHandlers[3] == handler);
		box.setLeft(std::min(widget_pos.x(), box.right() - minw));
	}

	boxResizeRequest(this, box, interaction);
}

void
BoxResizeHandler::dragFinished(ObjectDragHandler const*)
{
	boxResizeFinished(this);
}
