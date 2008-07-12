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
#include "OptionsWidget.h"
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
#include <algorithm>

using namespace imageproc;

namespace page_layout
{

ImageView::ImageView(
	QImage const& image, ImageTransformation const& xform,
	QRectF const& content_rect, QSizeF const& aggregate_content_size_mm, // TODO: remove me
	OptionsWidget const& opt_widget)
:	ImageViewBase(image, xform, Margins(5.0, 5.0, 5.0, 5.0)),
	m_origXform(xform),
	m_contentRect(content_rect),
	m_innerResizingMask(0),
	m_outerResizingMask(0),
	m_leftRightLinked(opt_widget.leftRightLinked()),
	m_topBottomLinked(opt_widget.topBottomLinked())
{
	setMouseTracking(true);
	
	calcAndFitMarginBox(opt_widget.marginsMM(), CENTER_IF_FITS);
}

ImageView::~ImageView()
{
}

void
ImageView::leftRightLinkToggled(bool const linked)
{
	m_leftRightLinked = linked;
	if (linked) {
		// TODO: if resizing is in progress, cancel it.
		Margins margins(calculateMarginsMM());
		if (margins.left() != margins.right()) {
			double const new_margin = std::min(
				margins.left(), margins.right()
			);
			margins.setLeft(new_margin);
			margins.setRight(new_margin);
			calcAndFitMarginBox(margins, CENTER_IF_FITS);
			emit marginsSetManually(margins);
		}
	}
}

void
ImageView::topBottomLinkToggled(bool const linked)
{
	m_topBottomLinked = linked;
	if (linked) {
		// TODO: if resizing is in progress, cancel it.
		Margins margins(calculateMarginsMM());
		if (margins.top() != margins.bottom()) {
			double const new_margin = std::min(
				margins.top(), margins.bottom()
			);
			margins.setTop(new_margin);
			margins.setBottom(new_margin);
			calcAndFitMarginBox(margins, CENTER_IF_FITS);
			emit marginsSetManually(margins);
		}
	}
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
	painter.drawRect(m_contentRect);
}

void
ImageView::wheelEvent(QWheelEvent* const event)
{
	handleZooming(event);
}

void
ImageView::mousePressEvent(QMouseEvent* const event)
{
	int const outer_mask = cursorLocationMask(event->pos(), m_contentPlusMargins);
	int const inner_mask = cursorLocationMask(event->pos(), m_contentRect);
	if (!(inner_mask | outer_mask) || isDraggingInProgress()) {
		handleImageDragging(event);
		return;
	}

	if (event->button() == Qt::LeftButton) {
		QTransform const orig_to_widget(
			m_origXform.transformBack() * physToVirt().transform()
			* virtualToWidget()
		);
		QTransform const widget_to_orig(
			widgetToVirtual() * physToVirt().transformBack()
			* m_origXform.transform()
		);
		m_beforeResizing.outerWidgetRect = orig_to_widget.mapRect(
			m_contentPlusMargins
		);
		m_beforeResizing.widgetToOrig = widget_to_orig;
		m_beforeResizing.mousePos = event->pos();
		m_beforeResizing.focalPoint = getFocalPoint();
		
		// We make sure that only one of inner mask and outer mask
		// is non-zero.  Note that inner mask takes precedence, as
		// an outer edge may be unmovable if it's on the edge of the
		// widget and very close to an inner edge.
		if (inner_mask) {
			m_innerResizingMask = inner_mask;
			m_outerResizingMask = 0;
		} else {
			m_outerResizingMask = outer_mask;
			m_innerResizingMask = 0;
		}
	}
}

void
ImageView::mouseReleaseEvent(QMouseEvent* const event)
{
	if (isDraggingInProgress()) {
		handleImageDragging(event);
		return;
	}
	
	if (event->button() == Qt::LeftButton
			&& (m_innerResizingMask | m_outerResizingMask)) {
		m_innerResizingMask = 0;
		m_outerResizingMask = 0;
		
		if (QRectF(rect()).contains(m_beforeResizing.outerWidgetRect)) {
			resetZoom();
			fitMarginBox(CENTER_IF_FITS);
		}
		
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
	
	if (m_innerResizingMask | m_outerResizingMask) {
		QPoint const delta(event->pos() - m_beforeResizing.mousePos);
		if (m_innerResizingMask) {
			resizeInnerRect(delta);
		} else {
			resizeOuterRect(delta);
		}
	} else {
		int const outer_mask = cursorLocationMask(event->pos(), m_contentPlusMargins);
		int const inner_mask = cursorLocationMask(event->pos(), m_contentRect);
		int const mask = inner_mask ? inner_mask : outer_mask;
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
	}
}

void
ImageView::hideEvent(QHideEvent* const event)
{
	ImageViewBase::hideEvent(event);
	int const old_resizing_mask = m_innerResizingMask | m_outerResizingMask;
	m_innerResizingMask = 0;
	m_outerResizingMask = 0;
	ensureCursorShape(Qt::ArrowCursor);
	if (old_resizing_mask) {
		emit marginsSetManually(calculateMarginsMM());
	}
}

void
ImageView::resizeInnerRect(QPoint const delta)
{
	int left_adjust = 0;
	int right_adjust = 0;
	int top_adjust = 0;
	int bottom_adjust = 0;
	int effective_dx = 0;
	int effective_dy = 0;
	
	if (m_innerResizingMask & LEFT_EDGE) {
		left_adjust = effective_dx = delta.x();
		if (m_leftRightLinked) {
			right_adjust = -left_adjust;
		}
	} else if (m_innerResizingMask & RIGHT_EDGE) {
		right_adjust = effective_dx = delta.x();
		if (m_leftRightLinked) {
			left_adjust = -right_adjust;
		}
	}
	if (m_innerResizingMask & TOP_EDGE) {
		top_adjust = effective_dy = delta.y();
		if (m_topBottomLinked) {
			bottom_adjust = -top_adjust;
		}
	} else if (m_innerResizingMask & BOTTOM_EDGE) {
		bottom_adjust = effective_dy = delta.y();
		if (m_topBottomLinked) {
			top_adjust = -bottom_adjust;
		}
	}
	
	{
		QRectF widget_rect(m_beforeResizing.outerWidgetRect);
		widget_rect.adjust(-left_adjust, -top_adjust, -right_adjust, -bottom_adjust);
		
		m_contentPlusMargins = m_beforeResizing.widgetToOrig.mapRect(widget_rect);
		forceNonNegativeMargins(m_contentPlusMargins); // invalidates widget_rect
	}
	
	// Updating the focal point is what makes the image move
	// as we drag an inner edge.
	QPointF fp(m_beforeResizing.focalPoint);
	fp = physToVirt().transform().map(fp);
	fp = virtualToWidget().map(fp);
	fp -= QPointF(effective_dx, effective_dy);
	fp = widgetToVirtual().map(fp);
	fp = physToVirt().transformBack().map(fp);
	setFocalPoint(fp);
	
	// This clips image by the outer rect.
	updatePresentationTransform();
	
	update();
	emit marginsSetManually(calculateMarginsMM());
}

void
ImageView::resizeOuterRect(QPoint const delta)
{
	double left_adjust = 0.0;
	double right_adjust = 0.0;
	double top_adjust = 0.0;
	double bottom_adjust = 0.0;
	
	QRectF const bounds(marginsRect());
	QRectF const old_outer_rect(m_beforeResizing.outerWidgetRect);
	
	if (m_outerResizingMask & LEFT_EDGE) {
		left_adjust = delta.x();
		if (old_outer_rect.left() + left_adjust < bounds.left()) {
			left_adjust = bounds.left() - old_outer_rect.left();
		}
		if (m_leftRightLinked) {
			right_adjust = -left_adjust;
		}
	} else if (m_outerResizingMask & RIGHT_EDGE) {
		right_adjust = delta.x();
		if (old_outer_rect.right() + right_adjust > bounds.right()) {
			right_adjust = bounds.right() - old_outer_rect.right();
		}
		if (m_leftRightLinked) {
			left_adjust = -right_adjust;
		}
	}
	if (m_outerResizingMask & TOP_EDGE) {
		top_adjust = delta.y();
		if (old_outer_rect.top() + top_adjust < bounds.top()) {
			top_adjust = bounds.top() - old_outer_rect.top();
		}
		if (m_topBottomLinked) {
			bottom_adjust = -top_adjust;
		}
	} else if (m_outerResizingMask & BOTTOM_EDGE) {
		bottom_adjust = delta.y();
		if (old_outer_rect.bottom() + bottom_adjust > bounds.bottom()) {
			bottom_adjust = bounds.bottom() - old_outer_rect.bottom();
		}
		if (m_topBottomLinked) {
			top_adjust = -bottom_adjust;
		}
	}
	
	{
		QRectF widget_rect(old_outer_rect);
		widget_rect.adjust(left_adjust, top_adjust, right_adjust, bottom_adjust);
		
		m_contentPlusMargins = m_beforeResizing.widgetToOrig.mapRect(widget_rect);
		forceNonNegativeMargins(m_contentPlusMargins); // invalidates widget_rect
	}
	
	// This clips image by the outer rect.
	updatePresentationTransform();
	
	update();
	emit marginsSetManually(calculateMarginsMM());
}

/**
 * Fits the image with margins into the widget by updating the presentation
 * transform and adjusting the focal point.
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

/**
 * Updates m_contentPlusMargins based on \p margins_mm, then does what
 * fitMarginBox() does.
 */
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

/**
 * Updates the presentation transform in such a way that ImageViewBase
 * sees the image extended / clipped by m_contentPlusMargins.
 */
void
ImageView::updatePresentationTransform()
{
	QPolygonF const poly_phys(
		m_origXform.transformBack().map(m_contentPlusMargins)
	);
	
	ImageTransformation new_xform(m_origXform);
	new_xform.setCropArea(QPolygonF()); // Reset the crop area and deskew angle.
	new_xform.setCropArea(new_xform.transform().map(poly_phys));
	new_xform.setPostRotation(m_origXform.postRotation());
	
	updateTransformPreservingScale(new_xform);
}

/**
 * \brief Checks if mouse pointer is near the edges of a rectangle.
 *
 * \param cursor_pos The mouse pointer position in widget coordinates.
 * \param orig_rect The rectangle for edge proximity testing, in m_origXform
 *        coordinates.
 * \return A bitwise OR of *_EDGE values, corresponding to the proximity of
 *         the \p cursor_pos to a particular edge of \p orig_rect.
 */
int
ImageView::cursorLocationMask(QPoint const& cursor_pos, QRectF const& orig_rect) const
{
	QTransform const orig_to_widget(
		m_origXform.transformBack() * physToVirt().transform()
		* virtualToWidget()
	);
	QRect const rect(orig_to_widget.mapRect(orig_rect).toRect());
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

/**
 * \brief Calculates margins in millimeters based on m_contentRect
 *        and m_contentPlusMargins.
 */
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
