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

#include "Thumbnail.h"
#include <QPolygonF>
#include <QPointF>
#include <QRectF>
#include <QSizeF>
#include <QTransform>
#include <QPainter>
#include <QPen>
#include <QBrush>
#include <QColor>

namespace page_split
{

Thumbnail::Thumbnail(
	IntrusivePtr<ThumbnailPixmapCache> const& thumbnail_cache,
	QSizeF const& max_size, ImageId const& image_id,
	ImageTransformation const& xform, PageLayout const& layout,
	bool left_half_removed, bool right_half_removed)
:	ThumbnailBase(thumbnail_cache, max_size, image_id, xform),
	m_layout(layout),
	m_leftHalfRemoved(left_half_removed),
	m_rightHalfRemoved(right_half_removed)
{
	if (left_half_removed || right_half_removed) {
		m_trashPixmap = QPixmap(":/icons/trashed-small.png");
	}
}

void
Thumbnail::paintOverImage(
	QPainter& painter, QTransform const& image_to_display,
	QTransform const& thumb_to_display)
{
	QRectF const canvas_rect(imageXform().resultingRect());
	
	painter.setRenderHint(QPainter::Antialiasing, false);
	painter.setWorldTransform(image_to_display);
	
	painter.setPen(Qt::NoPen);
	switch (m_layout.type()) {
		case PageLayout::SINGLE_PAGE_UNCUT:
			painter.setBrush(QColor(0, 0, 255, 50));
			painter.drawRect(canvas_rect);
			return; // No split line will be drawn.
		case PageLayout::SINGLE_PAGE_CUT:
			painter.setBrush(QColor(0, 0, 255, 50));
			painter.drawPolygon(m_layout.singlePageOutline());
			break;
		case PageLayout::TWO_PAGES:
			QPolygonF const left_poly(m_layout.leftPageOutline());
			QPolygonF const right_poly(m_layout.rightPageOutline());
			painter.setBrush(m_leftHalfRemoved ? QColor(0, 0, 0, 80) : QColor(0, 0, 255, 50));
			painter.drawPolygon(left_poly);
			painter.setBrush(m_rightHalfRemoved ? QColor(0, 0, 0, 80) : QColor(255, 0, 0, 50));
			painter.drawPolygon(right_poly);
			
			// Draw the trash icon.
			if (m_leftHalfRemoved || m_rightHalfRemoved) {
				painter.setWorldTransform(QTransform());
				
				int const subpage_idx = m_leftHalfRemoved ? 0 : 1;
				QPointF const center(
					subPageCenter(left_poly, right_poly, image_to_display, subpage_idx)
				);

				QRectF rect(m_trashPixmap.rect());				
				rect.moveCenter(center);
				painter.drawPixmap(rect.topLeft(), m_trashPixmap);
				
				painter.setWorldTransform(image_to_display);
			}
			break;
	}
	
	painter.setRenderHint(QPainter::Antialiasing, true);
	
	QPen pen(QColor(0x00, 0x00, 0xff));
	pen.setWidth(1);
	pen.setCosmetic(true);
	painter.setPen(pen);
	
	switch (m_layout.type()) {
		case PageLayout::SINGLE_PAGE_CUT:
			painter.drawLine(m_layout.inscribedCutterLine(0));
			painter.drawLine(m_layout.inscribedCutterLine(1));
			break;
		case PageLayout::TWO_PAGES:
			painter.drawLine(m_layout.inscribedCutterLine(0));
			break;
		default:;
	}
	
}

QPointF
Thumbnail::subPageCenter(
	QPolygonF const& left_page, QPolygonF const& right_page,
	QTransform const& image_to_display, int subpage_idx)
{
	QRectF rects[2];
	rects[0] = left_page.boundingRect();
	rects[1] = right_page.boundingRect();

	double const x_mid = 0.5 * (rects[0].right() + rects[1].left());
	rects[0].setRight(x_mid);
	rects[1].setLeft(x_mid);

	return image_to_display.map(rects[subpage_idx].center());
}

} // namespace page_split
