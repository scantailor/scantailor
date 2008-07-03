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

#include "ImageView.h.moc"
#include "ImageTransformation.h"
#include "PhysicalTransformation.h"
#include <QTransform>
#include <QLineF>
#include <QPolygonF>
#include <QString>
#include <QPainter>
#include <QBrush>
#include <QPen>
#include <QColor>
#include <QDebug>
#include <Qt>

namespace page_layout
{

ImageView::ImageView(
	QImage const& image, ImageTransformation const& xform,
	QRectF const& content_rect, QSizeF const& margins_mm,
	QSizeF const& aggregate_content_size_mm)
:	ImageViewBase(image, xform),
	m_origXform(xform),
	m_origContentRect(content_rect),
	m_marginsMM(margins_mm),
	m_aggregateContentSizeMM(aggregate_content_size_mm)
{
	//setMouseTracking(true);
	
	updateLayout();
}

ImageView::~ImageView()
{
}

void
ImageView::paintOverImage(QPainter& painter)
{
	QPainterPath margins;
	margins.addRect(m_contentPlusMargins);
	
	QPainterPath content;
	content.addRect(m_contentRect);
	
	painter.setRenderHint(QPainter::Antialiasing, false);
	
	painter.setPen(Qt::NoPen);
	painter.setBrush(QColor(0xff, 0xff, 0xff));
	painter.drawPath(margins.subtracted(content));
}

void
ImageView::wheelEvent(QWheelEvent* const event)
{
	handleZooming(event);
}

void
ImageView::mousePressEvent(QMouseEvent* const event)
{
	handleImageDragging(event);
}

void
ImageView::mouseReleaseEvent(QMouseEvent* const event)
{
	handleImageDragging(event);
}

void
ImageView::mouseMoveEvent(QMouseEvent* const event)
{
	handleImageDragging(event);
}

void
ImageView::hideEvent(QHideEvent* const event)
{
	// TODO
}

void
ImageView::updateLayout()
{
	PhysicalTransformation const phys_xform(m_origXform.origDpi());
	QTransform const virt_to_mm(m_origXform.transformBack() * phys_xform.pixelsToMM());
	//QTransform const mm_to_virt(phys_xform.mmToPixels() * m_origXform.transform());
	
	QPolygonF poly_mm(virt_to_mm.map(m_origContentRect));
	
	QLineF const down_uv_line(QLineF(poly_mm[0], poly_mm[3]).unitVector());
	QLineF const right_uv_line(QLineF(poly_mm[0], poly_mm[1]).unitVector());
	
	QPointF const down_uv(down_uv_line.p2() - down_uv_line.p1());
	QPointF const right_uv(right_uv_line.p2() - right_uv_line.p1());
	
	// top-left
	poly_mm[0] -= down_uv * m_marginsMM.height();
	poly_mm[0] -= right_uv * m_marginsMM.width();
	
	// top-right
	poly_mm[1] -= down_uv * m_marginsMM.height();
	poly_mm[1] += right_uv * m_marginsMM.width();
	
	// bottom-right
	poly_mm[2] += down_uv * m_marginsMM.height();
	poly_mm[2] += right_uv * m_marginsMM.width();
	
	// bottom-left
	poly_mm[3] += down_uv * m_marginsMM.height();
	poly_mm[3] -= right_uv * m_marginsMM.width();
	
	poly_mm[4] = poly_mm[3];
	
	ImageTransformation new_xform(m_origXform);
	new_xform.setCropArea(QPolygonF()); // Reset the crop area and deskew angle.
	new_xform.setCropArea(
		(phys_xform.mmToPixels() * new_xform.transform()).map(poly_mm)
	);
	new_xform.setPostRotation(m_origXform.postRotation());
	
	updateTransformAndFixFocalPoint(new_xform);
	
	m_contentPlusMargins = new_xform.resultingRect();
	m_contentRect = (m_origXform.transformBack() * new_xform.transform()).mapRect(m_origContentRect);
}

} // namespace page_layout
