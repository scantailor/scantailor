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

#include "DragHandler.h"
#include "ImageViewBase.h"
#include <QMouseEvent>
#include <QPointF>
#include <QPoint>
#include <Qt>

DragHandler::DragHandler(ImageViewBase& image_view)
:	m_rImageView(image_view),
	m_interactionPermitter(&InteractionHandler::defaultInteractionPermitter)
{
	init();
}

DragHandler::DragHandler(
	ImageViewBase& image_view,
	boost::function<bool(InteractionState const&)> const& explicit_interaction_permitter)
:	m_rImageView(image_view),
	m_interactionPermitter(explicit_interaction_permitter)
{
	init();
}

void
DragHandler::init()
{
	m_interaction.setInteractionStatusTip(
		tr("Unrestricted dragging is possible by holding down the Shift key.")
	);
}

bool
DragHandler::isActive() const
{
	return m_rImageView.interactionState().capturedBy(m_interaction);
}

void
DragHandler::onMousePressEvent(QMouseEvent* event, InteractionState& interaction)
{
	m_lastMousePos = event->pos();

	if ((event->buttons() & (Qt::LeftButton|Qt::MidButton)) &&
			!interaction.capturedBy(m_interaction)
			&& m_interactionPermitter(interaction)) {
		interaction.capture(m_interaction);
	}
}

void
DragHandler::onMouseReleaseEvent(QMouseEvent* event, InteractionState& interaction)
{
	if (interaction.capturedBy(m_interaction)) {
		m_interaction.release();
		event->accept();
	}
}

void
DragHandler::onMouseMoveEvent(QMouseEvent* event, InteractionState& interaction)
{
	if (interaction.capturedBy(m_interaction)) {
		QPoint movement(event->pos());
		movement -= m_lastMousePos;
		m_lastMousePos = event->pos();

		QPointF adjusted_fp(m_rImageView.getWidgetFocalPoint());
		adjusted_fp += movement;

		// These will call update() if necessary.
		if (event->modifiers() & Qt::ShiftModifier) {
			m_rImageView.setWidgetFocalPoint(adjusted_fp);
		} else {
			m_rImageView.adjustAndSetWidgetFocalPoint(adjusted_fp);
		}
	}
}
