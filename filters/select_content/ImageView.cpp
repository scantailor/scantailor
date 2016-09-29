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
#include "ImageTransformation.h"
#include "ImagePresentation.h"
#include <QMouseEvent>
#include <QContextMenuEvent>
#include <QMenu>
#include <QAction>
#include <QString>
#include <QPainter>
#include <QBrush>
#include <QPen>
#include <QColor>
#include <QCursor>
#include <QDebug>
#include <Qt>
#ifndef Q_MOC_RUN
#include <boost/bind.hpp>
#endif
#include <algorithm>

namespace select_content
{

ImageView::ImageView(
	QImage const& image, QImage const& downscaled_image,
	ImageTransformation const& xform, QRectF const& content_rect)
:	ImageViewBase(
		image, downscaled_image,
		ImagePresentation(xform.transform(), xform.resultingPreCropArea())
	),
	m_dragHandler(*this),
	m_zoomHandler(*this),
	m_pNoContentMenu(new QMenu(this)),
	m_pHaveContentMenu(new QMenu(this)),
	m_contentRect(content_rect),
	m_minBoxSize(10.0, 10.0)
{
	setMouseTracking(true);

	interactionState().setDefaultStatusTip(
		tr("Use the context menu to enable / disable the content box.")
	);

	QString const drag_tip(tr("Drag lines or corners to resize the content box."));

	// Setup corner drag handlers.
	static int const masks_by_corner[] = { TOP|LEFT, TOP|RIGHT, BOTTOM|RIGHT, BOTTOM|LEFT };
	for (int i = 0; i < 4; ++i) {
		m_corners[i].setPositionCallback(
			boost::bind(&ImageView::cornerPosition, this, masks_by_corner[i])
		);
		m_corners[i].setMoveRequestCallback(
			boost::bind(&ImageView::cornerMoveRequest, this, masks_by_corner[i], _1)
		);
		m_corners[i].setDragFinishedCallback(
			boost::bind(&ImageView::dragFinished, this)
		);
		m_cornerHandlers[i].setObject(&m_corners[i]);
		m_cornerHandlers[i].setProximityStatusTip(drag_tip);
		Qt::CursorShape cursor = (i & 1) ? Qt::SizeBDiagCursor : Qt::SizeFDiagCursor;
		m_cornerHandlers[i].setProximityCursor(cursor);
		m_cornerHandlers[i].setInteractionCursor(cursor);
		makeLastFollower(m_cornerHandlers[i]);
	}

	// Setup edge drag handlers.
	static int const masks_by_edge[] = { TOP, RIGHT, BOTTOM, LEFT };
	for (int i = 0; i < 4; ++i) {
		m_edges[i].setPositionCallback(
			boost::bind(&ImageView::edgePosition, this, masks_by_edge[i])
		);
		m_edges[i].setMoveRequestCallback(
			boost::bind(&ImageView::edgeMoveRequest, this, masks_by_edge[i], _1)
		);
		m_edges[i].setDragFinishedCallback(
			boost::bind(&ImageView::dragFinished, this)
		);
		m_edgeHandlers[i].setObject(&m_edges[i]);
		m_edgeHandlers[i].setProximityStatusTip(drag_tip);
		Qt::CursorShape cursor = (i & 1) ? Qt::SizeHorCursor : Qt::SizeVerCursor;
		m_edgeHandlers[i].setProximityCursor(cursor);
		m_edgeHandlers[i].setInteractionCursor(cursor);
		makeLastFollower(m_edgeHandlers[i]);
	}

	rootInteractionHandler().makeLastFollower(*this);
	rootInteractionHandler().makeLastFollower(m_dragHandler);
	rootInteractionHandler().makeLastFollower(m_zoomHandler);
	
	QAction* create = m_pNoContentMenu->addAction(tr("Create Content Box"));
	QAction* remove = m_pHaveContentMenu->addAction(tr("Remove Content Box"));
	connect(create, SIGNAL(triggered(bool)), this, SLOT(createContentBox()));
	connect(remove, SIGNAL(triggered(bool)), this, SLOT(removeContentBox()));
}

ImageView::~ImageView()
{
}

void
ImageView::createContentBox()
{
	if (!m_contentRect.isEmpty()) {
		return;
	}
	if (interactionState().captured()) {
		return;
	}
	
	QRectF const virtual_rect(virtualDisplayRect());
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
	if (interactionState().captured()) {
		return;
	}
	
	m_contentRect = QRectF();
	update();
	emit manualContentRectSet(m_contentRect);
}

void
ImageView::onPaint(QPainter& painter, InteractionState const& interaction)
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

