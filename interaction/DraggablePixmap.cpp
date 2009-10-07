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

#include "DraggablePixmap.h"
#include "Proximity.h"
#include "ImageViewBase.h"
#include <QPainter>
#include <QTransform>
#include <QRectF>
#include <algorithm>

DraggablePixmap::DraggablePixmap(int id, QPixmap const& pixmap, int proximity_priority)
:	m_pixmap(pixmap),
	m_hitAreaRadius(0.25 * (pixmap.width() + pixmap.height())),
	m_id(id),
	m_proximityPriority(proximity_priority)
{
}

void
DraggablePixmap::paint(QPainter& painter, InteractionState const& interaction)
{
	if (!isPixmapToBeDrawn(m_id, interaction)) {
		return;
	}

	painter.setTransform(QTransform());

	QRectF rect(m_pixmap.rect());
	rect.moveCenter(position(interaction));
	painter.drawPixmap(rect.topLeft(), m_pixmap);
}

Proximity
DraggablePixmap::proximityThreshold(InteractionState const&) const
{
	return Proximity::fromDist(m_hitAreaRadius);
}

int
DraggablePixmap::proximityPriority() const
{
	return m_proximityPriority;
}

Proximity
DraggablePixmap::proximity(
	QPointF const& widget_mouse_pos, InteractionState const& interaction)
{
	return Proximity(position(interaction), widget_mouse_pos);
}

QPointF
DraggablePixmap::position(InteractionState const& interaction) const
{
	return pixmapPosition(m_id, interaction);
}

void
DraggablePixmap::moveRequest(QPointF const& widget_pos)
{
	pixmapMoveRequest(m_id, widget_pos);
}
