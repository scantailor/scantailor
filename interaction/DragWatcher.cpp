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

#include "DragWatcher.h"
#include "DragHandler.h"
#include <QMouseEvent>

DragWatcher::DragWatcher(DragHandler& drag_handler)
:	m_rDragHandler(drag_handler),
	m_dragMaxSqDist(0),
	m_dragInProgress(false)
{
}

bool
DragWatcher::haveSignificantDrag() const
{
	if (!m_dragInProgress) {
		return false;
	}

	if (QDateTime::currentDateTime() < m_dragStartTime.addMSecs(500)) {
		return false;
	}

	if (m_dragMaxSqDist < 6*6) {
		return false;
	}

	return true;
}

void
DragWatcher::onMousePressEvent(QMouseEvent* event, InteractionState&)
{
	updateState(event->pos());
}

void
DragWatcher::onMouseMoveEvent(QMouseEvent* event, InteractionState&)
{
	updateState(event->pos());
}

void
DragWatcher::updateState(QPoint const mouse_pos)
{
	if (m_rDragHandler.isActive()) {
		if (!m_dragInProgress) {
			m_dragStartTime = QDateTime::currentDateTime();
			m_dragStartPos = mouse_pos;
			m_dragMaxSqDist = 0;
		} else {
			QPoint const delta(mouse_pos - m_dragStartPos);
			int const sqdist = delta.x() * delta.x() + delta.y() * delta.y();
			if (sqdist > m_dragMaxSqDist) {
				m_dragMaxSqDist = sqdist;
			}
		}
		m_dragInProgress = true;
	} else {
		m_dragInProgress = false;
	}
}
