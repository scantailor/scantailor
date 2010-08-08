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

#ifndef OBJECT_DRAG_HANDLER_H_
#define OBJECT_DRAG_HANDLER_H_

#include "NonCopyable.h"
#include "InteractionHandler.h"
#include "InteractionState.h"
#include "DraggableObject.h"
#include <QPointF>
#include <QPointF>

class QPainter;
class QCursor;
class QString;

class ObjectDragHandler : public InteractionHandler
{
	DECLARE_NON_COPYABLE(ObjectDragHandler)
public:
	ObjectDragHandler(DraggableObject* obj = 0);

	void setObject(DraggableObject* obj) { m_pObj = obj; }

	void setProximityCursor(QCursor const& cursor);

	void setInteractionCursor(QCursor const& cursor);

	void setProximityStatusTip(QString const& tip);

	void setInteractionStatusTip(QString const& tip);

	bool interactionInProgress(InteractionState const& interaction) const;

	bool proximityLeader(InteractionState const& interaction) const;

	void forceEnterDragState(InteractionState& interaction, QPoint widget_mouse_pos);
protected:
	virtual void onPaint(
		QPainter& painter, InteractionState const& interaction);

	virtual void onProximityUpdate(
		QPointF const& screen_mouse_pos, InteractionState& interaction);

	virtual void onMousePressEvent(
		QMouseEvent* event, InteractionState& interaction);

	virtual void onMouseReleaseEvent(
		QMouseEvent* event, InteractionState& interaction);

	virtual void onMouseMoveEvent(
		QMouseEvent* event, InteractionState& interaction);
private:
	DraggableObject* m_pObj;
	InteractionState::Captor m_interaction;
};

#endif
