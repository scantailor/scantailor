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

#include "DraggablePoint.h"
#include "Proximity.h"
#include "ImageViewBase.h"

DraggablePoint::DraggablePoint(int proximity_priority)
:	m_hitAreaRadius(),
	m_proximityPriority(proximity_priority)
{
}

Proximity
DraggablePoint::proximityThreshold(
	ObjectDragHandler const*, InteractionState const& state) const
{
	if (m_hitAreaRadius == 0.0) {
		return state.proximityThreshold();
	} else {
		return Proximity::fromDist(m_hitAreaRadius);
	}
}

int
DraggablePoint::proximityPriority(ObjectDragHandler const*) const
{
	return m_proximityPriority;
}

Proximity
DraggablePoint::proximity(
	ObjectDragHandler const* handler, QPointF const& widget_mouse_pos,
	InteractionState const& interaction)
{
	return Proximity(pointPosition(handler, interaction), widget_mouse_pos);
}

QPointF
DraggablePoint::position(
	ObjectDragHandler const* handler, InteractionState const& interaction) const
{
	return pointPosition(handler, interaction);
}

void
DraggablePoint::moveRequest(
	ObjectDragHandler const* handler, QPointF const& widget_pos,
	InteractionState const& interaction)
{
	pointMoveRequest(handler, widget_pos, interaction);
}
