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
#include "Proximity.h"
#include "PageId.h"
#include "ProjectPages.h"
#include <QPainter>
#include <QPen>
#include <QBrush>
#include <QColor>
#include <QtGlobal>
#include <QDebug>
#ifndef Q_MOC_RUN
#include <boost/bind.hpp>
#include <boost/foreach.hpp>
#endif
#include <algorithm>

namespace page_split
{

ImageView::ImageView(
	QImage const& image, QImage const& downscaled_image,
	ImageTransformation const& xform, PageLayout const& layout,
	IntrusivePtr<ProjectPages> const& pages, ImageId const& image_id,
	bool left_half_removed, bool right_half_removed)
:	ImageViewBase(
		image, downscaled_image,
		ImagePresentation(xform.transform(), xform.resultingPreCropArea())
	),
	m_ptrPages(pages),
	m_imageId(image_id),
	m_leftUnremoveButton(boost::bind(&ImageView::leftPageCenter, this)),
	m_rightUnremoveButton(boost::bind(&ImageView::rightPageCenter, this)),
	m_dragHandler(*this),
	m_zoomHandler(*this),
	m_handlePixmap(":/icons/aqua-sphere.png"),
	m_virtLayout(layout),
	m_leftPageRemoved(left_half_removed),
	m_rightPageRemoved(right_half_removed)
{
	setMouseTracking(true);

	m_leftUnremoveButton.setClickCallback(boost::bind(&ImageView::unremoveLeftPage, this));
	m_rightUnremoveButton.setClickCallback(boost::bind(&ImageView::unremoveRightPage, this));

	if (m_leftPageRemoved) {
		makeLastFollower(m_leftUnremoveButton);
	}
	if (m_rightPageRemoved) {
		makeLastFollower(m_rightUnremoveButton);
	}

	setupCuttersInteraction();

	rootInteractionHandler().makeLastFollower(*this);
	rootInteractionHandler().makeLastFollower(m_dragHandler);
	rootInteractionHandler().makeLastFollower(m_zoomHandler);
}

ImageView::~ImageView()
{
}

void
ImageView::setupCuttersInteraction()
{
	QString const tip(tr("Drag the line or the handles."));
	double const hit_radius = std::max<double>(0.5 * m_handlePixmap.width(), 15.0);
	int const num_cutters = m_virtLayout.numCutters();
	for (int i = 0; i < num_cutters; ++i) { // Loop over lines.
		m_lineInteractors[i].setObject(&m_lineSegments[0]);

		for (int j = 0; j < 2; ++j) { // Loop over handles.
			m_handles[i][j].setHitRadius(hit_radius);
			m_handles[i][j].setPositionCallback(
				boost::bind(&ImageView::handlePosition, this, i, j)
			);
			m_handles[i][j].setMoveRequestCallback(
				boost::bind(&ImageView::handleMoveRequest, this, i, j, _1)
			);
			m_handles[i][j].setDragFinishedCallback(
				boost::bind(&ImageView::dragFinished, this)
			);

			m_handleInteractors[i][j].setObject(&m_handles[i][j]);
			m_handleInteractors[i][j].setProximityStatusTip(tip);
			makeLastFollower(m_handleInteractors[i][j]);
		}
		
		m_lineSegments[i].setPositionCallback(
			boost::bind(&ImageView::linePosition, this, i)
		);
		m_lineSegments[i].setMoveRequestCallback(
			boost::bind(&ImageView::lineMoveRequest, this, i, _1)
		);
		m_lineSegments[i].setDragFinishedCallback(
			boost::bind(&ImageView::dragFinished, this)
		);

		m_lineInteractors[i].setObject(&m_lineSegments[i]);
		m_lineInteractors[i].setProximityCursor(Qt::SplitHCursor);
		m_lineInteractors[i].setInteractionCursor(Qt::SplitHCursor);
		m_lineInteractors[i].setProximityStatusTip(tip);
		makeLastFollower(m_lineInteractors[i]);
	}

	// Turn off cutters we don't need anymore.
	for (int i = num_cutters; i < 2; ++i) {
		for (int j = 0; j < 2; ++j) {
			m_handleInteractors[i][j].unlink();
		}
		m_lineInteractors[i].unlink();
	}
}

void
ImageView::pageLayoutSetExternally(PageLayout const& layout)
{
	m_virtLayout = layout;
	setupCuttersInteraction();
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
		case PageLayout::SINGLE_PAGE_CUT:
			painter.setBrush(QColor(0, 0, 255, 50));
			painter.drawPolygon(m_virtLayout.singlePageOutline());
			break;
		case PageLayout::TWO_PAGES:
			painter.setBrush(m_leftPageRemoved ? QColor(0, 0, 0, 80) : QColor(0, 0, 255, 50));
			painter.drawPolygon(m_virtLayout.leftPageOutline());
			painter.setBrush(m_rightPageRemoved ? QColor(0, 0, 0, 80) : QColor(255, 0, 0, 50));
			painter.drawPolygon(m_virtLayout.rightPageOutline());
			break;
	}

