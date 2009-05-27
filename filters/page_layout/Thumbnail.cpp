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

#include "Thumbnail.h"
#include "Utils.h"
#include "imageproc/PolygonUtils.h"
#include <QRectF>
#include <QSizeF>
#include <QLineF>
#include <QPolygonF>
#include <QTransform>
#include <QPainter>
#include <QPen>
#include <QBrush>
#include <QColor>
#include <QApplication>
#include <QPalette>
#include <QDebug>

using namespace imageproc;

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
	setExtendedClipArea(true);
}

void
Thumbnail::paintOverImage(
	QPainter& painter, QTransform const& image_to_display,
	QTransform const& thumb_to_display)
{
	QTransform const orig_to_presentation(
		m_origXform.transformBack() * imageXform().transform()
	);
	QTransform const orig_to_thumb(
		orig_to_presentation * image_to_display * thumb_to_display.inverted()
	);
	
	// We work in thumbnail coordinates because we want to adjust
	// rectangle coordinates by exactly their display width.
	painter.setWorldTransform(thumb_to_display);
	
	QRectF const inner_rect(orig_to_thumb.mapRect(m_adaptedContentRect));
	
	// We extend the outer rectangle because otherwise we may get white
	// thin lines near the edges due to rounding errors and the lack
	// of subpixel accuracy.  Doing that is actually OK, because what
	// we paint will be clipped anyway.
	QRectF const outer_rect(
		orig_to_thumb.mapRect(m_outerRect).adjusted(-1.0, -1.0, 1.0, 1.0)
	);
	
	QPainterPath outer_outline;
	outer_outline.addPolygon(PolygonUtils::round(outer_rect));
	
	QPainterPath content_outline;
	content_outline.addPolygon(PolygonUtils::round(inner_rect));
	
	QPainterPath page_outline;
	page_outline.addPolygon(
		PolygonUtils::round(
			orig_to_thumb.map(m_origXform.resultingCropArea())
		)
	);
	
	painter.setRenderHint(QPainter::Antialiasing, false);
	
	// Clear parts of the thumbnail that don't belong to the image
	// but belong to outer_rect.
	painter.fillPath(outer_outline.subtracted(page_outline), QApplication::palette().window());
	
	// Draw margins.
	painter.fillPath(outer_outline.subtracted(content_outline), QColor(0xbb, 0x00, 0xff, 40));
	
	QPen pen(QColor(0xbe, 0x5b, 0xec));
	pen.setCosmetic(true);
	pen.setWidthF(1.0);
	painter.setPen(pen);
	painter.setBrush(Qt::NoBrush);
	
	// toRect() is necessary because we turn off antialiasing.
	// For some reason, if we let Qt round the coordinates,
	// the result is slightly different.
	painter.drawRect(inner_rect.toRect());
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
	
	m_outerRect = m_mmToOrig.map(poly_mm).boundingRect();
	
	ImageTransformation const presentation_xform(
		Utils::calcPresentationTransform(
			m_origXform, m_physXform.mmToPixels().map(poly_mm)
		)
	);
	
	setImageXform(presentation_xform);
}

} // namespace page_layout
