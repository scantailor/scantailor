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
#include <QRectF>
#include <QSizeF>
#include <QTransform>
#include <QPainter>
#include <QPen>
#include <QBrush>
#include <QColor>

namespace select_content
{

Thumbnail::Thumbnail(
	IntrusivePtr<ThumbnailPixmapCache> const& thumbnail_cache,
	QSizeF const& max_size, ImageId const& image_id,
	ImageTransformation const& xform, QRectF const& content_rect)
:	ThumbnailBase(thumbnail_cache, max_size, image_id, xform),
	m_contentRect(content_rect)
{
}

void
Thumbnail::paintOverImage(
	QPainter& painter, QTransform const& image_to_display,
	QTransform const& thumb_to_display)
{
	if (m_contentRect.isNull()) {
		return;
	}
	
	painter.setRenderHint(QPainter::Antialiasing, false);
	
	QPen pen(QColor(0x00, 0x00, 0xff));
	pen.setWidth(1);
	pen.setCosmetic(true);
	painter.setPen(pen);
	
	painter.setBrush(QColor(0x00, 0x00, 0xff, 50));
	
	QRectF content_rect(virtToThumb().mapRect(m_contentRect));
	
	// Adjust to compensate for pen width.
	content_rect.adjust(-1, -1, 1, 1);
	
	// toRect() is necessary because we turn off antialiasing.
	// For some reason, if we let Qt round the coordinates,
	// the result is slightly different.
	painter.drawRect(content_rect.toRect());
}

} // namespace select_content
