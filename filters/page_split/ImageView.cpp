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
#include "ImageTransformation.h"
#include "imageproc/Constants.h"
#include "Utils.h"
#include <QRect>
#include <QRectF>
#include <QLineF>
#include <QMouseEvent>
#include <QPainter>
#include <QPen>
#include <QBrush>
#include <Qt>
#include <QDebug>
#include <algorithm>
#include <math.h>

namespace page_split
{

double const ImageView::m_maxSkewAngleCtg(1.0 / tan(65 * imageproc::constants::DEG2RAD));

ImageView::ImageView(
	QImage const& image, QImage const& downscaled_image,
	ImageTransformation const& xform, PageLayout const& layout)
:	ImageViewBase(image, downscaled_image, xform.transform(), xform.resultingCropArea()),
	m_imgSkewingHandle(":/icons/aqua-sphere.png"),
	m_pageLayout(layout),
	m_state(DEFAULT_STATE)
{
	m_dragHandleStatusTip = tr("Drag this handle to skew the line.");
	m_dragLineStatusTip = tr("This line can be dragged.");
	
	setMouseTracking(true);
}

ImageView::~ImageView()
{
}

void
ImageView::pageLayoutSetExternally(PageLayout const& layout)
{
	m_pageLayout = layout;
	m_state = DEFAULT_STATE;
	update();
}

void
ImageView::paintOverImage(QPainter& painter)
{
	painter.setRenderHints(QPainter::Antialiasing, false);
	
	painter.setPen(Qt::NoPen);
	QRectF const virt_rect(virtualDisplayRect());
	
	switch (m_pageLayout.type()) {
		case PageLayout::SINGLE_PAGE_UNCUT:
			painter.setBrush(QColor(0, 0, 255, 50));
			painter.drawRect(virt_rect);
			return; // No Split Line will be drawn.
		case PageLayout::LEFT_PAGE_PLUS_OFFCUT:
		case PageLayout::RIGHT_PAGE_PLUS_OFFCUT:
			painter.setBrush(QColor(0, 0, 255, 50));
			painter.drawPolygon(
				m_pageLayout.singlePageOutline(virt_rect)
			);
			break;
		case PageLayout::TWO_PAGES:
			painter.setBrush(QColor(0, 0, 255, 50));
			painter.drawPolygon(
				m_pageLayout.leftPageOutline(virt_rect)
			);
			painter.setBrush(QColor(255, 0, 0, 50));
			painter.drawPolygon(
				m_pageLayout.rightPageOutline(virt_rect)
			);
			break;
	}
	
	PageLayout const layout(m_pageLayout.transformed(virtualToWidget()));
	QRectF const image_rect(virtualToWidget().mapRect(virt_rect));
	
	double const handle_half_height = 0.5 * m_imgSkewingHandle.height();
	
	bool draw_top_handle = true;
	bool draw_bottom_handle = true;
	
	// There is one case where we don't want to draw one of the
	// handles: it's when skewing a line in a zoomed image.
	// The reason is that the handle opposite to the one we are
	// dragging, is not the rotation origin in this case.
	if (m_state == DRAGGING_TOP_HANDLE) {
		QLineF const image_split_line(layout.inscribedSplitLine(image_rect));
		double const bottom_y = std::max(image_split_line.p1().y(), image_split_line.p2().y());
		draw_bottom_handle = bottom_y - handle_half_height < height();
	} else if (m_state == DRAGGING_BOTTOM_HANDLE) {
		QLineF const image_split_line(layout.inscribedSplitLine(image_rect));
		double const top_y = std::min(image_split_line.p1().y(), image_split_line.p2().y());
		draw_top_handle = top_y + handle_half_height > 0.0;
	}
	
	QRectF image_adjusted_rect(image_rect);
	image_adjusted_rect.adjust(
		0.0, draw_top_handle ? handle_half_height : 0.0,
		0.0, draw_bottom_handle ? -handle_half_height : 0.0
	);
	
	QRectF widget_adjusted_rect(this->rect());
	widget_adjusted_rect.adjust(
		0.0, draw_top_handle ? handle_half_height : 0.0,
		0.0, draw_bottom_handle ? -handle_half_height : 0.0
	);
	
	QRectF const united(image_adjusted_rect | widget_adjusted_rect);
	QRectF const intersected(image_adjusted_rect & widget_adjusted_rect);
	
	QRectF resulting_rect;
	resulting_rect.setLeft(united.left());
	resulting_rect.setRight(united.right());
	resulting_rect.setTop(intersected.top());
	resulting_rect.setBottom(intersected.bottom());
	
	QLineF const widget_split_line(layout.inscribedSplitLine(resulting_rect));
	
	painter.setRenderHints(QPainter::Antialiasing, true);
	painter.setWorldMatrixEnabled(false);
	
	QPen pen(QColor(0, 0, 255));
	pen.setCosmetic(true);
	pen.setWidth(2);
	painter.setPen(pen);
	painter.setBrush(Qt::NoBrush);
	
	painter.drawLine(widget_split_line.p1(), widget_split_line.p2());
	
	m_topHandleRect = m_imgSkewingHandle.rect();
	m_bottomHandleRect = m_topHandleRect;
	m_topHandleRect.moveCenter(widget_split_line.p1());
	m_bottomHandleRect.moveCenter(widget_split_line.p2());
	
	if (m_topHandleRect.top() > m_bottomHandleRect.top()) {
		std::swap(m_topHandleRect, m_bottomHandleRect);
	}
	
	if (draw_top_handle) {
		painter.drawPixmap(m_topHandleRect.topLeft(), m_imgSkewingHandle);
	}
	if (draw_bottom_handle) {
		painter.drawPixmap(m_bottomHandleRect.topLeft(), m_imgSkewingHandle);
	}
	
	// Restore the world transform as it was.
	painter.setWorldMatrixEnabled(true);
}

void
ImageView::wheelEvent(QWheelEvent* const event)
{
	if (m_state == DEFAULT_STATE) {
		handleZooming(event);
	}
}

void
ImageView::mousePressEvent(QMouseEvent* const event)
{
	if (m_state == DEFAULT_STATE && !isDraggingInProgress()
	    && event->button() == Qt::LeftButton) {
		bool const top = m_topHandleRect.contains(event->pos());
		bool const bottom = m_bottomHandleRect.contains(event->pos());
		if (top || bottom) {
			QRectF const virt_rect(virtualDisplayRect());
			QRectF const image_rect(virtualToWidget().mapRect(virt_rect));
			PageLayout const layout(m_pageLayout.transformed(virtualToWidget()));
			QLineF const line(layout.inscribedSplitLine(image_rect));
			m_splitLineTouchPoint = projectPointToLine(event->pos(), line);
			m_splitLineOtherPoint = (top == (line.p1().y() <= line.p2().y()))
				? line.p2() : line.p1();
			m_initialMousePos = event->pos();
			m_state = top ? DRAGGING_TOP_HANDLE : DRAGGING_BOTTOM_HANDLE;
			ensureCursorShape(Qt::ClosedHandCursor);
		} else if (isCursorNearSplitLine(event->pos(),
				&m_splitLineTouchPoint, &m_splitLineOtherPoint)) {
			m_initialMousePos = event->pos();
			m_state = DRAGGING_LINE;
		}
	}
	
	if (m_state == DEFAULT_STATE) {
		handleImageDragging(event);
	} else {
		m_touchPointRange = getVisibleWidgetRect();
		extendToContain(m_touchPointRange, m_splitLineTouchPoint);
	}
}

void
ImageView::mouseReleaseEvent(QMouseEvent* const event)
{
	if (isDraggingInProgress()) {
		handleImageDragging(event);
		return;
	}
	
	if (event->button() == Qt::LeftButton && m_state != DEFAULT_STATE) {
		m_state = DEFAULT_STATE;
		update(); // Because one of the handles may not be drawn when skewing.
		emit pageLayoutSetLocally(m_pageLayout);
	}
}

void
ImageView::mouseMoveEvent(QMouseEvent* const event)
{
	if (isDraggingInProgress()) {
		handleImageDragging(event);
		return;
	}
	
	if (m_state == DEFAULT_STATE) {
		if (m_topHandleRect.contains(event->pos()) ||
				m_bottomHandleRect.contains(event->pos())) {
			ensureStatusTip(m_dragHandleStatusTip);
			if (event->buttons() & Qt::LeftButton) {
				ensureCursorShape(Qt::ClosedHandCursor);
			} else {
				ensureCursorShape(Qt::OpenHandCursor);
			}
		} else if (isCursorNearSplitLine(event->pos())) {
			ensureStatusTip(m_dragLineStatusTip);
			ensureCursorShape(Qt::SplitHCursor);
		} else {
			ensureStatusTip(defaultStatusTip());
			ensureCursorShape(Qt::ArrowCursor);
		}
	} else {
		QPointF movement(event->pos() - m_initialMousePos);
		movement.setY(0);
		
		QPointF new_touch_point(m_splitLineTouchPoint + movement);
		
		if (!m_touchPointRange.contains(new_touch_point)) {
			forcePointIntoRect(new_touch_point, m_touchPointRange);
			movement = new_touch_point - m_splitLineTouchPoint;
		}
		
		QPointF new_other_point(m_splitLineOtherPoint);
		if (m_state == DRAGGING_LINE) {
			new_other_point += movement;
		} else {
			// Limit the max skew angle.
			double xdiff = new_touch_point.x() - new_other_point.x();
			double ydiff = new_touch_point.y() - new_other_point.y();
			if (xdiff == 0.0 && ydiff == 0.0) {
				new_touch_point = m_splitLineTouchPoint;
			} else if (fabs(xdiff) > fabs(ydiff) * m_maxSkewAngleCtg) {
				double const xsign = xdiff >= 0.0 ? 1.0 : -1.0;
				double const ysign = (m_splitLineTouchPoint.y() -
					m_splitLineOtherPoint.y()) >= 0.0 ? 1.0 : -1.0;
				xdiff = m_maxSkewAngleCtg * xsign;
				ydiff = ysign;
				new_touch_point.setX(new_other_point.x() + xdiff);
				new_touch_point.setY(new_other_point.y() + ydiff);
			}
		}
		
		QLineF const new_split_line(new_touch_point, new_other_point);
		
		// Page layout in widget coordinates.
		PageLayout widget_layout(m_pageLayout.type(), new_split_line);
		m_pageLayout = widget_layout.transformed(widgetToVirtual());
		update();
	}
}

void
ImageView::hideEvent(QHideEvent* const event)
{
	ImageViewBase::hideEvent(event);
	State const old_state = m_state;
	m_state = DEFAULT_STATE;
	ensureCursorShape(Qt::ArrowCursor);
	if (old_state != DEFAULT_STATE) {
		emit pageLayoutSetLocally(m_pageLayout);
	}
}

/**
 * Extend the rectangle to contain the point.
 * Edges are considered to be inside the rectangle.
 */
void
ImageView::extendToContain(QRectF& rect, QPointF const& point)
{
	if (point.x() < rect.left()) {
		rect.setLeft(point.x());
	} else if (point.x() > rect.right()) {
		rect.setRight(point.x());
	}
	if (point.y() < rect.top()) {
		rect.setTop(point.y());
	} else if (point.y() > rect.bottom()) {
		rect.setBottom(point.y());
	}
}

/**
 * If the point is outside of the rectangle, move it inside.
 * Edges are considered to be inside the rectangle.
 */
void
ImageView::forcePointIntoRect(QPointF& point, QRectF const& rect)
{
	if (point.x() < rect.left()) {
		point.setX(rect.left());
	} else if (point.x() > rect.right()) {
		point.setX(rect.right());
	}
	if (point.y() < rect.top()) {
		point.setY(rect.top());
	} else if (point.y() > rect.bottom()) {
		point.setY(rect.bottom());
	}
}

double
ImageView::distanceSquared(QPointF const p1, QPointF const p2)
{
	double const dx = p1.x() - p2.x();
	double const dy = p1.y() - p2.y();
	return dx * dx + dy * dy;
}

QPointF
ImageView::projectPointToLine(QPointF const& point, QLineF const& line)
{
	// A line orthogonal to the given line and going
	// through the given point.
	QLineF orth_line(line.normalVector());
	orth_line.translate(-orth_line.p1());
	orth_line.translate(point);
	
	// Their intersection.
	QPointF intersection;
	orth_line.intersect(line, &intersection);
	
	return intersection;
}

/**
 * Checks if the cursor is near the split line, and if so, optionally
 * sets \p touchpoint to be the point on the split line near the cursor,
 * and \p far_end to be one of split line's endpoints, the one further away
 * from \p touchpoint.\n
 * \p touchpoint and \p far_end are returned in widget coordinates.
 */
bool
ImageView::isCursorNearSplitLine(
	QPointF const& cursor_pos, QPointF* touchpoint, QPointF* far_end) const
{
#if 0
	if (m_pageLayout.isNull()) {
		return false;
	}
#else
	if (m_pageLayout.type() == PageLayout::SINGLE_PAGE_UNCUT) {
		// No split line in this mode.
		return false;
	}
#endif
	
	double const max_distance = 5.0; // screen pixels
	double const max_distance_sq = max_distance * max_distance;
	
	// Page layout in widget coordinates.
	PageLayout const widget_layout(m_pageLayout.transformed(virtualToWidget()));
	
	// Projection of cursor_pos to the split line.
	QPointF const projection(projectPointToLine(cursor_pos, widget_layout.splitLine()));
	
	if (distanceSquared(cursor_pos, projection) <= max_distance_sq) {
		QRectF const visible_dst_rect(getVisibleWidgetRect());
		QRectF bounds(visible_dst_rect);
		bounds.adjust(-max_distance, -max_distance, max_distance, max_distance);
		if (bounds.contains(cursor_pos)) {
			if (touchpoint) {
				*touchpoint = projection;
			}
			if (far_end) {
				QLineF const bounded_line(
					widget_layout.inscribedSplitLine(visible_dst_rect)
				);
				if (distanceSquared(projection, bounded_line.p1()) >
				    distanceSquared(projection, bounded_line.p2())) {
					*far_end = bounded_line.p1();
				} else {
					*far_end = bounded_line.p2();
				}
			}
			return true;
		}
	}
	
	return false;
}

} // namespace page_split
