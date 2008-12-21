/*
    Scan Tailor - Interactive post-processing tool for scanned pages.
    Copyright (C) 2007-2008  Joseph Artsimovich <joseph_a@mail.ru>

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
	ThumbnailPixmapCache& thumbnail_cache, QSizeF const& max_size,
	ImageId const& image_id, ImageTransformation const& xform,
	PageLayout const& layout)
:	ThumbnailBase(thumbnail_cache, max_size, image_id, xform),
	m_layout(layout)
{
}

void
Thumbnail::paintOverImage(
	QPainter& painter, QTransform const& image_to_display,
	QTransform const& thumb_to_display)
{
	QRectF const rect(imageXform().resultingRect());
	
	painter.setRenderHint(QPainter::Antialiasing, false);
	painter.setWorldTransform(image_to_display);
	
	painter.setPen(Qt::NoPen);
	switch (m_layout.type()) {
		case PageLayout::SINGLE_PAGE_UNCUT:
			painter.setBrush(QColor(0, 0, 255, 50));
			painter.drawRect(rect);
			return; // No split line will be drawn.
		case PageLayout::LEFT_PAGE_PLUS_OFFCUT:
		case PageLayout::RIGHT_PAGE_PLUS_OFFCUT:
			painter.setBrush(QColor(0, 0, 255, 50));
			painter.drawPolygon(
				m_layout.singlePageOutline(rect)
			);
			break;
		case PageLayout::TWO_PAGES:
			painter.setBrush(QColor(0, 0, 255, 50));
			painter.drawPolygon(
				m_layout.leftPageOutline(rect)
			);
			painter.setBrush(QColor(255, 0, 0, 50));
			painter.drawPolygon(
				m_layout.rightPageOutline(rect)
			);
			break;
	}
	
	painter.setRenderHint(QPainter::Antialiasing, true);
	
	QPen pen(QColor(0x00, 0x00, 0xff));
	pen.setWidth(1);
	pen.setCosmetic(true);
	painter.setPen(pen);
	
	painter.drawLine(m_layout.inscribedSplitLine(rect));
}

} // namespace page_split
