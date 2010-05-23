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
#include <QLineF>
#include <QTransform>
#include <QPainter>
#include <QPen>
#include <QBrush>
#include <QColor>
#include <QVector>
#include <QLineF>

namespace deskew
{

Thumbnail::Thumbnail(
	IntrusivePtr<ThumbnailPixmapCache> const& thumbnail_cache,
	QSizeF const& max_size, ImageId const& image_id,
	ImageTransformation const& xform)
:	ThumbnailBase(thumbnail_cache, max_size, image_id, xform)
{
}

void
Thumbnail::paintOverImage(
	QPainter& painter, QTransform const& image_to_display,
	QTransform const& thumb_to_display)
{
	painter.setRenderHint(QPainter::Antialiasing, false);
	
	QPen pen(QColor(0, 0, 255, 70));
	pen.setWidth(1);
	pen.setCosmetic(true);
	painter.setPen(pen);
	
	QRectF const bounding_rect(boundingRect());
	
	double const cell_size = 8;
	double const left = bounding_rect.left();
	double const right = bounding_rect.right();
	double const top = bounding_rect.top();
	double const bottom = bounding_rect.bottom();
	double const w = bounding_rect.width();
	double const h = bounding_rect.height();
	
	QPointF const center(bounding_rect.center());
	QVector<QLineF> lines;
	for (double y = center.y(); y > 0.0; y -= cell_size) {
		lines.push_back(QLineF(left, y, right, y));
	}
	for (double y = center.y(); (y += cell_size) < h;) {
		lines.push_back(QLineF(left, y, right, y));
	}
	for (double x = center.x(); x > 0.0; x -= cell_size) {
		lines.push_back(QLineF(x, top, x, bottom));
	}
	for (double x = center.x(); (x += cell_size) < w;) {
		lines.push_back(QLineF(x, top, x, bottom));
	}
	painter.drawLines(lines);
}

} // namespace deskew
