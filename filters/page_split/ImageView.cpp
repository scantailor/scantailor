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
#include "Proximity.h"
#include <algorithm>
#include <QPainter>
#include <QPen>
#include <QBrush>
#include <QColor>
#include <QtGlobal>

namespace page_split
{

ImageView::ImageView(
	QImage const& image, QImage const& downscaled_image,
	ImageTransformation const& xform, PageLayout const& layout)
:	ImageViewBase(image, downscaled_image, xform.transform(), xform.resultingCropArea()),
	m_handle1DragHandler(static_cast<DraggablePoint*>(this)),
	m_handle2DragHandler(static_cast<DraggablePoint*>(this)),
	m_lineDragHandler(static_cast<DraggableLineSegment*>(this)),
	m_dragHandler(*this),
	m_zoomHandler(*this),
	m_handlePixmap(":/icons/aqua-sphere.png"),
	m_virtLayout(layout)
{
	setMouseTracking(true);

	double const hit_radius = std::max<double>(0.5 * m_handlePixmap.width(), 15.0);
	static_cast<DraggablePoint*>(this)->setHitRadius(hit_radius);

	m_lineDragHandler.setProximityCursor(Qt::SplitHCursor);
	m_lineDragHandler.setInteractionCursor(Qt::SplitHCursor);

	QString const tip(tr("Drag the line or the handles."));
	m_handle1DragHandler.setProximityStatusTip(tip);
	m_handle2DragHandler.setProximityStatusTip(tip);
	m_lineDragHandler.setProximityStatusTip(tip);

	rootInteractionHandler().makeLastFollower(*this);
	rootInteractionHandler().makeLastFollower(m_handle1DragHandler);
	rootInteractionHandler().makeLastFollower(m_handle2DragHandler);
	rootInteractionHandler().makeLastFollower(m_lineDragHandler);
	rootInteractionHandler().makeLastFollower(m_dragHandler);
	rootInteractionHandler().makeLastFollower(m_zoomHandler);
}

ImageView::~ImageView()
{
}

void
ImageView::pageLayoutSetExternally(PageLayout const& layout)
{
	m_virtLayout = layout;
	update();
}

void
ImageView::onPaint(QPainter& painter, InteractionState const& interaction)
{
	painter.setRenderHints(QPainter::Antialiasing, false);

	painter.setPen(Qt::NoPen);
	QRectF const virt_rect(virtualDisplayRect());

	switch (m_virtLayout.type()) {
		case PageLayout::SINGLE_PAGE_UNCUT:
			painter.setBrush(QColor(0, 0, 255, 50));
			painter.drawRect(virt_rect);
			return; // No Split Line will be drawn.
		case PageLayout::LEFT_PAGE_PLUS_OFFCUT:
		case PageLayout::RIGHT_PAGE_PLUS_OFFCUT:
			painter.setBrush(QColor(0, 0, 255, 50));
			painter.drawPolygon(
				m_virtLayout.singlePageOutline(virt_rect)
			);
			break;
		case PageLayout::TWO_PAGES:
			painter.setBrush(QColor(0, 0, 255, 50));
			painter.drawPolygon(
				m_virtLayout.leftPageOutline(virt_rect)
			);
			painter.setBrush(QColor(255, 0, 0, 50));
			painter.drawPolygon(
				m_virtLayout.rightPageOutline(virt_rect)
			);
			break;
	}

	painter.setRenderHints(QPainter::Antialiasing, true);
	painter.setWorldTransform(QTransform());

	QPen pen(QColor(0, 0, 255));
	pen.setCosmetic(true);
	pen.setWidth(2);
	painter.setPen(pen);
	painter.setBrush(Qt::NoBrush);

	QLineF const line(widgetSplitLine(interaction));
	painter.drawLine(line);

	if (m_virtLayout.type() != PageLayout::SINGLE_PAGE_UNCUT) {
		QRectF rect(m_handlePixmap.rect());

		rect.moveCenter(line.p1());
		painter.drawPixmap(rect.topLeft(), m_handlePixmap);

		rect.moveCenter(line.p2());
		painter.drawPixmap(rect.topLeft(), m_handlePixmap);
	}
}

void
ImageView::dragFinished(ObjectDragHandler const*)
{
	// When a handle is being dragged, the other handle is displayed not
	// at the edge of the widget widget but at the edge of the image.
	// That means we have to redraw once dragging is over.
	// BTW, the only reason for displaying handles at widget's edges
	// is to make them visible and accessible for dragging.
	update();
	emit pageLayoutSetLocally(m_virtLayout);
}

PageLayout
ImageView::widgetLayout() const
{
	return m_virtLayout.transformed(virtualToWidget());
}

QLineF
ImageView::widgetSplitLine(InteractionState const& interaction) const
{
	QLineF line(widgetLayout().inscribedSplitLine(widgetValidArea()));

	if (m_handle2DragHandler.interactionInProgress(interaction)) {
		line.setP1(virtualToWidget().map(virtualSplitLine().p1()));
	} else if (m_handle1DragHandler.interactionInProgress(interaction)) {
		line.setP2(virtualToWidget().map(virtualSplitLine().p2()));
	}

	return line;
}

QLineF
ImageView::virtualSplitLine() const
{
	QRectF virt_rect(virtualDisplayRect());
	QRectF widget_rect(virtualToWidget().mapRect(virt_rect));

	double const delta = 0.5 * m_handlePixmap.width();
	widget_rect.adjust(delta, delta, -delta, -delta);
	virt_rect = widgetToVirtual().mapRect(widget_rect);

	return m_virtLayout.inscribedSplitLine(virt_rect);
}

QRectF
ImageView::widgetValidArea() const
{
	double const delta = 0.5 * m_handlePixmap.width();
	return getVisibleWidgetRect().adjusted(delta, delta, -delta, -delta);
}

QPointF
ImageView::pointPosition(
	ObjectDragHandler const* handler, InteractionState const& interaction) const
{
	QLineF const line(widgetSplitLine(interaction));
	return handler == &m_handle1DragHandler ? line.p1() : line.p2();
}

void
ImageView::pointMoveRequest(
	ObjectDragHandler const* handler, QPointF const& widget_pos,
	InteractionState const&)
{
	QRectF const valid_area(widgetValidArea());
	QPointF const bound_widget_pos(
		qBound(valid_area.left(), widget_pos.x(), valid_area.right()),
		widget_pos.y()
	);

	double const x = widgetToVirtual().map(bound_widget_pos).x();

	QLineF virt_line(virtualSplitLine());
	if (handler == &m_handle1DragHandler) {
		virt_line.setP1(QPointF(x, virt_line.p1().y()));
	} else {
		virt_line.setP2(QPointF(x, virt_line.p2().y()));
	}

	m_virtLayout = PageLayout(m_virtLayout.type(), virt_line);
	update();
}

QLineF
ImageView::lineSegment(
	ObjectDragHandler const*, InteractionState const& interaction) const
{
	return widgetSplitLine(interaction);
}

QPointF
ImageView::lineSegmentPosition(
	ObjectDragHandler const* handler, InteractionState const& interaction) const
{
	return widgetSplitLine(interaction).p1();
}

void
ImageView::lineSegmentMoveRequest(
	ObjectDragHandler const*, QPointF const& widget_pos, InteractionState const&)
{
	QLineF line(widgetLayout().splitLine());

	// Make the line pass through widget_pos.
	line.translate(-line.p1());
	line.translate(widget_pos);

	// Intersect with top and bottom.
	QPointF p_top;
	QPointF p_bottom;
	QRectF const valid_area(widgetValidArea());
	line.intersect(QLineF(valid_area.topLeft(), valid_area.topRight()), &p_top);
	line.intersect(QLineF(valid_area.bottomLeft(), valid_area.bottomRight()), &p_bottom);

	// Limit movement.
	double const min_x = qMin(p_top.x(), p_bottom.x());
	double const max_x = qMax(p_top.x(), p_bottom.x());
	double const left = valid_area.left() - min_x;
	double const right = max_x - valid_area.right();
	if (left > right && left > 0.0) {
		line.translate(left, 0.0);
	} else if (right > 0.0) {
		line.translate(-right, 0.0);
	}

	PageLayout const new_widget_layout(m_virtLayout.type(), line);
	m_virtLayout = new_widget_layout.transformed(widgetToVirtual());
	update();
}

} // namespace page_split
