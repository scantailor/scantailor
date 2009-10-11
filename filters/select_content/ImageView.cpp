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
#include <algorithm>

namespace select_content
{

ImageView::ImageView(
	QImage const& image, QImage const& downscaled_image,
	ImageTransformation const& xform, QRectF const& content_rect)
:	ImageViewBase(image, downscaled_image, xform.transform(), xform.resultingCropArea()),
	m_dragHandler(*this),
	m_zoomHandler(*this),
	m_pNoContentMenu(new QMenu(this)),
	m_pHaveContentMenu(new QMenu(this)),
	m_contentRect(content_rect)
{
	setMouseTracking(true);

	setMinBoxSize(QSizeF(15, 15));
	rootInteractionHandler().makeLastFollower(static_cast<BoxResizeHandler&>(*this));
	rootInteractionHandler().makeLastFollower(m_dragHandler);
	rootInteractionHandler().makeLastFollower(m_zoomHandler);

	interactionState().setDefaultStatusTip(
		tr("Use the context menu to enable / disable the content box.")
	);
	static_cast<BoxResizeHandler*>(this)->setProximityStatusTip(
		tr("Drag lines or corners to resize the content box.")
	);
	
	QAction* create = m_pNoContentMenu->addAction(tr("Create Content Box"));
	QAction* remove = m_pHaveContentMenu->addAction(tr("Remove Content Box"));
	connect(create, SIGNAL(triggered(bool)), this, SLOT(createContentBox()));
	connect(remove, SIGNAL(triggered(bool)), this, SLOT(removeContentBox()));
}

ImageView::~ImageView()
{
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
	
	// Adjust to compensate for pen width.
	painter.drawRect(m_contentRect.adjusted(-0.5, -0.5, 0.5, 0.5));
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

QRectF
ImageView::boxPosition(BoxResizeHandler const*, InteractionState const&) const
{
	return virtualToWidget().mapRect(m_contentRect);
}

void
ImageView::boxResizeRequest(
	BoxResizeHandler const*, QRectF const& rect, InteractionState const&)
{
	m_contentRect = widgetToVirtual().mapRect(forceInsideImage(rect));
	update();
}

void
ImageView::boxResizeFinished(BoxResizeHandler const* handler)
{
	emit manualContentRectSet(m_contentRect);
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

QRectF
ImageView::forceInsideImage(QRectF widget_rect) const
{
	double const minw(minBoxSize().width());
	double const minh(minBoxSize().height());
	QRectF const image_rect(getVisibleWidgetRect());

	if (widget_rect.left() < image_rect.left()) {
		widget_rect.setLeft(image_rect.left());
		widget_rect.setRight(std::max(widget_rect.right(), widget_rect.left() + minw));
	}
	if (widget_rect.right() > image_rect.right()) {
		widget_rect.setRight(image_rect.right());
		widget_rect.setLeft(std::min(widget_rect.left(), widget_rect.right() - minw));
	}
	if (widget_rect.top() < image_rect.top()) {
		widget_rect.setTop(image_rect.top());
		widget_rect.setBottom(std::max(widget_rect.bottom(), widget_rect.top() + minh));
	}
	if (widget_rect.bottom() > image_rect.bottom()) {
		widget_rect.setBottom(image_rect.bottom());
		widget_rect.setTop(std::min(widget_rect.top(), widget_rect.bottom() - minh));
	}

	return widget_rect;
}

} // namespace select_content
