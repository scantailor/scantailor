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
#include "ImagePresentation.h"
#include "Proximity.h"
#include <QPainter>
#include <QPen>
#include <QBrush>
#include <QColor>
#include <QtGlobal>
#include <QDebug>
#include <boost/bind.hpp>
#include <algorithm>

namespace page_split
{

ImageView::ImageView(
	QImage const& image, QImage const& downscaled_image,
	ImageTransformation const& xform, PageLayout const& layout)
:	ImageViewBase(
		image, downscaled_image,
		ImagePresentation(xform.transform(), xform.resultingCropArea())
	),
	m_lineInteractor(&m_lineSegment),
	m_dragHandler(*this),
	m_zoomHandler(*this),
	m_handlePixmap(":/icons/aqua-sphere.png"),
	m_virtLayout(layout)
{
	setMouseTracking(true);

	QString const tip(tr("Drag the line or the handles."));
	double const hit_radius = std::max<double>(0.5 * m_handlePixmap.width(), 15.0);
	for (int i = 0; i < 2; ++i) {
		m_handles[i].setHitRadius(hit_radius);
		m_handles[i].setPositionCallback(
			boost::bind(&ImageView::handlePosition, this, i)
		);
		m_handles[i].setMoveRequestCallback(
			boost::bind(&ImageView::handleMoveRequest, this, i, _1)
		);
		m_handles[i].setDragFinishedCallback(
			boost::bind(&ImageView::dragFinished, this)
		);

		m_handleInteractors[i].setProximityStatusTip(tip);
		m_handleInteractors[i].setObject(&m_handles[i]);

		makeLastFollower(m_handleInteractors[i]);
	}

	m_lineSegment.setPositionCallback(
		boost::bind(&ImageView::linePosition, this)
	);
	m_lineSegment.setMoveRequestCallback(
		boost::bind(&ImageView::lineMoveRequest, this, _1)
	);
	m_lineSegment.setDragFinishedCallback(
		boost::bind(&ImageView::dragFinished, this)
	);

	m_lineInteractor.setProximityCursor(Qt::SplitHCursor);
	m_lineInteractor.setInteractionCursor(Qt::SplitHCursor);
	m_lineInteractor.setProximityStatusTip(tip);

	makeLastFollower(m_lineInteractor);

	rootInteractionHandler().makeLastFollower(*this);
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
	painter.setRenderHint(QPainter::Antialiasing, false);
	painter.setRenderHint(QPainter::SmoothPixmapTransform, true);

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

	painter.setRenderHint(QPainter::Antialiasing, true);
	painter.setWorldTransform(QTransform());

	QPen pen(QColor(0, 0, 255));
	pen.setCosmetic(true);
	pen.setWidth(2);
	painter.setPen(pen);
	painter.setBrush(Qt::NoBrush);

	QLineF const line(widgetSplitLine());
	painter.drawLine(line);

	if (m_virtLayout.type() != PageLayout::SINGLE_PAGE_UNCUT) {
		QRectF rect(m_handlePixmap.rect());

		rect.moveCenter(line.p1());
		painter.drawPixmap(rect.topLeft(), m_handlePixmap);

		rect.moveCenter(line.p2());
		painter.drawPixmap(rect.topLeft(), m_handlePixmap);
	}
}

PageLayout
ImageView::widgetLayout() const
{
	return m_virtLayout.transformed(virtualToWidget());
}

QLineF
ImageView::widgetSplitLine() const
{
	QLineF line(customInscribedSplitLine(widgetLayout().splitLine(), reducedWidgetArea()));

	if (m_handleInteractors[1].interactionInProgress(interactionState())) {
		line.setP1(virtualToWidget().map(virtualSplitLine().p1()));
	} else if (m_handleInteractors[0].interactionInProgress(interactionState())) {
		line.setP2(virtualToWidget().map(virtualSplitLine().p2()));
	}

	return line;
}

QLineF
ImageView::virtualSplitLine() const
{
	QRectF virt_rect(virtualDisplayRect());
	QRectF const widget_rect(virtualToWidget().mapRect(virt_rect));
	virt_rect = widgetToVirtual().mapRect(reducedWidgetArea());

	return customInscribedSplitLine(m_virtLayout.splitLine(), virt_rect);
}

QRectF
ImageView::reducedWidgetArea() const
{
	double const delta = 0.5 * m_handlePixmap.width();
	return getOccupiedWidgetRect().adjusted(0.0, delta, 0.0, -delta);
}

/**
 * This implementation differs from PageLayout::inscribedSplitLine() in that
 * it forces the endpoints to lie on the top and bottom boundary lines.
 * Line's angle may change as a result.
 */
QLineF
ImageView::customInscribedSplitLine(QLineF const& line, QRectF const& rect)
{
	if (line.p1().y() == line.p2().y()) {
		// This should not happen, but if it does, we need to handle it gracefully.
		double middle_x = 0.5 * (line.p1().x() + line.p2().x());
		middle_x = qBound(rect.left(), middle_x, rect.right());
		return QLineF(QPointF(middle_x, rect.top()), QPointF(middle_x, rect.bottom()));
	}

	QPointF top_pt;
	QPointF bottom_pt;

	line.intersect(QLineF(rect.topLeft(), rect.topRight()), &top_pt);
	line.intersect(QLineF(rect.bottomLeft(), rect.bottomRight()), &bottom_pt);

	double const top_x = qBound(rect.left(), top_pt.x(), rect.right());
	double const bottom_x = qBound(rect.left(), bottom_pt.x(), rect.right());

	return QLineF(QPointF(top_x, rect.top()), QPointF(bottom_x, rect.bottom()));
}

QPointF
ImageView::handlePosition(int id) const
{
	QLineF const line(widgetSplitLine());
	return id == 0 ? line.p1() : line.p2();
}

void
ImageView::handleMoveRequest(int const id, QPointF const& pos)
{
	QRectF const valid_area(getOccupiedWidgetRect());
	QPointF const bound_widget_pos(
		qBound(valid_area.left(), pos.x(), valid_area.right()), pos.y()
	);

	double const x = widgetToVirtual().map(bound_widget_pos).x();

	QLineF virt_line(virtualSplitLine());
	if (id == 0) {
		virt_line.setP1(QPointF(x, virt_line.p1().y()));
	} else {
		virt_line.setP2(QPointF(x, virt_line.p2().y()));
	}

	m_virtLayout = PageLayout(m_virtLayout.type(), virt_line);
	update();
}

QLineF
ImageView::linePosition() const
{
	return widgetSplitLine();
}

void
ImageView::lineMoveRequest(QLineF line)
{
	// Intersect with top and bottom.
	QPointF p_top;
	QPointF p_bottom;
	QRectF const valid_area(getOccupiedWidgetRect());
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

void
ImageView::dragFinished()
{
	// When a handle is being dragged, the other handle is displayed not
	// at the edge of the widget widget but at the edge of the image.
	// That means we have to redraw once dragging is over.
	// BTW, the only reason for displaying handles at widget's edges
	// is to make them visible and accessible for dragging.
	update();
	emit pageLayoutSetLocally(m_virtLayout);
}

} // namespace page_split
