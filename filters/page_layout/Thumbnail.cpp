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
#include "Utils.h"
#include <QRectF>
#include <QSizeF>
#include <QLineF>
#include <QPolygonF>
#include <QTransform>
#include <QPainter>
#include <QPen>
#include <QBrush>
#include <QColor>
#include <QDebug>

namespace page_layout
{

Thumbnail::Thumbnail(
	ThumbnailPixmapCache& thumbnail_cache, QSizeF const& max_size,
	ImageId const& image_id, ImageTransformation const& xform,
	Params const& params, QRectF const& adapted_content_rect,
	QSizeF const& aggregate_hard_size_mm)
:	ThumbnailBase(thumbnail_cache, max_size, image_id, xform),
	m_params(params),
	m_adaptedContentRect(adapted_content_rect),
	m_aggregateHardSizeMM(aggregate_hard_size_mm),
	m_origXform(xform),
	m_physXform(xform.origDpi()),
	m_origToMM(m_origXform.transformBack() * m_physXform.pixelsToMM()),
	m_mmToOrig(m_physXform.mmToPixels() * m_origXform.transform())
{
	recalcBoxesAndPresentationTransform();
}

void
Thumbnail::paintOverImage(
	QPainter& painter, QTransform const& image_to_display,
	QTransform const& thumb_to_display)
{
#if 0
	QTransform const orig_to_presentation(
		m_origXform.transformBack() * imageXform().transform()
	);
	
	painter.setRenderHint(QPainter::Antialiasing, false);
	
	QPen pen(QColor(0x00, 0x00, 0xff));
	pen.setWidth(1);
	pen.setCosmetic(true);
	painter.setPen(pen);
	
	painter.setBrush(QColor(0x00, 0x00, 0xff, 50));
	
	QRectF content_rect(
		(orig_to_presentation * imageToThumb()).mapRect(m_adaptedContentRect)
	);
	
	// Adjust to compensate for pen width.
	content_rect.adjust(-1, -1, 1, 1);
	
	// toRect() is necessary because we turn off antialiasing.
	// For some reason, if we let Qt round the coordinates,
	// the result is slightly different.
	painter.drawRect(content_rect.toRect());
	}
#endif
}

void
Thumbnail::recalcBoxesAndPresentationTransform()
{
	QPolygonF poly_mm(m_origToMM.map(m_adaptedContentRect));
	Utils::extendPolyRectWithMargins(poly_mm, m_params.hardMarginsMM());
	
	//QRectF const middle_rect(m_mmToOrig.map(poly_mm).boundingRect());
	
	QSizeF const hard_size_mm(
		QLineF(poly_mm[0], poly_mm[1]).length(),
		QLineF(poly_mm[0], poly_mm[3]).length()
	);
	
	Margins const soft_margins_mm(
		Utils::calcSoftMarginsMM(
			hard_size_mm, m_aggregateHardSizeMM, m_params.alignment()
		)
	);
	
	Utils::extendPolyRectWithMargins(poly_mm, soft_margins_mm);
	
	//QRectF const outer_rect(m_mmToOrig.map(poly_mm).boundingRect());
	
	ImageTransformation const presentation_xform(
		Utils::calcPresentationTransform(
			m_origXform, m_physXform.mmToPixels().map(poly_mm)
		)
	);
	
	setImageXform(presentation_xform);
}

} // namespace page_layout
