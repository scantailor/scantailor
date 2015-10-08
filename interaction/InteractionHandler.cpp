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
#include "NonCopyable.h"
#include <QPainter>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QWheelEvent>
#include <QContextMenuEvent>
#ifndef Q_MOC_RUn
#include <boost/lambda/lambda.hpp>
#include <boost/lambda/construct.hpp>
#include <boost/lambda/bind.hpp>
#endif
#include <assert.h>

#define DISPATCH(list, call) {                    \
	HandlerList::iterator it(list->begin());      \
	HandlerList::iterator const end(list->end()); \
	while (it != end) {                           \
		(it++)->call;                             \
	}                                             \
}

#define RETURN_IF_ACCEPTED(event) {               \
	if (event->isAccepted()) {                    \
		return;                                   \
	}                                             \
}

namespace
{

class ScopedClearAcceptance
{
	DECLARE_NON_COPYABLE(ScopedClearAcceptance)
public:
	ScopedClearAcceptance(QEvent* event);

	~ScopedClearAcceptance();
private:
	QEvent* m_pEvent;
	bool m_wasAccepted;
};

ScopedClearAcceptance::ScopedClearAcceptance(QEvent* event)
:	m_pEvent(event),
	m_wasAccepted(event->isAccepted())
{
	m_pEvent->setAccepted(false);
}

ScopedClearAcceptance::~ScopedClearAcceptance()
{
	if (m_wasAccepted) {
		m_pEvent->setAccepted(true);
	}
}

} // anonymous namespace

InteractionHandler::InteractionHandler()
:	m_ptrPreceeders(new HandlerList),
	m_ptrFollowers(new HandlerList)
{
}

InteractionHandler::~InteractionHandler()
{
	using namespace boost::lambda;
	m_ptrPreceeders->clear_and_dispose(bind(delete_ptr(), _1));
	m_ptrFollowers->clear_and_dispose(bind(delete_ptr(), _1));
}

void
InteractionHandler::paint(
	QPainter& painter, InteractionState const& interaction)
{
	// Keep them alive in case this object gets destroyed.
	IntrusivePtr<HandlerList> preceeders(m_ptrPreceeders);
	IntrusivePtr<HandlerList> followers(m_ptrFollowers);

	DISPATCH(preceeders, paint(painter, interaction));
	painter.save();
	onPaint(painter, interaction);
	painter.restore();
	DISPATCH(followers, paint(painter, interaction));
}

void
InteractionHandler::proximityUpdate(
	QPointF const& screen_mouse_pos, InteractionState& interaction)
{
	// Keep them alive in case this object gets destroyed.
	IntrusivePtr<HandlerList> preceeders(m_ptrPreceeders);
	IntrusivePtr<HandlerList> followers(m_ptrFollowers);

	DISPATCH(preceeders, proximityUpdate(screen_mouse_pos, interaction));
	onProximityUpdate(screen_mouse_pos, interaction);
	assert(!interaction.captured() && "onProximityUpdate() must not capture interaction");
	DISPATCH(followers, proximityUpdate(screen_mouse_pos, interaction));
}

void
InteractionHandler::keyPressEvent(QKeyEvent* event, InteractionState& interaction)
{
	RETURN_IF_ACCEPTED(event);

	// Keep them alive in case this object gets destroyed.
	IntrusivePtr<HandlerList> preceeders(m_ptrPreceeders);
	IntrusivePtr<HandlerList> followers(m_ptrFollowers);

	DISPATCH(preceeders, keyPressEvent(event, interaction));
	RETURN_IF_ACCEPTED(event);
	onKeyPressEvent(event, interaction);
	ScopedClearAcceptance guard(event);
	DISPATCH(followers, keyPressEvent(event, interaction));
}

void
InteractionHandler::keyReleaseEvent(QKeyEvent* event, InteractionState& interaction)
{
	RETURN_IF_ACCEPTED(event);

	// Keep them alive in case this object gets destroyed.
	IntrusivePtr<HandlerList> preceeders(m_ptrPreceeders);
	IntrusivePtr<HandlerList> followers(m_ptrFollowers);

	DISPATCH(preceeders, keyReleaseEvent(event, interaction));
	RETURN_IF_ACCEPTED(event);
	onKeyReleaseEvent(event, interaction);
	ScopedClearAcceptance guard(event);
	DISPATCH(followers, keyReleaseEvent(event, interaction));
}

