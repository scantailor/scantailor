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
#include "Margins.h"
#include "ImageTransformation.h"
#include "PhysicalTransformation.h"
#include "imageproc/PolygonUtils.h"
#include <QTransform>
#include <QPointF>
#include <QLineF>
#include <QPolygonF>
#include <QRect>
#include <QSize>
#include <QString>
#include <QPainter>
#include <QBrush>
#include <QPen>
#include <QColor>
#include <QDebug>
#include <QMouseEvent>
#include <Qt>

using namespace imageproc;

namespace page_layout
{

ImageView::ImageView(
	QImage const& image, ImageTransformation const& xform,
	QRectF const& content_rect, Margins const& margins_mm,
	QSizeF const& aggregate_content_size_mm)
:	ImageViewBase(image, xform, Margins(5.0, 5.0, 5.0, 5.0)),
	m_origXform(xform),
	m_contentRect(content_rect),
	m_aggregateContentSizeMM(aggregate_content_size_mm),
	m_resizingMask(0)
{
	setMouseTracking(true);
	
	calcAndFitMarginBox(margins_mm, CENTER_IF_FITS);
}

ImageView::~ImageView()
{
}

void
ImageView::paintOverImage(QPainter& painter)
{
	// Pretend we are drawing in m_origXform coordinates.
	painter.setWorldTransform(
		m_origXform.transformBack() * physToVirt().transform()
		* painter.worldTransform()
	);
	
	QPainterPath margins;
	margins.addPolygon(PolygonUtils::round(m_contentPlusMargins));
	
	QPainterPath content;
	content.addPolygon(PolygonUtils::round(m_contentRect));
	
	painter.setRenderHint(QPainter::Antialiasing, false);
	
	painter.setPen(Qt::NoPen);
	painter.setBrush(QColor(0xbb, 0x00, 0xff, 40));
	painter.drawPath(margins.subtracted(content));
	
	QPen pen(QColor(0xbe, 0x5b, 0xec));
	pen.setCosmetic(true);
	pen.setWidthF(2.0);
	painter.setPen(pen);
	painter.setBrush(Qt::NoBrush);
	painter.drawRect(m_contentPlusMargins);
}

void
ImageView::wheelEvent(QWheelEvent* const event)
{
	handleZooming(event);
}

void
ImageView::mousePressEvent(QMouseEvent* const event)
{
	int const mask = cursorLocationMask(event->pos());
	if (!mask || isDraggingInProgress()) {
		handleImageDragging(event);
		return;
	}

	if (event->button() == Qt::LeftButton) {
		QTransform const orig_to_widget(
			m_origXform.transformBack() * physToVirt().transform() * virtualToWidget()
		);
		QTransform const widget_to_orig(
			widgetToVirtual() * physToVirt().transformBack() * m_origXform.transform()
		);
		m_widgetToOrigBeforeResizing = widget_to_orig;
		m_widgetRectBeforeResizing = orig_to_widget.mapRect(m_contentPlusMargins);
		m_cursorPosBeforeResizing = event->pos();
		m_resizingMask = mask;
	}
}

void
ImageView::mouseReleaseEvent(QMouseEvent* const event)
{
	if (isDraggingInProgress()) {
		handleImageDragging(event);
		return;
	}
	
	if (event->button() == Qt::LeftButton && m_resizingMask != 0) {
		m_resizingMask = 0;
		fitMarginBox(CENTER_IF_FITS);
	//	emit marginsSetManually(calculateMarginsMM());
	}
}

void
ImageView::mouseMoveEvent(QMouseEvent* const event)
{
	if (isDraggingInProgress()) {
		handleImageDragging(event);
		return;
	}
	
	if (m_resizingMask == 0) { // not resizing
		int const mask = cursorLocationMask(event->pos());
		int const ver = mask & (TOP_EDGE|BOTTOM_EDGE);
		int const hor = mask & (LEFT_EDGE|RIGHT_EDGE);
		if (!ver && !hor) {
			ensureCursorShape(Qt::ArrowCursor);
		} else if (ver && !hor) {
			ensureCursorShape(Qt::SizeVerCursor);
		} else if (hor && !ver) {
			ensureCursorShape(Qt::SizeHorCursor);
		} else {
			int const top_left = LEFT_EDGE|TOP_EDGE;
			int const bottom_right = RIGHT_EDGE|BOTTOM_EDGE;
			if ((mask & top_left) == top_left ||
			    (mask & bottom_right) == bottom_right) {
				ensureCursorShape(Qt::SizeFDiagCursor);
			} else {
				ensureCursorShape(Qt::SizeBDiagCursor);
			}
		}
	} else {
		QPoint const delta(event->pos() - m_cursorPosBeforeResizing);
		QRectF widget_rect(m_widgetRectBeforeResizing);
		int left_adjust = 0;
		int right_adjust = 0;
		int top_adjust = 0;
		int bottom_adjust = 0;
		
		if (m_resizingMask & LEFT_EDGE) {
			left_adjust = delta.x();
		}
		if (m_resizingMask & RIGHT_EDGE) {
			right_adjust = delta.x();
		}
		if (m_resizingMask & TOP_EDGE) {
			top_adjust = delta.y();
		}
		if (m_resizingMask & BOTTOM_EDGE) {
			bottom_adjust = delta.y();
		}
		widget_rect.adjust(
			left_adjust, top_adjust, right_adjust, bottom_adjust
		);
		
		QRectF virt_rect(m_widgetToOrigBeforeResizing.mapRect(widget_rect));
		
		forceNonNegativeMargins(virt_rect);
		
		// Recalculate widget_rect, as virt_rect may have changed.
		widget_rect = m_widgetToOrigBeforeResizing.inverted().mapRect(virt_rect);
		
		if (virt_rect != m_contentPlusMargins) {
			m_contentPlusMargins = virt_rect;
			
			QRectF widget_rect_before(m_widgetRectBeforeResizing);
			// Extend it a bit, to prevent rounding errors.
			widget_rect_before.adjust(-0.1, -0.1, 0.1, 0.1);
			
			if (!widget_rect_before.contains(widget_rect)) {
				fitMarginBox(DONT_CENTER);
			}
			
			update();
			emit marginsSetManually(calculateMarginsMM());
		}
	}
}

void
ImageView::hideEvent(QHideEvent* const event)
{
	// TODO
}

/**
 * Fits the image with margins into the widget by feeding ImageViewBase
 * an adjusted transformation that makes it think the visible image area
 * is bigger than it really is.
 */
void
ImageView::fitMarginBox(FocalPointMode const fp_mode)
{
	QPolygonF const poly_phys(
		m_origXform.transformBack().map(m_contentPlusMargins)
	);
	
	ImageTransformation new_xform(m_origXform);
	new_xform.setCropArea(QPolygonF()); // Reset the crop area and deskew angle.
	new_xform.setCropArea(new_xform.transform().map(poly_phys));
	new_xform.setPostRotation(m_origXform.postRotation());
	
	updateTransformAndFixFocalPoint(new_xform, fp_mode);
}

void
ImageView::calcAndFitMarginBox(
	Margins const& margins_mm, FocalPointMode const fp_mode)
{
	PhysicalTransformation const phys_xform(m_origXform.origDpi());
	QTransform const orig_to_mm(
		m_origXform.transformBack() * phys_xform.pixelsToMM()
	);
	QTransform const mm_to_orig(
		phys_xform.mmToPixels() * m_origXform.transform()
	);
	
	QPolygonF poly_mm(orig_to_mm.map(m_contentRect));
	
	QLineF const down_uv_line(QLineF(poly_mm[0], poly_mm[3]).unitVector());
	QLineF const right_uv_line(QLineF(poly_mm[0], poly_mm[1]).unitVector());
	
	QPointF const down_uv(down_uv_line.p2() - down_uv_line.p1());
	QPointF const right_uv(right_uv_line.p2() - right_uv_line.p1());
	
	// top-left
	poly_mm[0] -= down_uv * margins_mm.top();
	poly_mm[0] -= right_uv * margins_mm.left();
	
	// top-right
	poly_mm[1] -= down_uv * margins_mm.top();
	poly_mm[1] += right_uv * margins_mm.right();
	
	// bottom-right
	poly_mm[2] += down_uv * margins_mm.bottom();
	poly_mm[2] += right_uv * margins_mm.right();
	
	// bottom-left
	poly_mm[3] += down_uv * margins_mm.bottom();
	poly_mm[3] -= right_uv * margins_mm.left();
	
	poly_mm[4] = poly_mm[3];
	
	ImageTransformation new_xform(m_origXform);
	new_xform.setCropArea(QPolygonF()); // Reset the crop area and deskew angle.
	new_xform.setCropArea(
		(phys_xform.mmToPixels() * new_xform.transform()).map(poly_mm)
	);
	new_xform.setPostRotation(m_origXform.postRotation());
	
	m_contentPlusMargins = mm_to_orig.map(poly_mm).boundingRect();
	
	updateTransformAndFixFocalPoint(new_xform, fp_mode);
}

int
ImageView::cursorLocationMask(QPoint const& cursor_pos) const
{
	QTransform const orig_to_widget(
		m_origXform.transformBack() * physToVirt().transform()
		* virtualToWidget()
	);
	QRect const rect(orig_to_widget.mapRect(m_contentPlusMargins).toRect());
	int const adj = 5;
	int mask = 0;
	
	QRect top_edge_rect(rect.topLeft(), QSize(rect.width(), 1));
	top_edge_rect.adjust(-adj, -adj, adj, adj);
	if (top_edge_rect.contains(cursor_pos)) {
		mask |= TOP_EDGE;
	}
	
	QRect bottom_edge_rect(rect.bottomLeft(), QSize(rect.width(), 1));
	bottom_edge_rect.adjust(-adj, -adj, adj, adj);
	if (bottom_edge_rect.contains(cursor_pos)) {
		mask |= BOTTOM_EDGE;
	}
	
	QRect left_edge_rect(rect.topLeft(), QSize(1, rect.height()));
	left_edge_rect.adjust(-adj, -adj, adj, adj);
	if (left_edge_rect.contains(cursor_pos)) {
		mask |= LEFT_EDGE;
	}
	
	QRect right_edge_rect(rect.topRight(), QSize(1, rect.height()));
	right_edge_rect.adjust(-adj, -adj, adj, adj);
	if (right_edge_rect.contains(cursor_pos)) {
		mask |= RIGHT_EDGE;
	}
	
	return mask;
}

void
ImageView::forceNonNegativeMargins(QRectF& content_plus_margins) const
{
	if (content_plus_margins.left() > m_contentRect.left()) {
		content_plus_margins.setLeft(m_contentRect.left());
	}
	if (content_plus_margins.right() < m_contentRect.right()) {
		content_plus_margins.setRight(m_contentRect.right());
	}
	if (content_plus_margins.top() > m_contentRect.top()) {
		content_plus_margins.setTop(m_contentRect.top());
	}
	if (content_plus_margins.bottom() < m_contentRect.bottom()) {
		content_plus_margins.setBottom(m_contentRect.bottom());
	}
}

Margins
ImageView::calculateMarginsMM() const
{
	QPointF const center(m_contentRect.center());
	
	QLineF const top_margin_line(
		QPointF(center.x(), m_contentPlusMargins.top()),
		QPointF(center.x(), m_contentRect.top())
	);
	
	QLineF const bottom_margin_line(
		QPointF(center.x(), m_contentRect.bottom()),
		QPointF(center.x(), m_contentPlusMargins.bottom())
	);
	
	QLineF const left_margin_line(
		QPointF(m_contentPlusMargins.left(), center.y()),
		QPointF(m_contentRect.left(), center.y())
	);
	
	QLineF const right_margin_line(
		QPointF(m_contentRect.right(), center.y()),
		QPointF(m_contentPlusMargins.right(), center.y())
	);
	
	PhysicalTransformation const phys_xform(m_origXform.origDpi());
	QTransform const virt_to_mm(
		m_origXform.transformBack() * phys_xform.pixelsToMM()
	);
	
	Margins margins;
	margins.setTop(virt_to_mm.map(top_margin_line).length());
	margins.setBottom(virt_to_mm.map(bottom_margin_line).length());
	margins.setLeft(virt_to_mm.map(left_margin_line).length());
	margins.setRight(virt_to_mm.map(right_margin_line).length());
	
	return margins;
}

} // namespace page_layout
