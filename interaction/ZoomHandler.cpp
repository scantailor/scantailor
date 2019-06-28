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

#include "ZoomHandler.h"
#include "InteractionState.h"
#include "ImageViewBase.h"
#include <QWheelEvent>
#include <QKeyEvent>
#include <QRectF>
#include <math.h>

ZoomHandler::ZoomHandler(ImageViewBase& image_view)
:	m_rImageView(image_view),
	m_interactionPermitter(&InteractionHandler::defaultInteractionPermitter),
	m_focus(CURSOR)
{
}

ZoomHandler::ZoomHandler(
	ImageViewBase& image_view,
	boost::function<bool(InteractionState const&)> const& explicit_interaction_permitter)
:	m_rImageView(image_view),
	m_interactionPermitter(explicit_interaction_permitter),
	m_focus(CURSOR)
{

}

void
ZoomHandler::onWheelEvent(QWheelEvent* event, InteractionState& interaction)
{
	if (event->orientation() != Qt::Vertical) {
		return;
	}

	if (!m_interactionPermitter(interaction)) {
		return;
	}

	event->accept();

	double zoom = m_rImageView.zoomLevel();

	if (zoom == 1.0 && event->delta() < 0) {
		// Already zoomed out and trying to zoom out more.
		
		// Scroll amount in terms of typical mouse wheel "clicks".
		double const delta_clicks = event->delta() / 120;
		
		double const dist = -delta_clicks * 30; // 30px per "click"
		m_rImageView.moveTowardsIdealPosition(dist);
		return;
	}

	double const degrees = event->delta() / 8.0;
	zoom *= pow(2.0, degrees / 60.0); // 2 times zoom for every 60 degrees

	if (zoom < 1.0) {
		zoom = 1.0;
	}

	QPointF focus_point;
	switch (m_focus) {
		case CENTER:
			focus_point = QRectF(m_rImageView.rect()).center();
			break;
		case CURSOR:
			focus_point = event->pos() + QPointF(0.5, 0.5);
			break;
	}
	m_rImageView.setWidgetFocalPointWithoutMoving(focus_point);
	m_rImageView.setZoomLevel(zoom); // this will call update()
}

void
ZoomHandler::onKeyPressEvent(QKeyEvent* event, InteractionState& interaction)
{	
	if (!m_interactionPermitter(interaction)) {
		return;
	}

	double zoom = m_rImageView.zoomLevel();

	switch (event->key()) {
		case Qt::Key_Plus:
			zoom *= 1.12246205; // == 2^( 1/6);
			break;
		case Qt::Key_Minus:
			zoom *= 0.89089872; // == 2^(-1/6);
			break;
		default:
			return;
	}

	QPointF focus_point = QRectF(m_rImageView.rect()).center();

	m_rImageView.setWidgetFocalPointWithoutMoving(focus_point);
	m_rImageView.setZoomLevel(zoom); // this will call update()
}

