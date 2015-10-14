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
#include "InteractionState.h"
#include "imageproc/Constants.h"
#include <QRect>
#include <QSizeF>
#include <QPainter>
#include <QWheelEvent>
#include <QVector>
#include <QLineF>
#include <QScrollBar>
#include <QStyle>
#include <Qt>
#ifndef Q_MOC_RUN
#include <boost/bind.hpp>
#endif
#include <algorithm>
#include <math.h>

namespace deskew
{

double const ImageView::m_maxRotationDeg = 45.0;
double const ImageView::m_maxRotationSin = sin(
	m_maxRotationDeg * imageproc::constants::DEG2RAD
);
int const ImageView::m_cellSize = 20;

ImageView::ImageView(
	QImage const& image, QImage const& downscaled_image,
	ImageTransformation const& xform)
:	ImageViewBase(
		image, downscaled_image,
		ImagePresentation(xform.transform(), xform.resultingPreCropArea())
	),
	m_handlePixmap(":/icons/aqua-sphere.png"),
	m_dragHandler(*this),
	m_zoomHandler(*this),
	m_xform(xform)
{
	setMouseTracking(true);

	interactionState().setDefaultStatusTip(
		tr("Use Ctrl+Wheel to rotate or Ctrl+Shift+Wheel for finer rotation.")
	);

	QString const tip(tr("Drag this handle to rotate the image."));
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

	m_zoomHandler.setFocus(ZoomHandler::CENTER);

	rootInteractionHandler().makeLastFollower(*this);
	rootInteractionHandler().makeLastFollower(m_dragHandler);
	rootInteractionHandler().makeLastFollower(m_zoomHandler);
}

ImageView::~ImageView()
{
}

void
ImageView::manualDeskewAngleSetExternally(double const degrees)
{
	if (m_xform.postRotation() == degrees) {
		return;
	}
	
	m_xform.setPostRotation(degrees);
	updateTransform(ImagePresentation(m_xform.transform(), m_xform.resultingPreCropArea()));
}

void
ImageView::onPaint(QPainter& painter, InteractionState const& interaction)
{
	painter.setWorldMatrixEnabled(false);
	painter.setRenderHints(QPainter::Antialiasing, false);

	double const w = maxViewportRect().width();
	double const h = maxViewportRect().height();
	QPointF const center(getImageRotationOrigin());

	// Draw the semi-transparent grid.
	QPen pen(QColor(0, 0, 255, 90));
	pen.setCosmetic(true);
	pen.setWidth(1);
	painter.setPen(pen);
	QVector<QLineF> lines;
	for (double y = center.y(); (y -= m_cellSize) > 0.0;) {
		lines.push_back(QLineF(0.5, y, w - 0.5, y));
	}
	for (double y = center.y(); (y += m_cellSize) < h;) {
		lines.push_back(QLineF(0.5, y, w - 0.5, y));
	}
	for (double x = center.x(); (x -= m_cellSize) > 0.0;) {
		lines.push_back(QLineF(x, 0.5, x, h - 0.5));
	}
	for (double x = center.x(); (x += m_cellSize) < w;) {
		lines.push_back(QLineF(x, 0.5, x, h - 0.5));
	}
	painter.drawLines(lines);

	// Draw the horizontal and vertical line crossing at the center.
	pen.setColor(QColor(0, 0, 255));
	painter.setPen(pen);
	painter.setBrush(Qt::NoBrush);
	painter.drawLine(
		QPointF(0.5, center.y()),
		QPointF(w - 0.5, center.y())
	);
	painter.drawLine(
		QPointF(center.x(), 0.5),
		QPointF(center.x(), h - 0.5)
	);

	// Draw the rotation arcs.
	// Those will look like this (  )
	QRectF const arc_square(getRotationArcSquare());

	painter.setRenderHints(QPainter::Antialiasing, true);
	pen.setWidthF(1.5);
	painter.setPen(pen);
	painter.setBrush(Qt::NoBrush);
	painter.drawArc(
		arc_square,
		qRound(16 * -m_maxRotationDeg),
		qRound(16 * 2 * m_maxRotationDeg)
	);
	painter.drawArc(
		arc_square,
		qRound(16 * (180 - m_maxRotationDeg)),
		qRound(16 * 2 * m_maxRotationDeg)
	);

	std::pair<QPointF, QPointF> const handles(
		getRotationHandles(arc_square)
	);

	QRectF rect(m_handlePixmap.rect());
	rect.moveCenter(handles.first);
	painter.drawPixmap(rect.topLeft(), m_handlePixmap);
	rect.moveCenter(handles.second);
	painter.drawPixmap(rect.topLeft(), m_handlePixmap);
}

void
ImageView::onWheelEvent(QWheelEvent* event, InteractionState& interaction)
{
	if (interaction.captured()) {
		return;
	}

	double degree_fraction = 0;

	if (event->modifiers() == Qt::ControlModifier) {
		degree_fraction = 0.1;
	} else if (event->modifiers() == (Qt::ControlModifier|Qt::ShiftModifier)) {
		degree_fraction = 0.05;
	} else {
		return;
	}
	
	event->accept();
	double const delta = degree_fraction * event->delta() / 120;
	double angle_deg = m_xform.postRotation() - delta;
	angle_deg = qBound(-m_maxRotationDeg, angle_deg, m_maxRotationDeg);
	if (angle_deg == m_xform.postRotation()) {
		return;
	}

	m_xform.setPostRotation(angle_deg);
	updateTransformPreservingScale(
		ImagePresentation(m_xform.transform(), m_xform.resultingPreCropArea())
	);
	emit manualDeskewAngleSet(m_xform.postRotation());
}

QPointF
ImageView::handlePosition(int idx) const
{
	std::pair<QPointF, QPointF> const handles(getRotationHandles(getRotationArcSquare()));
	if (idx == 0) {
		return handles.first;
	} else {
		return handles.second;
	}
}

void
ImageView::handleMoveRequest(int idx, QPointF const& pos)
{
	QRectF const arc_square(getRotationArcSquare());
	double const arc_radius = 0.5 * arc_square.width();
	double const abs_y = pos.y();
	double rel_y = abs_y - arc_square.center().y();
	rel_y = qBound(-arc_radius, rel_y, arc_radius);

	double angle_rad = asin(rel_y / arc_radius);
	if (idx == 0) {
		angle_rad = -angle_rad;
	}
	double angle_deg = angle_rad * imageproc::constants::RAD2DEG;
	angle_deg = qBound(-m_maxRotationDeg, angle_deg, m_maxRotationDeg);
	if (angle_deg == m_xform.postRotation()) {
		return;
	}

	m_xform.setPostRotation(angle_deg);
	updateTransformPreservingScale(ImagePresentation(m_xform.transform(), m_xform.resultingPreCropArea()));
}

void
ImageView::dragFinished()
{
	emit manualDeskewAngleSet(m_xform.postRotation());
}

/**
 * Get the point at the center of the widget, in widget coordinates.
 * The point may be adjusted to to ensure it's at the center of a pixel.
 */
QPointF
ImageView::getImageRotationOrigin() const
{
	QRectF const viewport_rect(maxViewportRect());
	return QPointF(
		floor(0.5 * viewport_rect.width()) + 0.5,
		floor(0.5 * viewport_rect.height()) + 0.5
	);
}

/**
 * Get the square in widget coordinates where two rotation arcs will be drawn.
 */
QRectF
ImageView::getRotationArcSquare() const
{
	double const h_margin = 0.5 * m_handlePixmap.width()
		+ verticalScrollBar()->style()->pixelMetric(QStyle::PM_ScrollBarExtent, 0, verticalScrollBar());
	double const v_margin = 0.5 * m_handlePixmap.height()
		+ horizontalScrollBar()->style()->pixelMetric(QStyle::PM_ScrollBarExtent, 0, horizontalScrollBar());

	QRectF reduced_screen_rect(maxViewportRect());
	reduced_screen_rect.adjust(h_margin, v_margin, -h_margin, -v_margin);

	QSizeF arc_size(1.0, m_maxRotationSin);
	arc_size.scale(reduced_screen_rect.size(), Qt::KeepAspectRatio);
	arc_size.setHeight(arc_size.width());

	QRectF arc_square(QPointF(0, 0), arc_size);
	arc_square.moveCenter(reduced_screen_rect.center());

	return arc_square;
}

std::pair<QPointF, QPointF>
ImageView::getRotationHandles(QRectF const& arc_square) const
{
	double const rot_sin = m_xform.postRotationSin();
	double const rot_cos = m_xform.postRotationCos();
	double const arc_radius = 0.5 * arc_square.width();
	QPointF const arc_center(arc_square.center());
	QPointF left_handle(-rot_cos * arc_radius, -rot_sin * arc_radius);
	left_handle += arc_center;
	QPointF right_handle(rot_cos * arc_radius, rot_sin * arc_radius);
	right_handle += arc_center;
	return std::make_pair(left_handle, right_handle);
}

} // namespace deskew
