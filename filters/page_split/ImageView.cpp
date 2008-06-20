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
#include "imageproc/Constants.h"
#include "Utils.h"
#include <QRect>
#include <QRectF>
#include <QLineF>
#include <QMouseEvent>
#include <QContextMenuEvent>
#include <QMenu>
#include <QAction>
#include <QPainter>
#include <QPen>
#include <QBrush>
#include <QPixmapCache>
#include <Qt>
#include <QDebug>
#include <algorithm>
#include <math.h>

namespace page_split
{

double const ImageView::m_maxSkewAngleCtg(1.0 / tan(65 * imageproc::constants::DEG2RAD));

ImageView::ImageView(QImage const& image,
	ImageTransformation const& xform, PageLayout const& layout)
:	ImageViewBase(image, xform),
	m_pContextMenu(new QMenu(this)),
	m_pLeftHalfAction(0),
	m_pRightHalfAction(0),
	m_imgSkewingHandle(":/icons/aqua-sphere.png"),
	m_pageLayout(layout),
	m_state(DEFAULT_STATE)
{
	setMouseTracking(true);
	
	m_pLeftHalfAction = m_pContextMenu->addAction(
		tr("Left Half", "context menu item")
	);
	m_pRightHalfAction = m_pContextMenu->addAction(
		tr("Right Half", "context menu item")
	);
	m_pLeftHalfAction->setCheckable(true);
	m_pRightHalfAction->setCheckable(true);
	QActionGroup* action_group = new QActionGroup(this);
	m_pLeftHalfAction->setActionGroup(action_group);
	m_pRightHalfAction->setActionGroup(action_group);
	action_group->setExclusive(true);
	connect(
		m_pLeftHalfAction, SIGNAL(toggled(bool)),
		this, SLOT(leftHalfToggled(bool))
	);
}

ImageView::~ImageView()
{
}

void
ImageView::paintOverImage(QPainter& painter)
{
	if (m_pageLayout.isNull()) {
		return;
	}
	
	painter.setRenderHints(QPainter::Antialiasing, false);
	
	painter.setPen(Qt::NoPen);
	QRectF const virt_rect(physToVirt().resultingRect());
	if (m_pageLayout.leftPageValid()) {
		painter.setBrush(QColor(0, 0, 255, 50));
		painter.drawPolygon(m_pageLayout.leftPage(virt_rect));
	}
	if (m_pageLayout.rightPageValid()) {
		painter.setBrush(QColor(255, 0, 0, 50));
		painter.drawPolygon(m_pageLayout.rightPage(virt_rect));
	}
	
	PageLayout const layout(m_pageLayout.transformed(virtualToWidget()));
	QRectF const image_rect(virtualToWidget().mapRect(virt_rect));
	
	double const handle_half_height = 0.5 * m_imgSkewingHandle.height();
	
	QRectF image_adjusted_rect(image_rect);
	image_adjusted_rect.adjust(0.0, -handle_half_height, 0.0, handle_half_height);
	
	QRectF widget_adjusted_rect(this->rect());
	widget_adjusted_rect.adjust(0.0, handle_half_height, 0.0, -handle_half_height);
	
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
	
	QRectF const visible_widget_rect(getVisibleWidgetRect());
	
	m_topHandleRect = m_imgSkewingHandle.rect();
	m_bottomHandleRect = m_topHandleRect;
	m_topHandleRect.moveCenter(widget_split_line.p1());
	m_bottomHandleRect.moveCenter(widget_split_line.p2());
	
	if (m_topHandleRect.top() > m_bottomHandleRect.top()) {
		std::swap(m_topHandleRect, m_bottomHandleRect);
	}
	
	painter.drawPixmap(m_topHandleRect.topLeft(), m_imgSkewingHandle);
	painter.drawPixmap(m_bottomHandleRect.topLeft(), m_imgSkewingHandle);
	
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
		PageLayout const widget_layout(
			m_pageLayout.transformed(virtualToWidget())
		);
		bool const top = m_topHandleRect.contains(event->pos());
		bool bottom = m_bottomHandleRect.contains(event->pos());
		if (top || bottom) {
			m_splitLineTouchPoint = projectPointToLine(
				event->pos(), widget_layout.splitLine()
			);
			m_splitLineOtherPoint =
				(top ? m_bottomHandleRect : m_topHandleRect).center();
			m_initialMousePos = event->pos();
			m_state = SKEWING_SPLIT_LINE;
			ensureCursorShape(Qt::ClosedHandCursor);
		} else if (isCursorNearSplitLine(event->pos(),
				&m_splitLineTouchPoint, &m_splitLineOtherPoint)) {
			m_initialMousePos = event->pos();
			m_state = DRAGGING_SPLIT_LINE;
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
		emit manualPageLayoutSet(m_pageLayout);
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
		if (!m_pageLayout.isNull()) {
			if (m_topHandleRect.contains(event->pos()) ||
			    m_bottomHandleRect.contains(event->pos())) {
				if (event->buttons() & Qt::LeftButton) {
					ensureCursorShape(Qt::ClosedHandCursor);
				} else {
					ensureCursorShape(Qt::OpenHandCursor);
				}
			} else if (isCursorNearSplitLine(event->pos())) {
				ensureCursorShape(Qt::SplitHCursor);
			} else {
				ensureCursorShape(Qt::ArrowCursor);
			}
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
		if (m_state == DRAGGING_SPLIT_LINE) {
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
		PageLayout widget_layout(
			new_split_line,
			m_pageLayout.leftPageValid(),
			m_pageLayout.rightPageValid()
		);
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
		emit manualPageLayoutSet(m_pageLayout);
	}
}

void
ImageView::contextMenuEvent(QContextMenuEvent* const event)
{
	if (m_state != DEFAULT_STATE) {
		return;
	}
	if (m_pageLayout.leftPageValid() == m_pageLayout.rightPageValid()) {
		return;
	}
	
	event->accept();
	m_pLeftHalfAction->setChecked(m_pageLayout.leftPageValid());
	m_pRightHalfAction->setChecked(m_pageLayout.rightPageValid());
	m_pContextMenu->popup(event->globalPos());
}

void
ImageView::leftHalfToggled(bool const left_enabled)
{
	if (m_pageLayout.leftPageValid() == m_pageLayout.rightPageValid()) {
		return;
	}
	if (m_pageLayout.leftPageValid() == left_enabled) {
		return;
	}
	
	m_pageLayout = PageLayout(
		m_pageLayout.splitLine(), left_enabled, !left_enabled
	);
	update();
	emit manualPageLayoutSet(m_pageLayout);
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
	if (m_pageLayout.isNull()) {
		return false;
	}
	
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
