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

#include "ImageView.h.moc"
#include "OptionsWidget.h"
#include "Margins.h"
#include "Settings.h"
#include "ImageTransformation.h"
#include "Utils.h"
#include "imageproc/PolygonUtils.h"
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
#include <assert.h>

using namespace imageproc;

namespace page_layout
{

ImageView::ImageView(
	IntrusivePtr<Settings> const& settings, PageId const& page_id,
	QImage const& image, QImage const& downscaled_image,
	ImageTransformation const& xform,
	QRectF const& adapted_content_rect, OptionsWidget const& opt_widget)
:	ImageViewBase(image, downscaled_image, xform, Margins(5.0, 5.0, 5.0, 5.0)),
	m_ptrSettings(settings),
	m_pageId(page_id),
	m_origXform(xform),
	m_physXform(xform.origDpi()),
	m_origToMM(m_origXform.transformBack() * m_physXform.pixelsToMM()),
	m_mmToOrig(m_physXform.mmToPixels() * m_origXform.transform()),
	m_innerRect(adapted_content_rect),
	m_aggregateHardSizeMM(settings->getAggregateHardSizeMM()),
	m_committedAggregateHardSizeMM(m_aggregateHardSizeMM),
	m_alignment(opt_widget.alignment()),
	m_innerResizingMask(0),
	m_middleResizingMask(0),
	m_leftRightLinked(opt_widget.leftRightLinked()),
	m_topBottomLinked(opt_widget.topBottomLinked())
{
	m_defaultStatusTip = tr("Resize margins by dragging any of the solid lines.");
	ensureStatusTip(defaultStatusTip());
	
	setMouseTracking(true);
	
	recalcBoxesAndFit(opt_widget.marginsMM());
}

ImageView::~ImageView()
{
}

void
ImageView::marginsSetExternally(Margins const& margins_mm)
{
	AggregateSizeChanged const changed = commitHardMargins(margins_mm);
	
	recalcBoxesAndFit(margins_mm);
	
	invalidateThumbnails(changed);
}

void
ImageView::leftRightLinkToggled(bool const linked)
{
	m_leftRightLinked = linked;
	if (linked) {
		Margins margins_mm(calcHardMarginsMM());
		if (margins_mm.left() != margins_mm.right()) {
			double const new_margin = std::min(
				margins_mm.left(), margins_mm.right()
			);
			margins_mm.setLeft(new_margin);
			margins_mm.setRight(new_margin);
			
			AggregateSizeChanged const changed =
					commitHardMargins(margins_mm);
			
			recalcBoxesAndFit(margins_mm);
			emit marginsSetLocally(margins_mm);
			
			invalidateThumbnails(changed);
		}
	}
}

void
ImageView::topBottomLinkToggled(bool const linked)
{
	m_topBottomLinked = linked;
	if (linked) {
		Margins margins_mm(calcHardMarginsMM());
		if (margins_mm.top() != margins_mm.bottom()) {
			double const new_margin = std::min(
				margins_mm.top(), margins_mm.bottom()
			);
			margins_mm.setTop(new_margin);
			margins_mm.setBottom(new_margin);
			
			AggregateSizeChanged const changed =
					commitHardMargins(margins_mm);
			
			recalcBoxesAndFit(margins_mm);
			emit marginsSetLocally(margins_mm);
			
			invalidateThumbnails(changed);
		}
	}
}

void
ImageView::alignmentChanged(Alignment const& alignment)
{
	m_alignment = alignment;
	m_ptrSettings->setPageAlignment(m_pageId, alignment);
	recalcBoxesAndFit(calcHardMarginsMM());
	emit invalidateThumbnail(m_pageId);
}

void
ImageView::aggregateHardSizeChanged()
{
	m_aggregateHardSizeMM = m_ptrSettings->getAggregateHardSizeMM();
	m_committedAggregateHardSizeMM = m_aggregateHardSizeMM;
	recalcOuterRect();
	updatePresentationTransform(FIT);
}

void
ImageView::paintOverImage(QPainter& painter)
{
	// Pretend we are drawing in m_origXform coordinates.
	painter.setWorldTransform(
		m_origXform.transformBack() * imageToVirt().transform()
		* painter.worldTransform()
	);
	
	QPainterPath outer_outline;
	outer_outline.addPolygon(
		PolygonUtils::round(
			m_alignment.isNull() ? m_middleRect : m_outerRect
		)
	);
	
	QPainterPath content_outline;
	content_outline.addPolygon(PolygonUtils::round(m_innerRect));
	
	painter.setRenderHint(QPainter::Antialiasing, false);
	
	painter.setPen(Qt::NoPen);
	painter.setBrush(QColor(0xbb, 0x00, 0xff, 40));
	painter.drawPath(outer_outline.subtracted(content_outline));
	
	QPen pen(QColor(0xbe, 0x5b, 0xec));
	pen.setCosmetic(true);
	pen.setWidthF(2.0);
	painter.setPen(pen);
	painter.setBrush(Qt::NoBrush);
	painter.drawRect(m_middleRect);
	painter.drawRect(m_innerRect);
	
	if (!m_alignment.isNull()) {
		pen.setStyle(Qt::DashLine);
		painter.setPen(pen);
		painter.drawRect(m_outerRect);
	}
}

void
ImageView::wheelEvent(QWheelEvent* const event)
{
	handleZooming(event);
}

void
ImageView::mousePressEvent(QMouseEvent* const event)
{
	int const middle_mask = cursorLocationMask(event->pos(), m_middleRect);
	int const inner_mask = cursorLocationMask(event->pos(), m_innerRect);
	if (!(inner_mask | middle_mask) || isDraggingInProgress()) {
		handleImageDragging(event);
		return;
	}

	if (event->button() == Qt::LeftButton) {
		QTransform const orig_to_widget(
			m_origXform.transformBack() * imageToVirt().transform()
			* virtualToWidget()
		);
		QTransform const widget_to_orig(
			widgetToVirtual() * imageToVirt().transformBack()
			* m_origXform.transform()
		);
		m_beforeResizing.middleWidgetRect = orig_to_widget.mapRect(
			m_middleRect
		);
		m_beforeResizing.origToWidget = orig_to_widget;
		m_beforeResizing.widgetToOrig = widget_to_orig;
		m_beforeResizing.mousePos = event->pos();
		m_beforeResizing.focalPoint = getWidgetFocalPoint();
		
		// We make sure that only one of inner mask and middle mask
		// is non-zero.  Note that inner mask takes precedence, as
		// a middle edge may be unmovable if it's on the edge of the
		// widget and very close to an inner edge.
		if (inner_mask) {
			m_innerResizingMask = inner_mask;
			m_middleResizingMask = 0;
		} else {
			m_middleResizingMask = middle_mask;
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
			&& (m_innerResizingMask | m_middleResizingMask)) {
		m_innerResizingMask = 0;
		m_middleResizingMask = 0;
		
		AggregateSizeChanged const agg_size_changed(
			commitHardMargins(calcHardMarginsMM())
		);
		
		recalcOuterRect();
		
		if (QRectF(rect()).contains(m_beforeResizing.middleWidgetRect)) {
			updatePresentationTransform(FIT);
		} else {
			updatePresentationTransform(DONT_FIT);
		}
		
		invalidateThumbnails(agg_size_changed);
	}
}

void
ImageView::mouseMoveEvent(QMouseEvent* const event)
{
	if (isDraggingInProgress()) {
		handleImageDragging(event);
		return;
	}
	
	if (m_innerResizingMask | m_middleResizingMask) {
		QPoint const delta(event->pos() - m_beforeResizing.mousePos);
		if (m_innerResizingMask) {
			resizeInnerRect(delta);
		} else {
			resizeMiddleRect(delta);
		}
	} else {
		int const middle_mask = cursorLocationMask(
			event->pos(), m_middleRect
		);
		int const inner_mask = cursorLocationMask(
			event->pos(), m_innerRect
		);
		int const mask = inner_mask ? inner_mask : middle_mask;
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
	int const old_resizing_mask = m_innerResizingMask | m_middleResizingMask;
	m_innerResizingMask = 0;
	m_middleResizingMask = 0;
	ensureCursorShape(Qt::ArrowCursor);
	if (old_resizing_mask) {
		Margins const margins_mm(calcHardMarginsMM());
		
		AggregateSizeChanged const agg_size_changed(
			commitHardMargins(margins_mm)
		);
		
		recalcBoxesAndFit(margins_mm);
		emit marginsSetLocally(margins_mm);
		
		invalidateThumbnails(agg_size_changed);
	}
}

QString
ImageView::defaultStatusTip() const
{
	return m_defaultStatusTip;
}

void
ImageView::resizeInnerRect(QPoint const delta)
{
	int left_adjust = 0;
	int right_adjust = 0;
	int top_adjust = 0;
	int bottom_adjust = 0;
	
	if (m_innerResizingMask & LEFT_EDGE) {
		left_adjust = delta.x();
		if (m_leftRightLinked) {
			right_adjust = -left_adjust;
		}
	} else if (m_innerResizingMask & RIGHT_EDGE) {
		right_adjust = delta.x();
		if (m_leftRightLinked) {
			left_adjust = -right_adjust;
		}
	}
	if (m_innerResizingMask & TOP_EDGE) {
		top_adjust = delta.y();
		if (m_topBottomLinked) {
			bottom_adjust = -top_adjust;
		}
	} else if (m_innerResizingMask & BOTTOM_EDGE) {
		bottom_adjust = delta.y();
		if (m_topBottomLinked) {
			top_adjust = -bottom_adjust;
		}
	}
	
	QRectF widget_rect(m_beforeResizing.middleWidgetRect);
	widget_rect.adjust(-left_adjust, -top_adjust, -right_adjust, -bottom_adjust);
	
	m_middleRect = m_beforeResizing.widgetToOrig.mapRect(widget_rect);
	forceNonNegativeHardMargins(m_middleRect);
	widget_rect = m_beforeResizing.origToWidget.mapRect(m_middleRect);
	
	double effective_dx = 0;
	double effective_dy = 0;
	
	QRectF const& old_widget_rect = m_beforeResizing.middleWidgetRect;
	if (m_innerResizingMask & LEFT_EDGE) {
		effective_dx = old_widget_rect.left() - widget_rect.left();
	} else if (m_innerResizingMask & RIGHT_EDGE) {
		effective_dx = old_widget_rect.right() - widget_rect.right();
	}
	if (m_innerResizingMask & TOP_EDGE) {
		effective_dy = old_widget_rect.top() - widget_rect.top();
	} else if (m_innerResizingMask & BOTTOM_EDGE) {
		effective_dy = old_widget_rect.bottom()- widget_rect.bottom();
	}
	
	// Updating the focal point is what makes the image move
	// as we drag an inner edge.
	QPointF fp(m_beforeResizing.focalPoint);
	fp += QPointF(effective_dx, effective_dy);
	setWidgetFocalPoint(fp);
	
	m_aggregateHardSizeMM = m_ptrSettings->getAggregateHardSizeMM(
		m_pageId, origRectToSizeMM(m_middleRect), m_alignment
	);
	
	recalcOuterRect();
	
	updatePresentationTransform(DONT_FIT);
	
	emit marginsSetLocally(calcHardMarginsMM());
}

void
ImageView::resizeMiddleRect(QPoint const delta)
{
	double left_adjust = 0.0;
	double right_adjust = 0.0;
	double top_adjust = 0.0;
	double bottom_adjust = 0.0;
	
	QRectF const bounds(marginsRect());
	QRectF const old_middle_rect(m_beforeResizing.middleWidgetRect);
	
	if (m_middleResizingMask & LEFT_EDGE) {
		left_adjust = delta.x();
		if (old_middle_rect.left() + left_adjust < bounds.left()) {
			left_adjust = bounds.left() - old_middle_rect.left();
		}
		if (m_leftRightLinked) {
			right_adjust = -left_adjust;
		}
	} else if (m_middleResizingMask & RIGHT_EDGE) {
		right_adjust = delta.x();
		if (old_middle_rect.right() + right_adjust > bounds.right()) {
			right_adjust = bounds.right() - old_middle_rect.right();
		}
		if (m_leftRightLinked) {
			left_adjust = -right_adjust;
		}
	}
	if (m_middleResizingMask & TOP_EDGE) {
		top_adjust = delta.y();
		if (old_middle_rect.top() + top_adjust < bounds.top()) {
			top_adjust = bounds.top() - old_middle_rect.top();
		}
		if (m_topBottomLinked) {
			bottom_adjust = -top_adjust;
		}
	} else if (m_middleResizingMask & BOTTOM_EDGE) {
		bottom_adjust = delta.y();
		if (old_middle_rect.bottom() + bottom_adjust > bounds.bottom()) {
			bottom_adjust = bounds.bottom() - old_middle_rect.bottom();
		}
		if (m_topBottomLinked) {
			top_adjust = -bottom_adjust;
		}
	}
	
	{
		QRectF widget_rect(old_middle_rect);
		widget_rect.adjust(left_adjust, top_adjust, right_adjust, bottom_adjust);
		
		m_middleRect = m_beforeResizing.widgetToOrig.mapRect(widget_rect);
		forceNonNegativeHardMargins(m_middleRect); // invalidates widget_rect
	}
	
	m_aggregateHardSizeMM = m_ptrSettings->getAggregateHardSizeMM(
		m_pageId, origRectToSizeMM(m_middleRect), m_alignment
	);
	
	recalcOuterRect();
	
	updatePresentationTransform(DONT_FIT);
	
	emit marginsSetLocally(calcHardMarginsMM());
}

/**
 * Updates m_middleRect and m_outerRect based on \p margins_mm,
 * m_aggregateHardSizeMM and m_alignment, updates the presentation transform.
 */
void
ImageView::recalcBoxesAndFit(Margins const& margins_mm)
{
	QPolygonF poly_mm(m_origToMM.map(m_innerRect));
	Utils::extendPolyRectWithMargins(poly_mm, margins_mm);

	QRectF const middle_rect(m_mmToOrig.map(poly_mm).boundingRect());
	
	QSizeF const hard_size_mm(
		QLineF(poly_mm[0], poly_mm[1]).length(),
		QLineF(poly_mm[0], poly_mm[3]).length()
	);
	Margins const soft_margins_mm(
		Utils::calcSoftMarginsMM(
			hard_size_mm, m_aggregateHardSizeMM, m_alignment
		)
	);
	
	Utils::extendPolyRectWithMargins(poly_mm, soft_margins_mm);

	QRectF const outer_rect(m_mmToOrig.map(poly_mm).boundingRect());
	
	ImageTransformation const new_xform(
		Utils::calcPresentationTransform(
			m_origXform, m_physXform.mmToPixels().map(poly_mm)
		)
	);
	
	updateTransformAndFixFocalPoint(new_xform, CENTER_IF_FITS);
	m_middleRect = middle_rect;
	m_outerRect = outer_rect;
}

/**
 * Updates the presentation transform in such a way that ImageViewBase
 * sees the image extended / clipped by m_outerRect.
 * Additionally, if \p fit_mode is set to FIT, ensures that the whole area
 * defined by m_outerRect is visible.
 */
void
ImageView::updatePresentationTransform(FitMode const fit_mode)
{
	QPolygonF const poly_phys(
		m_origXform.transformBack().map(m_outerRect)
	);
	
	ImageTransformation const new_xform(
		Utils::calcPresentationTransform(m_origXform, poly_phys)
	);
	
	if (fit_mode == DONT_FIT) {
		updateTransformPreservingScale(new_xform);
	} else {
		resetZoom();
		updateTransformAndFixFocalPoint(new_xform, CENTER_IF_FITS);
	}
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
		m_origXform.transformBack() * imageToVirt().transform()
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
ImageView::forceNonNegativeHardMargins(QRectF& middle_rect) const
{
	if (middle_rect.left() > m_innerRect.left()) {
		middle_rect.setLeft(m_innerRect.left());
	}
	if (middle_rect.right() < m_innerRect.right()) {
		middle_rect.setRight(m_innerRect.right());
	}
	if (middle_rect.top() > m_innerRect.top()) {
		middle_rect.setTop(m_innerRect.top());
	}
	if (middle_rect.bottom() < m_innerRect.bottom()) {
		middle_rect.setBottom(m_innerRect.bottom());
	}
}

/**
 * \brief Calculates margins in millimeters between m_innerRect and m_middleRect.
 */
Margins
ImageView::calcHardMarginsMM() const
{
	QPointF const center(m_innerRect.center());
	
	QLineF const top_margin_line(
		QPointF(center.x(), m_middleRect.top()),
		QPointF(center.x(), m_innerRect.top())
	);
	
	QLineF const bottom_margin_line(
		QPointF(center.x(), m_innerRect.bottom()),
		QPointF(center.x(), m_middleRect.bottom())
	);
	
	QLineF const left_margin_line(
		QPointF(m_middleRect.left(), center.y()),
		QPointF(m_innerRect.left(), center.y())
	);
	
	QLineF const right_margin_line(
		QPointF(m_innerRect.right(), center.y()),
		QPointF(m_middleRect.right(), center.y())
	);
	
	Margins margins;
	margins.setTop(m_origToMM.map(top_margin_line).length());
	margins.setBottom(m_origToMM.map(bottom_margin_line).length());
	margins.setLeft(m_origToMM.map(left_margin_line).length());
	margins.setRight(m_origToMM.map(right_margin_line).length());
	
	return margins;
}

/**
 * \brief Recalculates m_outerRect based on m_middleRect, m_aggregateHardSizeMM
 *        and m_alignment.
 */
void
ImageView::recalcOuterRect()
{
	QPolygonF poly_mm(m_origToMM.map(m_middleRect));
	
	QSizeF const hard_size_mm(
		QLineF(poly_mm[0], poly_mm[1]).length(),
		QLineF(poly_mm[0], poly_mm[3]).length()
	);
	Margins const soft_margins_mm(
		Utils::calcSoftMarginsMM(
			hard_size_mm, m_aggregateHardSizeMM, m_alignment
		)
	);
	
	Utils::extendPolyRectWithMargins(poly_mm, soft_margins_mm);
	
	m_outerRect = m_mmToOrig.map(poly_mm).boundingRect();
}

QSizeF
ImageView::origRectToSizeMM(QRectF const& rect) const
{
	QLineF const hor_line(rect.topLeft(), rect.topRight());
	QLineF const vert_line(rect.topLeft(), rect.bottomLeft());
	
	QSizeF const size_mm(
		m_origToMM.map(hor_line).length(),
		m_origToMM.map(vert_line).length()
	);
	
	return size_mm;
}

ImageView::AggregateSizeChanged
ImageView::commitHardMargins(Margins const& margins_mm)
{
	m_ptrSettings->setHardMarginsMM(m_pageId, margins_mm);
	m_aggregateHardSizeMM = m_ptrSettings->getAggregateHardSizeMM();
	
	AggregateSizeChanged changed = AGGREGATE_SIZE_UNCHANGED;
	if (m_committedAggregateHardSizeMM != m_aggregateHardSizeMM) {
		 changed = AGGREGATE_SIZE_CHANGED;
	}
	
	m_committedAggregateHardSizeMM = m_aggregateHardSizeMM;
	
	return changed;
}

void
ImageView::invalidateThumbnails(AggregateSizeChanged const agg_size_changed)
{
	if (agg_size_changed == AGGREGATE_SIZE_CHANGED) {
		emit invalidateAllThumbnails();
	} else {
		emit invalidateThumbnail(m_pageId);
	}
}

} // namespace page_layout
