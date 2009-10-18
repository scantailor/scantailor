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

#include "InteractionHandler.h"
#include "InteractionState.h"
#include <QPainter>
#include <boost/lambda/lambda.hpp>
#include <boost/lambda/construct.hpp>
#include <boost/lambda/bind.hpp>
#include <assert.h>

#define DISPATCH(list, call) {                   \
	HandlerList::iterator it(list.begin());      \
	HandlerList::iterator const end(list.end()); \
	while (it != end) {                          \
		(it++)->call;                            \
	}                                            \
}

InteractionHandler::~InteractionHandler()
{
	using namespace boost::lambda;
	m_preceeders.clear_and_dispose(bind(delete_ptr(), _1));
	m_followers.clear_and_dispose(bind(delete_ptr(), _1));
}

void
InteractionHandler::paint(
	QPainter& painter, InteractionState const& interaction)
{
	DISPATCH(m_preceeders, paint(painter, interaction));
	painter.save();
	onPaint(painter, interaction);
	painter.restore();
	DISPATCH(m_followers, paint(painter, interaction));
}

void
InteractionHandler::proximityUpdate(
	QPointF const& screen_mouse_pos, InteractionState& interaction)
{
	DISPATCH(m_preceeders, proximityUpdate(screen_mouse_pos, interaction));
	onProximityUpdate(screen_mouse_pos, interaction);
	assert(!interaction.captured() && "onProximityUpdate() must not capture interaction");
	DISPATCH(m_followers, proximityUpdate(screen_mouse_pos, interaction));
}

void
InteractionHandler::keyPressEvent(QKeyEvent* event, InteractionState& interaction)
{
	DISPATCH(m_preceeders, keyPressEvent(event, interaction));
	onKeyPressEvent(event, interaction);
	DISPATCH(m_followers, keyPressEvent(event, interaction));
}

void
InteractionHandler::keyReleaseEvent(QKeyEvent* event, InteractionState& interaction)
{
	DISPATCH(m_preceeders, keyReleaseEvent(event, interaction));
	onKeyReleaseEvent(event, interaction);
	DISPATCH(m_followers, keyReleaseEvent(event, interaction));
}

void
InteractionHandler::mousePressEvent(QMouseEvent* event, InteractionState& interaction)
{
	DISPATCH(m_preceeders, mousePressEvent(event, interaction));
	onMousePressEvent(event, interaction);
	DISPATCH(m_followers, mousePressEvent(event, interaction));
}

void
InteractionHandler::mouseReleaseEvent(QMouseEvent* event, InteractionState& interaction)
{
	DISPATCH(m_preceeders, mouseReleaseEvent(event, interaction));
	onMouseReleaseEvent(event, interaction);
	DISPATCH(m_followers, mouseReleaseEvent(event, interaction));
}

void
InteractionHandler::mouseMoveEvent(QMouseEvent* event, InteractionState& interaction)
{
	DISPATCH(m_preceeders, mouseMoveEvent(event, interaction));
	onMouseMoveEvent(event, interaction);
	DISPATCH(m_followers, mouseMoveEvent(event, interaction));
}

void
InteractionHandler::wheelEvent(QWheelEvent* event, InteractionState& interaction)
{
	DISPATCH(m_preceeders, wheelEvent(event, interaction));
	onWheelEvent(event, interaction);
	DISPATCH(m_followers, wheelEvent(event, interaction));
}

void
InteractionHandler::contextMenuEvent(
	QContextMenuEvent* event, InteractionState& interaction)
{
	DISPATCH(m_preceeders, contextMenuEvent(event, interaction));
	onContextMenuEvent(event, interaction);
	DISPATCH(m_followers, contextMenuEvent(event, interaction));
}

void
InteractionHandler::makePeerPreceeder(InteractionHandler& handler)
{
	handler.unlink();
	HandlerList::node_algorithms::link_before(this, &handler);
}

void
InteractionHandler::makePeerFollower(InteractionHandler& handler)
{
	using namespace boost::intrusive;
	handler.unlink();
	HandlerList::node_algorithms::link_after(this, &handler);
}

void
InteractionHandler::makeFirstPreceeder(InteractionHandler& handler)
{
	handler.unlink();
	m_preceeders.push_front(handler);
}

void
InteractionHandler::makeLastPreceeder(InteractionHandler& handler)
{
	handler.unlink();
	m_preceeders.push_back(handler);
}

void
InteractionHandler::makeFirstFollower(InteractionHandler& handler)
{
	handler.unlink();
	m_followers.push_front(handler);
}

void
InteractionHandler::makeLastFollower(InteractionHandler& handler)
{
	handler.unlink();
	m_followers.push_back(handler);
}
