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

#ifndef ZONE_CREATION_INTERACTION_H_
#define ZONE_CREATION_INTERACTION_H_

#include "InteractionHandler.h"
#include "InteractionState.h"
#include "DragHandler.h"
#include "ZoomHandler.h"
#include "BasicSplineVisualizer.h"
#include "EditableSpline.h"
#include <QPointF>
#include <QDateTime>
#include <QCoreApplication>

class ZoneInteractionContext;

class ZoneCreationInteraction : public InteractionHandler
{
	Q_DECLARE_TR_FUNCTIONS(ZoneCreationInteraction)
public:
	ZoneCreationInteraction(
		ZoneInteractionContext& context, InteractionState& interaction);
protected:
	ZoneInteractionContext& context() { return m_rContext; }

	virtual void onPaint(QPainter& painter, InteractionState const& interaction);

	virtual void onKeyPressEvent(QKeyEvent* event, InteractionState& interaction);

	virtual void onMouseReleaseEvent(QMouseEvent* event, InteractionState& interaction);

	virtual void onMouseMoveEvent(QMouseEvent* event, InteractionState& interaction);
private:
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

	void updateStatusTip();

	ZoneInteractionContext& m_rContext;
	DragHandler m_dragHandler;
	DragWatcher m_dragWatcher; // Must go after m_dragHandler.
	ZoomHandler m_zoomHandler;
	BasicSplineVisualizer m_visualizer;
	InteractionState::Captor m_interaction;
	EditableSpline::Ptr m_ptrSpline;
	QPointF m_nextVertexImagePos;
};

#endif