	painter.setRenderHint(QPainter::Antialiasing, true);
	painter.setWorldTransform(QTransform());

	QPen pen(QColor(0, 0, 255));
	pen.setCosmetic(true);
	pen.setWidth(2);
	painter.setPen(pen);
	painter.setBrush(Qt::NoBrush);

	int const num_cutters = m_virtLayout.numCutters();
	for (int i = 0; i < num_cutters; ++i) {
		QLineF const cutter(widgetCutterLine(i));
		painter.drawLine(cutter);
		
		QRectF rect(m_handlePixmap.rect());

		rect.moveCenter(cutter.p1());
		painter.drawPixmap(rect.topLeft(), m_handlePixmap);

		rect.moveCenter(cutter.p2());
		painter.drawPixmap(rect.topLeft(), m_handlePixmap);
	}
}

PageLayout
ImageView::widgetLayout() const
{
	return m_virtLayout.transformed(virtualToWidget());
}

QLineF
ImageView::widgetCutterLine(int const line_idx) const
{
	QRectF const widget_rect(virtualToWidget().mapRect(virtualDisplayRect()));
	QRectF reduced_widget_rect(reducedWidgetArea());
	reduced_widget_rect.setLeft(widget_rect.left());
	reduced_widget_rect.setRight(widget_rect.right());
	// The reason we restore original left and right boundaries is that
	// we want to allow cutter handles to go off-screen horizontally
	// but not vertically.

	QLineF line(
		customInscribedCutterLine(
			widgetLayout().cutterLine(line_idx), reduced_widget_rect
		)
	);

	if (m_handleInteractors[line_idx][1].interactionInProgress(interactionState())) {
		line.setP1(virtualToWidget().map(virtualCutterLine(line_idx).p1()));
	} else if (m_handleInteractors[line_idx][0].interactionInProgress(interactionState())) {
		line.setP2(virtualToWidget().map(virtualCutterLine(line_idx).p2()));
	}

	return line;
}

QLineF
ImageView::virtualCutterLine(int line_idx) const
{
	QRectF const virt_rect(virtualDisplayRect());

	QRectF widget_rect(virtualToWidget().mapRect(virt_rect));
	double const delta = 0.5 * m_handlePixmap.width();
	widget_rect.adjust(0.0, delta, 0.0, -delta);

	QRectF reduced_virt_rect(widgetToVirtual().mapRect(widget_rect));
	reduced_virt_rect.setLeft(virt_rect.left());
	reduced_virt_rect.setRight(virt_rect.right());
	// The reason we restore original left and right boundaries is that
	// we want to allow cutter handles to go off-screen horizontally
	// but not vertically.
	
	return customInscribedCutterLine(
		m_virtLayout.cutterLine(line_idx), reduced_virt_rect
	);
}

