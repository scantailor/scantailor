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

#ifndef INTERACTION_HANDLER_H_
#define INTERACTION_HANDLER_H_

#include "NonCopyable.h"
#include "RefCountable.h"
#include "IntrusivePtr.h"
#include <boost/intrusive/list.hpp>

class InteractionState;
class QPainter;
class QKeyEvent;
class QMouseEvent;
class QWheelEvent;
class QContextMenuEvent;
class QPointF;

class InteractionHandler :
	public boost::intrusive::list_base_hook<
		boost::intrusive::link_mode<boost::intrusive::auto_unlink>
	>
{
	DECLARE_NON_COPYABLE(InteractionHandler)
public:
	InteractionHandler();

	virtual ~InteractionHandler();

	void paint(QPainter& painter, InteractionState const& interaction);

	void proximityUpdate(QPointF const& screen_mouse_pos, InteractionState& interaction);

	void keyPressEvent(QKeyEvent* event, InteractionState& interaction);

	void keyReleaseEvent(QKeyEvent* event, InteractionState& interaction);

	void mousePressEvent(QMouseEvent* event, InteractionState& interaction);

	void mouseReleaseEvent(QMouseEvent* event, InteractionState& interaction);

	void mouseMoveEvent(QMouseEvent* event, InteractionState& interaction);

	void wheelEvent(QWheelEvent* event, InteractionState& interaction);

	void contextMenuEvent(QContextMenuEvent* event, InteractionState& interaction);

	void makePeerPreceeder(InteractionHandler& handler);

	void makePeerFollower(InteractionHandler& handler);

	void makeFirstPreceeder(InteractionHandler& handler);

	void makeLastPreceeder(InteractionHandler& handler);

	void makeFirstFollower(InteractionHandler& handler);

	void makeLastFollower(InteractionHandler& handler);
protected:
	virtual void onPaint(
		QPainter& painter, InteractionState const& interaction) {}

	virtual void onProximityUpdate(
		QPointF const& screen_mouse_pos, InteractionState& interaction) {}

	virtual void onKeyPressEvent(
		QKeyEvent* event, InteractionState& interaction) {}

	virtual void onKeyReleaseEvent(
		QKeyEvent* event, InteractionState& interaction) {}

	virtual void onMousePressEvent(
		QMouseEvent* event, InteractionState& interaction) {}

	virtual void onMouseReleaseEvent(
		QMouseEvent* event, InteractionState& interaction) {}

	virtual void onMouseMoveEvent(
		QMouseEvent* event, InteractionState& interaction) {}

	virtual void onWheelEvent(
		QWheelEvent* event, InteractionState& interaction) {}

	virtual void onContextMenuEvent(
		QContextMenuEvent* event, InteractionState& interaction) {}

	static bool defaultInteractionPermitter(InteractionState const& interaction);
private:
	class HandlerList :
		public RefCountable,
		public boost::intrusive::list<
			InteractionHandler, boost::intrusive::constant_time_size<false>
		>
	{
	};

	IntrusivePtr<HandlerList> m_ptrPreceeders;
	IntrusivePtr<HandlerList> m_ptrFollowers;
};

#endif
