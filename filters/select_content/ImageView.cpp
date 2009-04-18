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
#include <QMouseEvent>
#include <QContextMenuEvent>
#include <QMenu>
#include <QAction>
#include <QString>
#include <QPainter>
#include <QBrush>
#include <QPen>
#include <QColor>
#include <QDebug>
#include <Qt>

namespace select_content
{

ImageView::ImageView(
	QImage const& image, QImage const& downscaled_image,
	ImageTransformation const& xform, QRectF const& content_rect)
:	ImageViewBase(image, downscaled_image, xform),
	m_pNoContentMenu(new QMenu(this)),
	m_pHaveContentMenu(new QMenu(this)),
	m_contentRect(content_rect),
	m_resizingMask(0)
{
	setMouseTracking(true);
	
	m_defaultStatusTip = tr("Use the context menu to enable / disable the content box.");
	m_resizeStatusTip = tr("Drag lines or corners to resize the content box.");
	ensureStatusTip(defaultStatusTip());
	
	QAction* create = m_pNoContentMenu->addAction(tr("Create Content Box"));
	QAction* remove = m_pHaveContentMenu->addAction(tr("Remove Content Box"));
	connect(create, SIGNAL(triggered(bool)), this, SLOT(createContentBox()));
	connect(remove, SIGNAL(triggered(bool)), this, SLOT(removeContentBox()));
}

ImageView::~ImageView()
{
}

void
ImageView::paintOverImage(QPainter& painter)
{
	if (m_contentRect.isNull()) {
		return;
	}
	
	painter.setRenderHints(QPainter::Antialiasing, true);
	
	// Draw the content bounding box.
	QPen pen(QColor(0x00, 0x00, 0xff));
	pen.setWidth(1);
	pen.setCosmetic(true);
	painter.setPen(pen);
	
	painter.setBrush(QColor(0x00, 0x00, 0xff, 50));
	
	// Adjust to compensate for pen width.
	painter.drawRect(m_contentRect.adjusted(-0.5, -0.5, 0.5, 0.5));
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
		m_widgetRectBeforeResizing = virtualToWidget().mapRect(m_contentRect);
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
		emit manualContentRectSet(m_contentRect);
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
		if (ver || hor) {
			ensureStatusTip(m_resizeStatusTip);
		} else {
			ensureStatusTip(m_defaultStatusTip);
		}
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
		
		forceMinWidthAndHeight(widget_rect);
		forceInsideImage(widget_rect);
		
		QRectF const new_content_rect(
			virtualToWidget().inverted().mapRect(widget_rect)
		);
		if (new_content_rect != m_contentRect) {
			m_contentRect = new_content_rect;
			update();
		}
	}
}

void
ImageView::hideEvent(QHideEvent* const event)
{
	ImageViewBase::hideEvent(event);
	int const old_resizing_mask = m_resizingMask;
	m_resizingMask = 0;
	ensureCursorShape(Qt::ArrowCursor);
	if (old_resizing_mask) {
		emit manualContentRectSet(m_contentRect);
	}
}

void
ImageView::contextMenuEvent(QContextMenuEvent* const event)
{
	if (m_resizingMask != 0) {
		// No context menus during resizing.
		return;
	}
	
	event->accept();
	
	if (m_contentRect.isEmpty()) {
		m_pNoContentMenu->popup(event->globalPos());
	} else {
		m_pHaveContentMenu->popup(event->globalPos());
	}
}

QString
ImageView::defaultStatusTip() const
{
	return m_defaultStatusTip;
}

void
ImageView::createContentBox()
{
	if (!m_contentRect.isEmpty()) {
		return;
	}
	if (m_resizingMask) {
		return;
	}
	if (isDraggingInProgress()) {
		return;
	}
	
	QRectF const virtual_rect(imageToVirt().resultingRect());
	QRectF content_rect(0, 0, virtual_rect.width() * 0.7, virtual_rect.height() * 0.7);
	content_rect.moveCenter(virtual_rect.center());
	m_contentRect = content_rect;
	update();
	emit manualContentRectSet(m_contentRect);
}

void
ImageView::removeContentBox()
{
	if (m_contentRect.isEmpty()) {
		return;
	}
	if (m_resizingMask) {
		return;
	}
	if (isDraggingInProgress()) {
		return;
	}
	
	m_contentRect = QRectF();
	update();
	emit manualContentRectSet(m_contentRect);
}

int
ImageView::cursorLocationMask(QPoint const& cursor_pos) const
{
	QRect const rect(virtualToWidget().mapRect(m_contentRect).toRect());
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
ImageView::forceMinWidthAndHeight(QRectF& widget_rect) const
{
	double const min_width = 15;
	double const min_height = 15;
	
	if (widget_rect.width() < min_width) {
		if (m_resizingMask & LEFT_EDGE) {
			widget_rect.setLeft(widget_rect.right() - min_width);
		} else if (m_resizingMask & RIGHT_EDGE) {
			widget_rect.setRight(widget_rect.left() + min_width);
		}
	}
	if (widget_rect.height() < min_height) {
		if (m_resizingMask & TOP_EDGE) {
			widget_rect.setTop(widget_rect.bottom() - min_height);
		} else if (m_resizingMask & BOTTOM_EDGE) {
			widget_rect.setBottom(widget_rect.top() + min_height);
		}
	}
}

void
ImageView::forceInsideImage(QRectF& widget_rect) const
{
	QRectF const image_rect(getVisibleWidgetRect());
	if ((m_resizingMask & LEFT_EDGE) &&
			widget_rect.left() < image_rect.left()) {
		widget_rect.setLeft(image_rect.left());
	}
	if ((m_resizingMask & RIGHT_EDGE) &&
			widget_rect.right() > image_rect.right()) {
		widget_rect.setRight(image_rect.right());
	}
	if ((m_resizingMask & TOP_EDGE) &&
			widget_rect.top() < image_rect.top()) {
		widget_rect.setTop(image_rect.top());
	}
	if ((m_resizingMask & BOTTOM_EDGE) &&
			widget_rect.bottom() > image_rect.bottom()) {
		widget_rect.setBottom(image_rect.bottom());
	}
}

} // namespace select_content
