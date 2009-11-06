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

#ifndef DRAG_WATCHER_H_
#define DRAG_WATCHER_H_

#include "InteractionHandler.h"
#include <QPoint>
#include <QDateTime>

class DragHandler;
class InteractionState;
class QMouseEvent;

/**
 * Objects of this class are intended to be used as a descendants followers
 * of DragHandler objects.  The idea is to collect some statistics about
 * an ongoing drag and decide if the drag was significant or not, in which
 * case we could perform some other operation on mouse release.
 */
class DragWatcher : public InteractionHandler
{
public:
	DragWatcher(DragHandler& drag_handler);

	bool haveSignificantDrag() const;
protected:
	virtual void onMousePressEvent(QMouseEvent* event, InteractionState& interaction);

	virtual void onMouseMoveEvent(QMouseEvent* event, InteractionState& interaction);
private:
	void updateState(QPoint mouse_pos);

	DragHandler& m_rDragHandler;
	QDateTime m_dragStartTime;
	QPoint m_dragStartPos;
	int m_dragMaxSqDist;
	bool m_dragInProgress;
};

#endif
