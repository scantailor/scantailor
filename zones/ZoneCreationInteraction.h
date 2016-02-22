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
#include "DragWatcher.h"
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
	void updateStatusTip();

	ZoneInteractionContext& m_rContext;

	/**
	 * We have our own drag handler even though there is already a global one
	 * for the purpose of being able to monitor it with DragWatcher.  Because
	 * we capture a state in the constructor, it's guaranteed the global
	 * drag handler will not be functioning until we release the state.
	 */
	DragHandler m_dragHandler;

	/**
	 * This must go after m_dragHandler, otherwise DragHandler's destructor
	 * will try to destroy this object.
	 */
	DragWatcher m_dragWatcher;

	/**
	 * Because we hold an interaction state from constructor to destructor,
	 * we have to have our own zoom handler with explicit interaction permission
	 * if we want zoom to work.
	 */
	ZoomHandler m_zoomHandler;

	BasicSplineVisualizer m_visualizer;
	InteractionState::Captor m_interaction;
	EditableSpline::Ptr m_ptrSpline;
	QPointF m_nextVertexImagePos;
//begin of modified by monday2000
//Square_Picture_Zones
//added
	bool m_ctrl;
	QPointF m_nextVertexImagePos_mid1;
	QPointF m_nextVertexImagePos_mid2;
//end of modified by monday2000
};

#endif