	// Pen strokes will be outside of m_contentRect - that's how drawRect() works.
	painter.drawRect(m_contentRect);
}

void
ImageView::onContextMenuEvent(QContextMenuEvent* event, InteractionState& interaction)
{
	if (interaction.captured()) {
		// No context menus during resizing.
		return;
	}

	if (m_contentRect.isEmpty()) {
		m_pNoContentMenu->popup(event->globalPos());
	} else {
		m_pHaveContentMenu->popup(event->globalPos());
	}
}

QPointF
ImageView::cornerPosition(int edge_mask) const
{
	QRectF const r(virtualToWidget().mapRect(m_contentRect));
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

	return pt;
}

void
ImageView::cornerMoveRequest(int edge_mask, QPointF const& pos)
{
	QRectF r(virtualToWidget().mapRect(m_contentRect));
	qreal const minw = m_minBoxSize.width();
	qreal const minh = m_minBoxSize.height();

	if (edge_mask & TOP) {
		r.setTop(std::min(pos.y(), r.bottom() - minh));
	} else if (edge_mask & BOTTOM) {
		r.setBottom(std::max(pos.y(), r.top() + minh));
	}

	if (edge_mask & LEFT) {
		r.setLeft(std::min(pos.x(), r.right() - minw));
	} else if (edge_mask & RIGHT) {
		r.setRight(std::max(pos.x(), r.left() + minw));
	}

	forceInsideImage(r, edge_mask);
	m_contentRect = widgetToVirtual().mapRect(r);
	update();
}

QLineF
ImageView::edgePosition(int const edge) const
{
	QRectF const r(virtualToWidget().mapRect(m_contentRect));

	if (edge == TOP) {
		return QLineF(r.topLeft(), r.topRight());
	} else if (edge == BOTTOM) {
		return QLineF(r.bottomLeft(), r.bottomRight());
	} else if (edge == LEFT) {
		return QLineF(r.topLeft(), r.bottomLeft());
	} else {
		return QLineF(r.topRight(), r.bottomRight());
	}
}

void
ImageView::edgeMoveRequest(int const edge, QLineF const& line)
{
	cornerMoveRequest(edge, line.p1());
}

void
ImageView::dragFinished()
{
	emit manualContentRectSet(m_contentRect);
}

void
ImageView::forceInsideImage(QRectF& widget_rect, int const edge_mask) const
{
	qreal const minw = m_minBoxSize.width();
	qreal const minh = m_minBoxSize.height();
	QRectF const image_rect(getOccupiedWidgetRect());

	if ((edge_mask & LEFT) && widget_rect.left() < image_rect.left()) {
		widget_rect.setLeft(image_rect.left());
		widget_rect.setRight(std::max(widget_rect.right(), widget_rect.left() + minw));
	}
	if ((edge_mask & RIGHT) && widget_rect.right() > image_rect.right()) {
		widget_rect.setRight(image_rect.right());
		widget_rect.setLeft(std::min(widget_rect.left(), widget_rect.right() - minw));
	}
	if ((edge_mask & TOP) && widget_rect.top() < image_rect.top()) {
		widget_rect.setTop(image_rect.top());
		widget_rect.setBottom(std::max(widget_rect.bottom(), widget_rect.top() + minh));
	}
	if ((edge_mask & BOTTOM) && widget_rect.bottom() > image_rect.bottom()) {
		widget_rect.setBottom(image_rect.bottom());
		widget_rect.setTop(std::min(widget_rect.top(), widget_rect.bottom() - minh));
	}
}

} // namespace select_content
