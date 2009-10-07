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
#include <QRectF>
#include <math.h>

ZoomHandler::ZoomHandler(ImageViewBase& image_view)
:	m_rImageView(image_view),
	m_zoom(1.0),
	m_focus(CURSOR)
{
}

void
ZoomHandler::onWheelEvent(QWheelEvent* event, InteractionState& interaction)
{
	if (interaction.captured()) {
		return;
	}

	double const degrees = event->delta() / 8.0;
	m_zoom *= pow(2.0, degrees / 60.0); // 2 times zoom for every 60 degrees
	if (m_zoom < 1.0) {
		m_zoom = 1.0;
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
	m_rImageView.zoom(m_zoom); // this will call update()
}