void
InteractionHandler::mousePressEvent(QMouseEvent* event, InteractionState& interaction)
{
	RETURN_IF_ACCEPTED(event);

	// Keep them alive in case this object gets destroyed.
	IntrusivePtr<HandlerList> preceeders(m_ptrPreceeders);
	IntrusivePtr<HandlerList> followers(m_ptrFollowers);

	DISPATCH(preceeders, mousePressEvent(event, interaction));
	RETURN_IF_ACCEPTED(event);
	onMousePressEvent(event, interaction);
	ScopedClearAcceptance guard(event);
	DISPATCH(followers, mousePressEvent(event, interaction));
}

void
InteractionHandler::mouseReleaseEvent(QMouseEvent* event, InteractionState& interaction)
{
	RETURN_IF_ACCEPTED(event);

	// Keep them alive in case this object gets destroyed.
	IntrusivePtr<HandlerList> preceeders(m_ptrPreceeders);
	IntrusivePtr<HandlerList> followers(m_ptrFollowers);

	DISPATCH(preceeders, mouseReleaseEvent(event, interaction));
	RETURN_IF_ACCEPTED(event);
	onMouseReleaseEvent(event, interaction);
	ScopedClearAcceptance guard(event);
	DISPATCH(followers, mouseReleaseEvent(event, interaction));
}

void
InteractionHandler::mouseMoveEvent(QMouseEvent* event, InteractionState& interaction)
{
	RETURN_IF_ACCEPTED(event);

	// Keep them alive in case this object gets destroyed.
	IntrusivePtr<HandlerList> preceeders(m_ptrPreceeders);
	IntrusivePtr<HandlerList> followers(m_ptrFollowers);

	DISPATCH(preceeders, mouseMoveEvent(event, interaction));
	RETURN_IF_ACCEPTED(event);
	onMouseMoveEvent(event, interaction);
	ScopedClearAcceptance guard(event);
	DISPATCH(followers, mouseMoveEvent(event, interaction));
}

void
InteractionHandler::wheelEvent(QWheelEvent* event, InteractionState& interaction)
{
	RETURN_IF_ACCEPTED(event);

	// Keep them alive in case this object gets destroyed.
	IntrusivePtr<HandlerList> preceeders(m_ptrPreceeders);
	IntrusivePtr<HandlerList> followers(m_ptrFollowers);

	DISPATCH(preceeders, wheelEvent(event, interaction));
	RETURN_IF_ACCEPTED(event);
	onWheelEvent(event, interaction);
	ScopedClearAcceptance guard(event);
	DISPATCH(followers, wheelEvent(event, interaction));
}

void
InteractionHandler::contextMenuEvent(
	QContextMenuEvent* event, InteractionState& interaction)
{
	RETURN_IF_ACCEPTED(event);

	// Keep them alive in case this object gets destroyed.
	IntrusivePtr<HandlerList> preceeders(m_ptrPreceeders);
	IntrusivePtr<HandlerList> followers(m_ptrFollowers);

	DISPATCH(preceeders, contextMenuEvent(event, interaction));
	RETURN_IF_ACCEPTED(event);
	onContextMenuEvent(event, interaction);
	ScopedClearAcceptance guard(event);
	DISPATCH(followers, contextMenuEvent(event, interaction));
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
	m_ptrPreceeders->push_front(handler);
}

void
InteractionHandler::makeLastPreceeder(InteractionHandler& handler)
{
	handler.unlink();
	m_ptrPreceeders->push_back(handler);
}

void
InteractionHandler::makeFirstFollower(InteractionHandler& handler)
{
	handler.unlink();
	m_ptrFollowers->push_front(handler);
}

void
InteractionHandler::makeLastFollower(InteractionHandler& handler)
{
	handler.unlink();
	m_ptrFollowers->push_back(handler);
}

bool
InteractionHandler::defaultInteractionPermitter(InteractionState const& interaction)
{
	return !interaction.captured();
}
