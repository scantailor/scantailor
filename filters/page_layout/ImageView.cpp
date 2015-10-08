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

#include "ImageView.h"
#include "ImageView.h.moc"
#include "OptionsWidget.h"
#include "Margins.h"
#include "Settings.h"
#include "ImageTransformation.h"
#include "ImagePresentation.h"
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
#include <Qt>
#ifndef Q_MOC_RUN
#include <boost/bind.hpp>
#include <boost/lambda/lambda.hpp>
#endif
#include <algorithm>
#include <math.h>
#include <assert.h>

using namespace imageproc;

namespace page_layout
{

ImageView::ImageView(
	IntrusivePtr<Settings> const& settings, PageId const& page_id,
	QImage const& image, QImage const& downscaled_image,
	ImageTransformation const& xform,
	QRectF const& adapted_content_rect, OptionsWidget const& opt_widget)
:	ImageViewBase(
		image, downscaled_image,
		ImagePresentation(xform.transform(), xform.resultingPreCropArea()),
		Margins(5, 5, 5, 5)
	),
	m_dragHandler(*this),
	m_zoomHandler(*this),
	m_ptrSettings(settings),
	m_pageId(page_id),
	m_physXform(xform.origDpi()),
	m_innerRect(adapted_content_rect),
	m_aggregateHardSizeMM(settings->getAggregateHardSizeMM()),
	m_committedAggregateHardSizeMM(m_aggregateHardSizeMM),
	m_alignment(opt_widget.alignment()),
	m_leftRightLinked(opt_widget.leftRightLinked()),
	m_topBottomLinked(opt_widget.topBottomLinked())
{
	setMouseTracking(true);
	
	interactionState().setDefaultStatusTip(
		tr("Resize margins by dragging any of the solid lines.")
	);

	// Setup interaction stuff.
	static int const masks_by_edge[] = { TOP, RIGHT, BOTTOM, LEFT };
	static int const masks_by_corner[] = { TOP|LEFT, TOP|RIGHT, BOTTOM|RIGHT, BOTTOM|LEFT };
	for (int i = 0; i < 4; ++i) {
		// Proximity priority - inner rect higher than middle, corners higher than edges.
		m_innerCorners[i].setProximityPriorityCallback(
			boost::lambda::constant(4)
		);
		m_innerEdges[i].setProximityPriorityCallback(
			boost::lambda::constant(3)
		);
		m_middleCorners[i].setProximityPriorityCallback(
			boost::lambda::constant(2)
		);
		m_middleEdges[i].setProximityPriorityCallback(
			boost::lambda::constant(1)
		);

		// Proximity.
		m_innerCorners[i].setProximityCallback(
			boost::bind(&ImageView::cornerProximity, this, masks_by_corner[i], &m_innerRect, _1)
		);
		m_middleCorners[i].setProximityCallback(
			boost::bind(&ImageView::cornerProximity, this, masks_by_corner[i], &m_middleRect, _1)
		);
		m_innerEdges[i].setProximityCallback(
			boost::bind(&ImageView::edgeProximity, this, masks_by_edge[i], &m_innerRect, _1)
		);
		m_middleEdges[i].setProximityCallback(
			boost::bind(&ImageView::edgeProximity, this, masks_by_edge[i], &m_middleRect, _1)
		);

		// Drag initiation.
		m_innerCorners[i].setDragInitiatedCallback(
			boost::bind(&ImageView::dragInitiated, this, _1)
		);
		m_middleCorners[i].setDragInitiatedCallback(
			boost::bind(&ImageView::dragInitiated, this, _1)
		);
		m_innerEdges[i].setDragInitiatedCallback(
			boost::bind(&ImageView::dragInitiated, this, _1)
		);
		m_middleEdges[i].setDragInitiatedCallback(
			boost::bind(&ImageView::dragInitiated, this, _1)
		);

		// Drag continuation.
		m_innerCorners[i].setDragContinuationCallback(
			boost::bind(&ImageView::innerRectDragContinuation, this, masks_by_corner[i], _1)
		);
		m_middleCorners[i].setDragContinuationCallback(
			boost::bind(&ImageView::middleRectDragContinuation, this, masks_by_corner[i], _1)
		);
		m_innerEdges[i].setDragContinuationCallback(
			boost::bind(&ImageView::innerRectDragContinuation, this, masks_by_edge[i], _1)
		);
		m_middleEdges[i].setDragContinuationCallback(
			boost::bind(&ImageView::middleRectDragContinuation, this, masks_by_edge[i], _1)
		);

		// Drag finishing.
		m_innerCorners[i].setDragFinishedCallback(
			boost::bind(&ImageView::dragFinished, this)
		);
		m_middleCorners[i].setDragFinishedCallback(
			boost::bind(&ImageView::dragFinished, this)
		);
		m_innerEdges[i].setDragFinishedCallback(
			boost::bind(&ImageView::dragFinished, this)
		);
		m_middleEdges[i].setDragFinishedCallback(
			boost::bind(&ImageView::dragFinished, this)
		);

		m_innerCornerHandlers[i].setObject(&m_innerCorners[i]);
		m_middleCornerHandlers[i].setObject(&m_middleCorners[i]);
		m_innerEdgeHandlers[i].setObject(&m_innerEdges[i]);
		m_middleEdgeHandlers[i].setObject(&m_middleEdges[i]);

		Qt::CursorShape corner_cursor = (i & 1) ? Qt::SizeBDiagCursor : Qt::SizeFDiagCursor;
		m_innerCornerHandlers[i].setProximityCursor(corner_cursor);
		m_innerCornerHandlers[i].setInteractionCursor(corner_cursor);
		m_middleCornerHandlers[i].setProximityCursor(corner_cursor);
		m_middleCornerHandlers[i].setInteractionCursor(corner_cursor);

		Qt::CursorShape edge_cursor = (i & 1) ? Qt::SizeHorCursor : Qt::SizeVerCursor;
		m_innerEdgeHandlers[i].setProximityCursor(edge_cursor);
		m_innerEdgeHandlers[i].setInteractionCursor(edge_cursor);
		m_middleEdgeHandlers[i].setProximityCursor(edge_cursor);
		m_middleEdgeHandlers[i].setInteractionCursor(edge_cursor);

		makeLastFollower(m_innerCornerHandlers[i]);
		makeLastFollower(m_innerEdgeHandlers[i]);
		makeLastFollower(m_middleCornerHandlers[i]);
		makeLastFollower(m_middleEdgeHandlers[i]);
	}

	rootInteractionHandler().makeLastFollower(*this);
	rootInteractionHandler().makeLastFollower(m_dragHandler);
	rootInteractionHandler().makeLastFollower(m_zoomHandler);

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

	Settings::AggregateSizeChanged const size_changed =
		m_ptrSettings->setPageAlignment(m_pageId, alignment);
	
	recalcBoxesAndFit(calcHardMarginsMM());
	
	if (size_changed == Settings::AGGREGATE_SIZE_CHANGED) {
		emit invalidateAllThumbnails();
	} else {
		emit invalidateThumbnail(m_pageId);
	}
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
ImageView::onPaint(QPainter& painter, InteractionState const& interaction)
{	
	QColor bg_color;
	QColor fg_color;
	if (m_alignment.isNull()) {
		// "Align with other pages" is turned off.
		// Different color is useful on a thumbnail list to
		// distinguish "safe" pages from potentially problematic ones.
		bg_color = QColor(0x58, 0x7f, 0xf4, 70);
		fg_color = QColor(0x00, 0x52, 0xff);
	} else {
		bg_color = QColor(0xbb, 0x00, 0xff, 40);
		fg_color = QColor(0xbe, 0x5b, 0xec);
	}

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
	painter.setBrush(bg_color);
	painter.drawPath(outer_outline.subtracted(content_outline));
	
	QPen pen(fg_color);
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

Proximity
ImageView::cornerProximity(
	int const edge_mask, QRectF const* box, QPointF const& mouse_pos) const
{
	QRectF const r(virtualToWidget().mapRect(*box));
	QPointF pt;

	if (edge_mask & TOP) {
		pt.setY(r.top());
	} else if (edge_mask & BOTTOM) {
		pt.setY(r.bottom());
	}

	if (edge_mask & LEFT) {
		pt.setX(r.left());
	} else if (edge_mask & RIGHT) {
		pt.setX(r.right());
	}

	return Proximity(pt, mouse_pos);
}

Proximity
ImageView::edgeProximity(
	int const edge_mask, QRectF const* box, QPointF const& mouse_pos) const
{
	QRectF const r(virtualToWidget().mapRect(*box));
	QLineF line;

	switch (edge_mask) {
		case TOP:
			line.setP1(r.topLeft());
			line.setP2(r.topRight());
			break;
		case BOTTOM:
			line.setP1(r.bottomLeft());
			line.setP2(r.bottomRight());
			break;
		case LEFT:
			line.setP1(r.topLeft());
			line.setP2(r.bottomLeft());
			break;
		case RIGHT:
			line.setP1(r.topRight());
			line.setP2(r.bottomRight());
			break;
		default:
			assert(!"Unreachable");
	}

	return Proximity::pointAndLineSegment(mouse_pos, line);
}

void
ImageView::dragInitiated(QPointF const& mouse_pos)
{
	m_beforeResizing.middleWidgetRect = virtualToWidget().mapRect(m_middleRect);
	m_beforeResizing.virtToWidget = virtualToWidget();
	m_beforeResizing.widgetToVirt = widgetToVirtual();
	m_beforeResizing.mousePos = mouse_pos;
	m_beforeResizing.focalPoint = getWidgetFocalPoint();
}

void
ImageView::innerRectDragContinuation(int edge_mask, QPointF const& mouse_pos)
{
	// What really happens when we resize the inner box is resizing
	// the middle box in the opposite direction and moving the scene
	// on screen so that the object being dragged is still under mouse.

	QPointF const delta(mouse_pos - m_beforeResizing.mousePos);
	qreal left_adjust = 0;
	qreal right_adjust = 0;
	qreal top_adjust = 0;
	qreal bottom_adjust = 0;

	if (edge_mask & LEFT) {
		left_adjust = delta.x();
		if (m_leftRightLinked) {
			right_adjust = -left_adjust;
		}
	} else if (edge_mask & RIGHT) {
		right_adjust = delta.x();
		if (m_leftRightLinked) {
			left_adjust = -right_adjust;
		}
	}
	if (edge_mask & TOP) {
		top_adjust = delta.y();
		if (m_topBottomLinked) {
			bottom_adjust = -top_adjust;
		}
	} else if (edge_mask & BOTTOM) {
		bottom_adjust = delta.y();
		if (m_topBottomLinked) {
			top_adjust = -bottom_adjust;
		}
	}

	QRectF widget_rect(m_beforeResizing.middleWidgetRect);
	widget_rect.adjust(-left_adjust, -top_adjust, -right_adjust, -bottom_adjust);

	m_middleRect = m_beforeResizing.widgetToVirt.mapRect(widget_rect);
	forceNonNegativeHardMargins(m_middleRect);
	widget_rect = m_beforeResizing.virtToWidget.mapRect(m_middleRect);

	qreal effective_dx = 0;
	qreal effective_dy = 0;

	QRectF const& old_widget_rect = m_beforeResizing.middleWidgetRect;
	if (edge_mask & LEFT) {
		effective_dx = old_widget_rect.left() - widget_rect.left();
	} else if (edge_mask & RIGHT) {
		effective_dx = old_widget_rect.right() - widget_rect.right();
	}
	if (edge_mask & TOP) {
		effective_dy = old_widget_rect.top() - widget_rect.top();
	} else if (edge_mask & BOTTOM) {
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
ImageView::middleRectDragContinuation(int const edge_mask, QPointF const& mouse_pos)
{
	QPointF const delta(mouse_pos - m_beforeResizing.mousePos);
	qreal left_adjust = 0;
	qreal right_adjust = 0;
	qreal top_adjust = 0;
	qreal bottom_adjust = 0;

	QRectF const bounds(maxViewportRect());
	QRectF const old_middle_rect(m_beforeResizing.middleWidgetRect);

	if (edge_mask & LEFT) {
		left_adjust = delta.x();
		if (old_middle_rect.left() + left_adjust < bounds.left()) {
			left_adjust = bounds.left() - old_middle_rect.left();
		}
		if (m_leftRightLinked) {
			right_adjust = -left_adjust;
		}
	} else if (edge_mask & RIGHT) {
		right_adjust = delta.x();
		if (old_middle_rect.right() + right_adjust > bounds.right()) {
			right_adjust = bounds.right() - old_middle_rect.right();
		}
		if (m_leftRightLinked) {
			left_adjust = -right_adjust;
		}
	}
	if (edge_mask & TOP) {
		top_adjust = delta.y();
		if (old_middle_rect.top() + top_adjust < bounds.top()) {
			top_adjust = bounds.top() - old_middle_rect.top();
		}
		if (m_topBottomLinked) {
			bottom_adjust = -top_adjust;
		}
	} else if (edge_mask & BOTTOM) {
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

		m_middleRect = m_beforeResizing.widgetToVirt.mapRect(widget_rect);
		forceNonNegativeHardMargins(m_middleRect); // invalidates widget_rect
	}

	m_aggregateHardSizeMM = m_ptrSettings->getAggregateHardSizeMM(
		m_pageId, origRectToSizeMM(m_middleRect), m_alignment
	);

	recalcOuterRect();

	updatePresentationTransform(DONT_FIT);

	emit marginsSetLocally(calcHardMarginsMM());
}

void
ImageView::dragFinished()
{
	AggregateSizeChanged const agg_size_changed(
		commitHardMargins(calcHardMarginsMM())
	);

	QRectF const extended_viewport(maxViewportRect().adjusted(-0.5, -0.5, 0.5, 0.5));
	if (extended_viewport.contains(m_beforeResizing.middleWidgetRect)) {
		updatePresentationTransform(FIT);
	} else {
		updatePresentationTransform(DONT_FIT);
	}

	invalidateThumbnails(agg_size_changed);
}

/**
 * Updates m_middleRect and m_outerRect based on \p margins_mm,
 * m_aggregateHardSizeMM and m_alignment, updates the displayed area.
 */
void
ImageView::recalcBoxesAndFit(Margins const& margins_mm)
{
	QTransform const virt_to_mm(virtualToImage() * m_physXform.pixelsToMM());
	QTransform const mm_to_virt(m_physXform.mmToPixels() * imageToVirtual());

	QPolygonF poly_mm(virt_to_mm.map(m_innerRect));
	Utils::extendPolyRectWithMargins(poly_mm, margins_mm);

	QRectF const middle_rect(mm_to_virt.map(poly_mm).boundingRect());
	
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

	QRectF const outer_rect(mm_to_virt.map(poly_mm).boundingRect());
	updateTransformAndFixFocalPoint(ImagePresentation(imageToVirtual(), outer_rect), CENTER_IF_FITS);

	m_middleRect = middle_rect;
	m_outerRect = outer_rect;
}

/**
 * Updates the virtual image area to be displayed by ImageViewBase,
 * optionally ensuring that this area completely fits into the view.
 *
 * \note virtualToImage() and imageToVirtual() are not affected by this.
 */
void
ImageView::updatePresentationTransform(FitMode const fit_mode)
{
	if (fit_mode == DONT_FIT) {
		updateTransformPreservingScale(ImagePresentation(imageToVirtual(), m_outerRect));
	} else {
		setZoomLevel(1.0);
		updateTransformAndFixFocalPoint(
			ImagePresentation(imageToVirtual(), m_outerRect), CENTER_IF_FITS
		);
	}
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
	
	QTransform const virt_to_mm(virtualToImage() * m_physXform.pixelsToMM());

	Margins margins;
	margins.setTop(virt_to_mm.map(top_margin_line).length());
	margins.setBottom(virt_to_mm.map(bottom_margin_line).length());
	margins.setLeft(virt_to_mm.map(left_margin_line).length());
	margins.setRight(virt_to_mm.map(right_margin_line).length());
	
	return margins;
}

/**
 * \brief Recalculates m_outerRect based on m_middleRect, m_aggregateHardSizeMM
 *        and m_alignment.
 */
void
ImageView::recalcOuterRect()
{
	QTransform const virt_to_mm(virtualToImage() * m_physXform.pixelsToMM());
	QTransform const mm_to_virt(m_physXform.mmToPixels() * imageToVirtual());

	QPolygonF poly_mm(virt_to_mm.map(m_middleRect));
	
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
	
	m_outerRect = mm_to_virt.map(poly_mm).boundingRect();
}

QSizeF
ImageView::origRectToSizeMM(QRectF const& rect) const
{
	QTransform const virt_to_mm(virtualToImage() * m_physXform.pixelsToMM());

	QLineF const hor_line(rect.topLeft(), rect.topRight());
	QLineF const vert_line(rect.topLeft(), rect.bottomLeft());
	
	QSizeF const size_mm(
		virt_to_mm.map(hor_line).length(),
		virt_to_mm.map(vert_line).length()
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
