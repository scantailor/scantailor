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

#include "DraggablePoint.h"
#include "Proximity.h"
#include "ImageViewBase.h"

DraggablePoint::DraggablePoint()
:	m_hitAreaRadius(),
	m_proximityPriority(1)
{
}

Proximity
DraggablePoint::proximityThreshold(InteractionState const& state) const
{
	if (m_hitAreaRadius == 0.0) {
		return state.proximityThreshold();
	} else {
		return Proximity::fromDist(m_hitAreaRadius);
	}
}

int
DraggablePoint::proximityPriority() const
{
	return m_proximityPriority;
}

Proximity
DraggablePoint::proximity(QPointF const& mouse_pos)
{
	return Proximity(pointPosition(), mouse_pos);
}

void
DraggablePoint::dragInitiated(QPointF const& mouse_pos)
{
	m_pointRelativeToMouse = pointPosition() - mouse_pos;
}

void
DraggablePoint::dragContinuation(QPointF const& mouse_pos)
{
	pointMoveRequest(mouse_pos + m_pointRelativeToMouse);
}
