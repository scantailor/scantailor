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

#include "IncompleteThumbnail.h"
#include <QRectF>
#include <QSizeF>
#include <QTransform>
#include <QPainter>
#include <QString>
#include <QFont>
#include <QPen>
#include <QColor>
#include <QDebug>

QPainterPath IncompleteThumbnail::m_sCachedPath;

IncompleteThumbnail::IncompleteThumbnail(
	ThumbnailPixmapCache& thumbnail_cache,
	ImageId const& image_id, ImageTransformation const& image_xform)
:	ThumbnailBase(thumbnail_cache, image_id, image_xform)
{
}

IncompleteThumbnail::~IncompleteThumbnail()
{
}

void
IncompleteThumbnail::paintOverImage(
	QPainter& painter, QTransform const& image_to_display,
	QTransform const& thumb_to_display)
{
	QString const text(QString::fromAscii("?"));
	QRectF const bounding_rect(boundingRect());
	
	// Because painting happens only from the main thread, we don't
	// need to care about concurrent access.
	if (m_sCachedPath.isEmpty()) {
		QFont font(painter.font());
		font.setWeight(QFont::DemiBold);
		font.setStyleStrategy(QFont::ForceOutline);
		m_sCachedPath.addText(0, 0, font, text);
	}
	QRectF const text_rect(m_sCachedPath.boundingRect());
	
	QTransform xform1;
	xform1.translate(-text_rect.left(), -text_rect.top());
	
	QSizeF const unscaled_size(text_rect.size());
	QSizeF scaled_size(unscaled_size);
	scaled_size.scale(bounding_rect.size() * 0.9, Qt::KeepAspectRatio);
	
	double const hscale = scaled_size.width() / unscaled_size.width();
	double const vscale = scaled_size.height() / unscaled_size.height();
	QTransform xform2;
	xform2.scale(hscale, vscale);
	
	// Position the text at the center of our bounding rect.
	QSizeF const translation(bounding_rect.size() * 0.5 - scaled_size * 0.5);
	QTransform xform3;
	xform3.translate(translation.width(), translation.height());
	
	painter.setWorldTransform(xform1 * xform2 * xform3 * thumb_to_display);
	painter.setRenderHint(QPainter::Antialiasing);
	
	QPen pen(QColor(0x00, 0x00, 0x00, 60));
	pen.setWidth(2);
	pen.setCosmetic(true);
	painter.setPen(pen);
	
	painter.drawPath(m_sCachedPath);
}