QRectF
ImageView::reducedWidgetArea() const
{
	qreal const delta = 0.5 * m_handlePixmap.width();
	return getOccupiedWidgetRect().adjusted(0.0, delta, 0.0, -delta);
}

/**
 * This implementation differs from PageLayout::inscribedCutterLine() in that
 * it forces the endpoints to lie on the top and bottom boundary lines.
 * Line's angle may change as a result.
 */
QLineF
ImageView::customInscribedCutterLine(QLineF const& line, QRectF const& rect)
{
	if (line.p1().y() == line.p2().y()) {
		// This should not happen, but if it does, we need to handle it gracefully.
		qreal middle_x = 0.5 * (line.p1().x() + line.p2().x());
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
ImageView::handlePosition(int line_idx, int handle_idx) const
{
	QLineF const line(widgetCutterLine(line_idx));
	return handle_idx == 0 ? line.p1() : line.p2();
}

void
ImageView::handleMoveRequest(int line_idx, int handle_idx, QPointF const& pos)
{
	QRectF const valid_area(getOccupiedWidgetRect());
	double const x = qBound(valid_area.left(), pos.x(), valid_area.right());
	QPointF const wpt(x, handle_idx == 0 ? valid_area.top() : valid_area.bottom());
	QPointF const vpt(widgetToVirtual().map(wpt));

	QLineF virt_line(virtualCutterLine(line_idx));
	if (handle_idx == 0) {
		virt_line.setP1(vpt);
	} else {
		virt_line.setP2(vpt);
	}

	m_virtLayout.setCutterLine(line_idx, virt_line);
	update();
}

QLineF
ImageView::linePosition(int line_idx) const
{
	return widgetCutterLine(line_idx);
}

void
ImageView::lineMoveRequest(int line_idx, QLineF line)
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

	m_virtLayout.setCutterLine(line_idx, widgetToVirtual().map(line));
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

QPointF
ImageView::leftPageCenter() const
{
	QRectF left_rect(m_virtLayout.leftPageOutline().boundingRect());
	QRectF right_rect(m_virtLayout.rightPageOutline().boundingRect());

	double const x_mid = 0.5 * (left_rect.right() + right_rect.left());
	left_rect.setRight(x_mid);
	right_rect.setLeft(x_mid);

	return virtualToWidget().map(left_rect.center());
}

QPointF
ImageView::rightPageCenter() const
{
	QRectF left_rect(m_virtLayout.leftPageOutline().boundingRect());
	QRectF right_rect(m_virtLayout.rightPageOutline().boundingRect());

	double const x_mid = 0.5 * (left_rect.right() + right_rect.left());
	left_rect.setRight(x_mid);
	right_rect.setLeft(x_mid);

	return virtualToWidget().map(right_rect.center());
}

void
ImageView::unremoveLeftPage()
{
	PageInfo page_info(
		m_ptrPages->unremovePage(PageId(m_imageId, PageId::LEFT_PAGE))
	);
	m_leftUnremoveButton.unlink();
	m_leftPageRemoved = false;
	
	update();

	// We need invalidateThumbnail(PageInfo) rather than (PageId),
	// as we are updating page removal status.
	page_info.setId(PageId(m_imageId, PageId::SINGLE_PAGE));
	emit invalidateThumbnail(page_info);
}

void
ImageView::unremoveRightPage()
{
	PageInfo page_info(
		m_ptrPages->unremovePage(PageId(m_imageId, PageId::RIGHT_PAGE))
	);
	m_rightUnremoveButton.unlink();
	m_rightPageRemoved = false;
	
	update();
	
	// We need invalidateThumbnail(PageInfo) rather than (PageId),
	// as we are updating page removal status.
	page_info.setId(PageId(m_imageId, PageId::SINGLE_PAGE));
	emit invalidateThumbnail(page_info);
}

} // namespace page_split
