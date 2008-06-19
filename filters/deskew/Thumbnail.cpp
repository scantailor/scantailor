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
#include <QLineF>
#include <QTransform>
#include <QPainter>
#include <QPen>
#include <QBrush>
#include <QColor>

namespace deskew
{

Thumbnail::Thumbnail(
	ThumbnailPixmapCache& thumbnail_cache, QSizeF const& max_size,
	ImageId const& image_id, ImageTransformation const& xform)
:	ThumbnailBase(thumbnail_cache, max_size, image_id, xform)
{
}

void
Thumbnail::paintOverImage(
	QPainter& painter, QTransform const& image_to_display,
	QTransform const& thumb_to_display)
{
	painter.setRenderHint(QPainter::Antialiasing, false);
	
	QPen pen(QColor(0x00, 0x00, 0xff));
	pen.setWidth(1);
	pen.setCosmetic(true);
	painter.setPen(pen);
	
	QRectF const bounding_rect(boundingRect());
	
	QLineF vert_line(bounding_rect.topLeft(), bounding_rect.bottomLeft());
	vert_line.translate(0.5 * bounding_rect.width(), 0.0);
	painter.drawLine(vert_line);
	
	QLineF hor_line(bounding_rect.topLeft(), bounding_rect.topRight());
	hor_line.translate(0.0, 0.5 * bounding_rect.height());
	painter.drawLine(hor_line);
}

} // namespace deskew
