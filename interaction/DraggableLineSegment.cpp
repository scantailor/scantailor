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

#include "DraggableLineSegment.h"
#include "Proximity.h"
#include "ImageViewBase.h"
#include <QPainter>
#include <QTransform>
#include <QRectF>
#include <algorithm>

DraggableLineSegment::DraggableLineSegment(int proximity_priority)
:	m_proximityPriority(proximity_priority)
{
}

int
DraggableLineSegment::proximityPriority(ObjectDragHandler const*) const
{
	return m_proximityPriority;
}

Proximity
DraggableLineSegment::proximity(
	ObjectDragHandler const* handler, QPointF const& widget_mouse_pos,
	InteractionState const& interaction)
{
	QLineF const line(lineSegment(handler, interaction));
	return Proximity::pointAndLineSegment(widget_mouse_pos, line);
}

QPointF
DraggableLineSegment::position(
	ObjectDragHandler const* handler, InteractionState const& interaction) const
{
	return lineSegmentPosition(handler, interaction);
}

void
DraggableLineSegment::moveRequest(
	ObjectDragHandler const* handler, QPointF const& widget_pos,
	InteractionState const& interaction)
{
	lineSegmentMoveRequest(handler, widget_pos, interaction);
}
