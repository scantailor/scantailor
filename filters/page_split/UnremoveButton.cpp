/*
	Scan Tailor - Interactive post-processing tool for scanned pages.
	Copyright (C)  Joseph Artsimovich <joseph.artsimovich@gmail.com>

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

#include "UnremoveButton.h"
#include "Proximity.h"
#include <QRectF>
#include <QTransform>
#include <QPainter>
#include <QCursor>
#include <QMouseEvent>

namespace page_split
{

UnremoveButton::UnremoveButton(PositionGetter const& position_getter)
:	m_positionGetter(position_getter),
	m_clickCallback(&UnremoveButton::noOp),
	m_defaultPixmap(":/icons/trashed-big.png"),
	m_hoveredPixmap(":/icons/untrash-big.png"),
	m_wasHovered(false)
{
	m_proximityInteraction.setProximityCursor(Qt::PointingHandCursor);
	m_proximityInteraction.setProximityStatusTip(tr("Restore removed page."));
}

void
UnremoveButton::onPaint(QPainter& painter, InteractionState const& interaction)
{
	QPixmap const& pixmap = interaction.proximityLeader(m_proximityInteraction)
		? m_hoveredPixmap : m_defaultPixmap;

	QRectF rect(pixmap.rect());
	rect.moveCenter(m_positionGetter());
	
	painter.setWorldTransform(QTransform());
	painter.drawPixmap(rect.topLeft(), pixmap);
}

void
UnremoveButton::onProximityUpdate(
	QPointF const& screen_mouse_pos, InteractionState& interaction)
{
	QRectF rect(m_defaultPixmap.rect());
	rect.moveCenter(m_positionGetter());

	bool const hovered = rect.contains(screen_mouse_pos);
	if (hovered != m_wasHovered) {
		m_wasHovered = hovered;
		interaction.setRedrawRequested(true);
	}

	interaction.updateProximity(
		m_proximityInteraction, Proximity::fromSqDist(hovered ? 0.0 : 1e10)
	);
}

void
UnremoveButton::onMousePressEvent(QMouseEvent* event, InteractionState& interaction)
{
	if (!interaction.captured() && interaction.proximityLeader(m_proximityInteraction)) {
		event->accept();
		m_clickCallback();
	}
}

} // namespace page_split
