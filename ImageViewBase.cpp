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

#include "ImageViewBase.h"
#include "ImageViewBase.h.moc"
#include "NonCopyable.h"
#include "ImagePresentation.h"
#include "OpenGLSupport.h"
#include "PixmapRenderer.h"
#include "BackgroundExecutor.h"
#include "Dpm.h"
#include "Dpi.h"
#include "ScopedIncDec.h"
#include "imageproc/PolygonUtils.h"
#include "imageproc/Transform.h"
#include "config.h"
#include <QScrollBar>
#include <QPointer>
#include <QAtomicInt>
#include <QPaintEngine>
#include <QPainter>
#include <QPainterPath>
#include <QBrush>
#include <QLineF>
#include <QPolygonF>
#include <QPalette>
#include <QMouseEvent>
#include <QWheelEvent>
#include <QResizeEvent>
#include <QStatusTipEvent>
#include <QApplication>
#include <QSettings>
#include <QVariant>
#include <Qt>
#include <QDebug>
#include <algorithm>
#include <assert.h>
#include <math.h>

#ifdef ENABLE_OPENGL
#include <QGLWidget>
#include <QGLFormat>
#endif

using namespace imageproc;

class ImageViewBase::HqTransformTask :
	public AbstractCommand0<IntrusivePtr<AbstractCommand0<void> > >,
	public QObject
{
	DECLARE_NON_COPYABLE(HqTransformTask)
public:
	HqTransformTask(
		ImageViewBase* image_view,
		QImage const& image, QTransform const& xform,
		QSize const& target_size);
	
	void cancel() { m_ptrResult->cancel(); }
	
	bool const isCancelled() const { return m_ptrResult->isCancelled(); }
	
	virtual IntrusivePtr<AbstractCommand0<void> > operator()();
private:
	class Result : public AbstractCommand0<void>
	{
	public:
		Result(ImageViewBase* image_view);
		
		void setData(QPoint const& origin, QImage const& hq_image);
		
		void cancel() { m_cancelFlag.fetchAndStoreRelaxed(1); }
		
		bool isCancelled() const { return m_cancelFlag.fetchAndAddRelaxed(0) != 0; }
		
		virtual void operator()();
	private:
		QPointer<ImageViewBase> m_ptrImageView;
		QPoint m_origin;
		QImage m_hqImage;
		mutable QAtomicInt m_cancelFlag;
	};
	
	IntrusivePtr<Result> m_ptrResult;
	QImage m_image;
	QTransform m_xform;
	QSize m_targetSize;
};


/**
 * \brief Temporarily adjust the widget focal point, then change it back.
 *
 * When adjusting and restoring the widget focal point, the pixmap
 * focal point is recalculated accordingly.
 */
class ImageViewBase::TempFocalPointAdjuster
{
public:
	/**
	 * Change the widget focal point to obj.centeredWidgetFocalPoint().
	 */
	TempFocalPointAdjuster(ImageViewBase& obj);
	
	/**
	 * Change the widget focal point to \p temp_widget_fp
	 */
	TempFocalPointAdjuster(ImageViewBase& obj, QPointF temp_widget_fp);
	
	/**
	 * Restore the widget focal point.
	 */
	~TempFocalPointAdjuster();
private:
	ImageViewBase& m_rObj;
	QPointF m_origWidgetFP;
};


class ImageViewBase::TransformChangeWatcher
{
public:
	TransformChangeWatcher(ImageViewBase& owner);

	~TransformChangeWatcher();
private:
	ImageViewBase& m_rOwner;
	QTransform m_imageToVirtual;
	QTransform m_virtualToWidget;
	QRectF m_virtualDisplayArea;
};


ImageViewBase::ImageViewBase(
	QImage const& image, ImagePixmapUnion const& downscaled_version,
	ImagePresentation const& presentation, Margins const& margins)
:	m_image(image),
	m_virtualImageCropArea(presentation.cropArea()),
	m_virtualDisplayArea(presentation.displayArea()),
	m_imageToVirtual(presentation.transform()),
	m_virtualToImage(presentation.transform().inverted()),
	m_lastMaximumViewportSize(maximumViewportSize()),
	m_margins(margins),
	m_zoom(1.0),
	m_transformChangeWatchersActive(0),
	m_ignoreScrollEvents(0),
	m_ignoreResizeEvents(0),
	m_hqTransformEnabled(true)
{
#ifdef ENABLE_OPENGL
	if (QSettings().value("settings/use_3d_acceleration", false) != false) {
		if (OpenGLSupport::supported()) {
			QGLFormat format;
			format.setSampleBuffers(true);
			format.setStencil(true);
			format.setAlpha(true);
			format.setRgba(true);
			format.setDepth(false);

			// Most of hardware refuses to work for us with direct rendering enabled.
			format.setDirectRendering(false);

			setViewport(new QGLWidget(format));
		}
	}
#endif

	setFrameShape(QFrame::NoFrame);
	viewport()->setFocusPolicy(Qt::WheelFocus);

	if (downscaled_version.isNull()) {
		m_pixmap = QPixmap::fromImage(createDownscaledImage(image));
	} else if (downscaled_version.pixmap().isNull()) {
		m_pixmap = QPixmap::fromImage(downscaled_version.image());
	} else {
		m_pixmap = downscaled_version.pixmap();
	}
	
	m_pixmapToImage.scale(
		(double)m_image.width() / m_pixmap.width(),
		(double)m_image.height() / m_pixmap.height()
	);
	
	m_widgetFocalPoint = centeredWidgetFocalPoint();
	m_pixmapFocalPoint = m_virtualToImage.map(virtualDisplayRect().center());
	
	m_timer.setSingleShot(true);
	m_timer.setInterval(150); // msec
	connect(
		&m_timer, SIGNAL(timeout()),
		this, SLOT(initiateBuildingHqVersion())
	);
	
	updateWidgetTransformAndFixFocalPoint(CENTER_IF_FITS);

	interactionState().setDefaultStatusTip(
		tr("Use the mouse wheel or +/- to zoom.  When zoomed, dragging is possible.")
	);
	ensureStatusTip(interactionState().statusTip());

	connect(horizontalScrollBar(), SIGNAL(sliderReleased()), SLOT(updateScrollBars()));
	connect(verticalScrollBar(), SIGNAL(sliderReleased()), SLOT(updateScrollBars()));
	connect(horizontalScrollBar(), SIGNAL(valueChanged(int)), SLOT(reactToScrollBars()));
	connect(verticalScrollBar(), SIGNAL(valueChanged(int)), SLOT(reactToScrollBars()));
}

ImageViewBase::~ImageViewBase()
{
}

void
ImageViewBase::hqTransformSetEnabled(bool const enabled)
{
	if (!enabled && m_hqTransformEnabled) {
		// Turning off.
		m_hqTransformEnabled = false;
		if (m_ptrHqTransformTask.get()) {
			m_ptrHqTransformTask->cancel();
			m_ptrHqTransformTask.reset();
		}
		if (!m_hqPixmap.isNull()) {
			m_hqPixmap = QPixmap();
			update();
		}
	} else if (enabled && !m_hqTransformEnabled) {
		// Turning on.
		m_hqTransformEnabled = true;
		update();
	}
}

QImage
ImageViewBase::createDownscaledImage(QImage const& image)
{
	assert(!image.isNull());
	
	// Original and downscaled DPM.
	Dpm const o_dpm(image);
	Dpm const d_dpm(Dpi(300, 300));
	
	int const o_w = image.width();
	int const o_h = image.height();
	
	int d_w = o_w * d_dpm.horizontal() / o_dpm.horizontal();
	int d_h = o_h * d_dpm.vertical() / o_dpm.vertical();
	d_w = qBound(1, d_w, o_w);
	d_h = qBound(1, d_h, o_h);
	
	if (d_w * 1.2 > o_w || d_h * 1.2 > o_h) {
		// Sizes are close - no point in downscaling.
		return image;
	}
	
	QTransform xform;
	xform.scale((double)d_w / o_w, (double)d_h / o_h);
	return transform(
		image, xform, QRect(0, 0, d_w, d_h),
		OutsidePixels::assumeColor(Qt::white)
	);
}

QRectF
ImageViewBase::maxViewportRect() const
{
	QRectF const viewport_rect(QPointF(0, 0), maximumViewportSize());
	QRectF r(viewport_rect);
	r.adjust(
		m_margins.left(), m_margins.top(),
		-m_margins.right(), -m_margins.bottom()
	);
	if (r.isEmpty()) {
		return QRectF(viewport_rect.center(), viewport_rect.center());
	}
	return r;
}

QRectF
ImageViewBase::dynamicViewportRect() const
{
	QRectF const viewport_rect(viewport()->rect());
	QRectF r(viewport_rect);
	r.adjust(
		m_margins.left(), m_margins.top(),
		-m_margins.right(), -m_margins.bottom()
	);
	if (r.isEmpty()) {
		return QRectF(viewport_rect.center(), viewport_rect.center());
	}
	return r;
}

QRectF
ImageViewBase::getOccupiedWidgetRect() const
{
	QRectF const widget_rect(m_virtualToWidget.mapRect(virtualDisplayRect()));
	return widget_rect.intersected(dynamicViewportRect());
}

void
ImageViewBase::setWidgetFocalPoint(QPointF const& widget_fp)
{
	setNewWidgetFP(widget_fp, /*update =*/true);
}

void
ImageViewBase::adjustAndSetWidgetFocalPoint(QPointF const& widget_fp)
{
	adjustAndSetNewWidgetFP(widget_fp, /*update=*/true);
}

void
ImageViewBase::setZoomLevel(double zoom)
{
	if (m_zoom != zoom) {
		m_zoom = zoom;
		updateWidgetTransform();
		update();
	}
}

void
ImageViewBase::moveTowardsIdealPosition(double const pixel_length)
{
	if (pixel_length <= 0) {
		// The name implies we are moving *towards* the ideal position.
		return;
	}

	QPointF const ideal_widget_fp(getIdealWidgetFocalPoint(CENTER_IF_FITS));
	if (ideal_widget_fp == m_widgetFocalPoint) {
		return;
	}

	QPointF vec(ideal_widget_fp - m_widgetFocalPoint);
	double const max_length = sqrt(vec.x() * vec.x() + vec.y() * vec.y());
	if (pixel_length >= max_length) {
		m_widgetFocalPoint = ideal_widget_fp;
	} else {
		vec *= pixel_length / max_length;
		m_widgetFocalPoint += vec;
	}

	updateWidgetTransform();
	update();
}

void
ImageViewBase::updateTransform(ImagePresentation const& presentation)
{
	TransformChangeWatcher const watcher(*this);
	TempFocalPointAdjuster const temp_fp(*this);

	m_imageToVirtual = presentation.transform();
	m_virtualToImage = m_imageToVirtual.inverted();
	m_virtualImageCropArea = presentation.cropArea();
	m_virtualDisplayArea = presentation.displayArea();

	updateWidgetTransform();
	update();
}

void
ImageViewBase::updateTransformAndFixFocalPoint(
	ImagePresentation const& presentation, FocalPointMode const mode)
{
	TransformChangeWatcher const watcher(*this);
	TempFocalPointAdjuster const temp_fp(*this);
	
	m_imageToVirtual = presentation.transform();
	m_virtualToImage = m_imageToVirtual.inverted();
	m_virtualImageCropArea = presentation.cropArea();
	m_virtualDisplayArea = presentation.displayArea();

	updateWidgetTransformAndFixFocalPoint(mode);
	update();
}

void
ImageViewBase::updateTransformPreservingScale(ImagePresentation const& presentation)
{
	TransformChangeWatcher const watcher(*this);
	TempFocalPointAdjuster const temp_fp(*this);
	
	// An arbitrary line in image coordinates.
	QLineF const image_line(0.0, 0.0, 1.0, 1.0);
	
	QLineF const widget_line_before(
		(m_imageToVirtual * m_virtualToWidget).map(image_line)
	);
	
	m_imageToVirtual = presentation.transform();
	m_virtualToImage = m_imageToVirtual.inverted();
	m_virtualImageCropArea = presentation.cropArea();
	m_virtualDisplayArea = presentation.displayArea();

	updateWidgetTransform();
	
	QLineF const widget_line_after(
		(m_imageToVirtual * m_virtualToWidget).map(image_line)
	);
	
	m_zoom *= widget_line_before.length() / widget_line_after.length();
	updateWidgetTransform();
	
	update();
}

void
ImageViewBase::ensureStatusTip(QString const& status_tip)
{
	QString const cur_status_tip(statusTip());
	if (cur_status_tip.constData() == status_tip.constData()) {
		return;
	}
	if (cur_status_tip == status_tip) {
		return;
	}
	
	viewport()->setStatusTip(status_tip);
	
	if (viewport()->underMouse()) {
		// Note that setStatusTip() alone is not enough,
		// as it's only taken into account when the mouse
		// enters the widget.
		// Also note that we use postEvent() rather than sendEvent(),
		// because sendEvent() may immediately process other events.
		QApplication::postEvent(viewport(), new QStatusTipEvent(status_tip));
	}
}

void
ImageViewBase::paintEvent(QPaintEvent* event)
{
	QPainter painter(viewport());
	painter.save();

	// On X11 (except with OpenGL), SmoothPixmapTransform is too slow, so don't enable it.
	bool smooth_pixmap_ok = true;
#if defined(Q_WS_X11)
	smooth_pixmap_ok = viewport()->inherits("QGLWidget");
#endif
	if (smooth_pixmap_ok) {
		double const xscale = m_virtualToWidget.m11();

		// Width of a source pixel in mm, as it's displayed on screen.
		double const pixel_width = widthMM() * xscale / width();

		// Disable antialiasing for large zoom levels.
		painter.setRenderHint(QPainter::SmoothPixmapTransform, pixel_width < 0.5);
	}

	if (validateHqPixmap()) {
		// HQ pixmap maps one to one to screen pixels, so antialiasing is not necessary.
		painter.setRenderHint(QPainter::SmoothPixmapTransform, false);
		painter.drawPixmap(m_hqPixmapPos, m_hqPixmap);
	} else {
		scheduleHqVersionRebuild();

		painter.setWorldTransform(
			m_pixmapToImage * m_imageToVirtual * m_virtualToWidget
		);
		PixmapRenderer::drawPixmap(painter, m_pixmap);
	}

	painter.setRenderHints(QPainter::Antialiasing, true);
	painter.setWorldMatrixEnabled(false);

	// Cover parts of the image that should not be visible with background.
	// Note that because of Qt::WA_OpaquePaintEvent attribute, we need
	// to paint the whole widget, which we do here.

	QPolygonF const image_area(
		PolygonUtils::round(
			m_virtualToWidget.map(
				m_imageToVirtual.map(QRectF(m_image.rect()))
			)
		)
	);
	QPolygonF const crop_area(
		PolygonUtils::round(m_virtualToWidget.map(m_virtualImageCropArea))
	);

	QPolygonF const intersected_area(
		PolygonUtils::round(image_area.intersected(crop_area))
	);

	QPainterPath intersected_path;
	intersected_path.addPolygon(intersected_area);

	QPainterPath containing_path;
	containing_path.addRect(viewport()->rect());

	QBrush const brush(palette().color(QPalette::Window));
	QPen pen(brush, 1.0);
	pen.setCosmetic(true);

	// By using a pen with the same color as the brush, we essentially
	// expanding the area we are going to draw.  It's necessary because
	// XRender doesn't provide subpixel accuracy.

	painter.setPen(pen);
	painter.setBrush(brush);
	painter.drawPath(containing_path.subtracted(intersected_path));

	painter.restore();

	painter.setWorldTransform(m_virtualToWidget);

	m_interactionState.resetProximity();
	if (!m_interactionState.captured()) {
		m_rootInteractionHandler.proximityUpdate(
			QPointF(0.5, 0.5) + mapFromGlobal(QCursor::pos()), m_interactionState
		);
		updateStatusTipAndCursor();
	}

	m_rootInteractionHandler.paint(painter, m_interactionState);
	maybeQueueRedraw();
}

void
ImageViewBase::keyPressEvent(QKeyEvent* event)
{
	event->setAccepted(false);
	m_rootInteractionHandler.keyPressEvent(event, m_interactionState);
	event->setAccepted(true);
	updateStatusTipAndCursor();
	maybeQueueRedraw();
}

void
ImageViewBase::keyReleaseEvent(QKeyEvent* event)
{
	event->setAccepted(false);
	m_rootInteractionHandler.keyReleaseEvent(event, m_interactionState);
	event->setAccepted(true);
	updateStatusTipAndCursor();
	maybeQueueRedraw();
}

void
ImageViewBase::mousePressEvent(QMouseEvent* event)
{
	m_interactionState.resetProximity();
	if (!m_interactionState.captured()) {
		m_rootInteractionHandler.proximityUpdate(
			QPointF(0.5, 0.5) + event->pos(), m_interactionState
		);
	}

	event->setAccepted(false);
	m_rootInteractionHandler.mousePressEvent(event, m_interactionState);
	event->setAccepted(true);
	updateStatusTipAndCursor();
	void maybeQueueRedraw();
}

void
ImageViewBase::mouseReleaseEvent(QMouseEvent* event)
{
	m_interactionState.resetProximity();
	if (!m_interactionState.captured()) {
		m_rootInteractionHandler.proximityUpdate(
			QPointF(0.5, 0.5) + event->pos(), m_interactionState
		);
	}

	event->setAccepted(false);
	m_rootInteractionHandler.mouseReleaseEvent(event, m_interactionState);
	event->setAccepted(true);
	updateStatusTipAndCursor();
	maybeQueueRedraw();
}

void
ImageViewBase::mouseMoveEvent(QMouseEvent* event)
{
	m_interactionState.resetProximity();
	if (!m_interactionState.captured()) {
		m_rootInteractionHandler.proximityUpdate(
			QPointF(0.5, 0.5) + event->pos(), m_interactionState
		);
	}

	event->setAccepted(false);
	m_rootInteractionHandler.mouseMoveEvent(event, m_interactionState);
	event->setAccepted(true);
	updateStatusTipAndCursor();
	maybeQueueRedraw();
}

void
ImageViewBase::wheelEvent(QWheelEvent* event)
{
	event->setAccepted(false);
	m_rootInteractionHandler.wheelEvent(event, m_interactionState);
	event->setAccepted(true);
	updateStatusTipAndCursor();
	maybeQueueRedraw();
}

void
ImageViewBase::contextMenuEvent(QContextMenuEvent* event)
{
	event->setAccepted(false);
	m_rootInteractionHandler.contextMenuEvent(event, m_interactionState);
	event->setAccepted(true);
	updateStatusTipAndCursor();
	maybeQueueRedraw();
}

void
ImageViewBase::resizeEvent(QResizeEvent* event)
{
	QAbstractScrollArea::resizeEvent(event);

	if (m_ignoreResizeEvents) {
		return;
	}

	ScopedIncDec<int> const guard(m_ignoreScrollEvents);

	if (maximumViewportSize() != m_lastMaximumViewportSize) {
		m_lastMaximumViewportSize = maximumViewportSize();
		m_widgetFocalPoint = centeredWidgetFocalPoint();
		updateWidgetTransform();
	} else {
		TransformChangeWatcher const watcher(*this);
		TempFocalPointAdjuster const temp_fp(*this, QPointF(0, 0));
		updateTransformPreservingScale(
			ImagePresentation(m_imageToVirtual, m_virtualImageCropArea, m_virtualDisplayArea)
		);
	}
}

void
ImageViewBase::enterEvent(QEvent* event)
{
	viewport()->setFocus();
	QAbstractScrollArea::enterEvent(event);
}

/**
 * Called when any of the transformations change.
 */
void
ImageViewBase::transformChanged()
{
	updateScrollBars();
}

void
ImageViewBase::updateScrollBars()
{
	if (verticalScrollBar()->isSliderDown() || horizontalScrollBar()->isSliderDown()) {
		return;
	}

	ScopedIncDec<int> const guard1(m_ignoreScrollEvents);
	ScopedIncDec<int> const guard2(m_ignoreResizeEvents);

	QRectF const picture(m_virtualToWidget.mapRect(virtualDisplayRect()));
	QPointF const viewport_center(maxViewportRect().center());
	QPointF const picture_center(picture.center());
	QRectF viewport(maxViewportRect());

	// Introduction of one scrollbar will decrease the available size in
	// another direction, which may cause a scrollbar in that direction
	// to become necessary.  For this reason, we have a loop here.
	for (int i = 0; i < 2; ++i) {
		double const xval = picture_center.x();
		double xmin, xmax; // Minimum and maximum positions for picture center.
		if (picture_center.x() < viewport_center.x()) {
			xmin = std::min<double>(xval, viewport.right() - 0.5 * picture.width());
			xmax = std::max<double>(viewport_center.x(), viewport.left() + 0.5 * picture.width());
		} else {
			xmax = std::max<double>(xval, viewport.left() + 0.5 * picture.width());
			xmin = std::min<double>(viewport_center.x(), viewport.right() - 0.5 * picture.width());
		}

		double const yval = picture_center.y();
		double ymin, ymax; // Minimum and maximum positions for picture center.
		if (picture_center.y() < viewport_center.y()) {
			ymin = std::min<double>(yval, viewport.bottom() - 0.5 * picture.height());
			ymax = std::max<double>(viewport_center.y(), viewport.top() + 0.5 * picture.height());
		} else {
			ymax = std::max<double>(yval, viewport.top() + 0.5 * picture.height());
			ymin = std::min<double>(viewport_center.y(), viewport.bottom() - 0.5 * picture.height());
		}

		int const xrange = (int)ceil(xmax - xmin);
		int const yrange = (int)ceil(ymax - ymin);
		int const xfirst = 0;
		int const xlast = xrange - 1;
		int const yfirst = 0;
		int const ylast = yrange - 1;

		// We are going to map scrollbar coordinates to widget coordinates
		// of the central point of the display area using a linear function.
		// f(x) = ax + b

		// xmin = xa * xlast + xb
		// xmax = xa * xfirst + xb
		double const xa = (xfirst == xlast) ? 1 : (xmax - xmin) / (xfirst - xlast);
		double const xb = xmax - xa * xfirst;
		double const ya = (yfirst == ylast) ? 1 : (ymax - ymin) / (yfirst - ylast);
		double const yb = ymax - ya * yfirst;

		// Inverse transformation.
		// xlast = ixa * xmin + ixb
		// xfirst = ixa * xmax + ixb
		double const ixa = (xmax == xmin) ? 1 : (xfirst - xlast) / (xmax - xmin);
		double const ixb = xfirst - ixa * xmax;
		double const iya = (ymax == ymin) ? 1 : (yfirst - ylast) / (ymax - ymin);
		double const iyb = yfirst - iya * ymax;

		m_scrollTransform.setMatrix(xa, 0, 0, 0, ya, 0, xb, yb, 1);

		int const xcur = qRound(ixa * xval + ixb);
		int const ycur = qRound(iya * yval + iyb);

		horizontalScrollBar()->setRange(xfirst, xlast);
		verticalScrollBar()->setRange(yfirst, ylast);

		horizontalScrollBar()->setValue(xcur);
		verticalScrollBar()->setValue(ycur);

		horizontalScrollBar()->setPageStep(qRound(viewport.width()));
		verticalScrollBar()->setPageStep(qRound(viewport.height()));

		// XXX: a hack to force immediate update of viewport()->rect(),
		// which is used by dynamicViewportRect() below.
		// Note that it involves a resize event being sent not only to
		// the viewport, but for some reason also to the containing
		// QAbstractScrollArea, that is to this object.
		setHorizontalScrollBarPolicy(horizontalScrollBarPolicy());

		QRectF const old_viewport(viewport);
		viewport = dynamicViewportRect();
		if (viewport == old_viewport) {
			break;
		}
	}
}

void
ImageViewBase::reactToScrollBars()
{
	if (m_ignoreScrollEvents) {
		return;
	}

	TransformChangeWatcher const watcher(*this);

	QPointF const raw_position(
		horizontalScrollBar()->value(), verticalScrollBar()->value()
	);
	QPointF const new_fp(m_scrollTransform.map(raw_position));
	QPointF const old_fp(getWidgetFocalPoint());

	m_pixmapFocalPoint = m_virtualToImage.map(m_virtualDisplayArea.center());
	m_widgetFocalPoint = new_fp;
	updateWidgetTransform();

	setWidgetFocalPointWithoutMoving(old_fp);
}

/**
 * Updates m_virtualToWidget and m_widgetToVirtual.\n
 * To be called whenever any of the following is modified:
 * m_imageToVirt, m_widgetFocalPoint, m_pixmapFocalPoint, m_zoom.
 * Modifying both m_widgetFocalPoint and m_pixmapFocalPoint in a way
 * that doesn't cause image movement doesn't require calling this method.
 */
void
ImageViewBase::updateWidgetTransform()
{
	TransformChangeWatcher const watcher(*this);

	QRectF const virt_rect(virtualDisplayRect());
	QPointF const virt_origin(m_imageToVirtual.map(m_pixmapFocalPoint));
	QPointF const widget_origin(m_widgetFocalPoint);
	
	QSizeF zoom1_widget_size(virt_rect.size());
	zoom1_widget_size.scale(maxViewportRect().size(), Qt::KeepAspectRatio);
	
	double const zoom1_x = zoom1_widget_size.width() / virt_rect.width();
	double const zoom1_y = zoom1_widget_size.height() / virt_rect.height();
	
	QTransform xform;
	xform.translate(-virt_origin.x(), -virt_origin.y());
	xform *= QTransform().scale(zoom1_x * m_zoom, zoom1_y * m_zoom);
	xform *= QTransform().translate(widget_origin.x(), widget_origin.y());

	m_virtualToWidget = xform;
	m_widgetToVirtual = m_virtualToWidget.inverted();
}

/**
 * Updates m_virtualToWidget and m_widgetToVirtual and adjusts
 * the focal point if necessary.\n
 * To be called whenever m_imageToVirt is modified in such a way that
 * may invalidate the focal point.
 */
void
ImageViewBase::updateWidgetTransformAndFixFocalPoint(FocalPointMode const mode)
{
	TransformChangeWatcher const watcher(*this);

	// This must go before getIdealWidgetFocalPoint(), as it
	// recalculates m_virtualToWidget, that is used by
	// getIdealWidgetFocalPoint().
	updateWidgetTransform();
	
	QPointF const ideal_widget_fp(getIdealWidgetFocalPoint(mode));
	if (ideal_widget_fp != m_widgetFocalPoint) {
		m_widgetFocalPoint = ideal_widget_fp;
		updateWidgetTransform();
	}
}

/**
 * Returns a proposed value for m_widgetFocalPoint to minimize the
 * unused widget space.  Unused widget space indicates one or both
 * of the following:
 * \li The image is smaller than the display area.
 * \li Parts of the image are outside of the display area.
 *
 * \param mode If set to CENTER_IF_FITS, then the returned focal point
 *        will center the image if it completely fits into the widget.
 *        This works in horizontal and vertical directions independently.\n
 *        If \p mode is set to DONT_CENTER and the image completely fits
 *        the widget, then the returned focal point will cause a minimal
 *        move to force the whole image to be visible.
 *
 * In case there is no unused widget space, the returned focal point
 * is equal to the current focal point (m_widgetFocalPoint).  This works
 * in horizontal and vertical dimensions independently.
 */
QPointF
ImageViewBase::getIdealWidgetFocalPoint(FocalPointMode const mode) const
{
	// Widget rect reduced by margins.
	QRectF const display_area(maxViewportRect());
	
	// The virtual image rectangle in widget coordinates.
	QRectF const image_area(m_virtualToWidget.mapRect(virtualDisplayRect()));
	
	// Unused display space from each side.
	double const left_margin = image_area.left() - display_area.left();
	double const right_margin = display_area.right() - image_area.right();
	double const top_margin = image_area.top() - display_area.top();
	double const bottom_margin = display_area.bottom() - image_area.bottom();
	
	QPointF widget_focal_point(m_widgetFocalPoint);
	
	if (mode == CENTER_IF_FITS && left_margin + right_margin >= 0.0) {
		// Image fits horizontally, so center it in that direction
		// by equalizing its left and right margins.
		double const new_margins = 0.5 * (left_margin + right_margin);
		widget_focal_point.rx() += new_margins - left_margin;
	} else if (left_margin < 0.0 && right_margin > 0.0) {
		// Move image to the right so that either left_margin or
		// right_margin becomes zero, whichever requires less movement.
		double const movement = std::min(fabs(left_margin), fabs(right_margin));
		widget_focal_point.rx() += movement;
	} else if (right_margin < 0.0 && left_margin > 0.0) {
		// Move image to the left so that either left_margin or
		// right_margin becomes zero, whichever requires less movement.
		double const movement = std::min(fabs(left_margin), fabs(right_margin));
		widget_focal_point.rx() -= movement;
	}
	
	if (mode == CENTER_IF_FITS && top_margin + bottom_margin >= 0.0) {
		// Image fits vertically, so center it in that direction
		// by equalizing its top and bottom margins.
		double const new_margins = 0.5 * (top_margin + bottom_margin);
		widget_focal_point.ry() += new_margins - top_margin;
	} else if (top_margin < 0.0 && bottom_margin > 0.0) {
		// Move image down so that either top_margin or bottom_margin
		// becomes zero, whichever requires less movement.
		double const movement = std::min(fabs(top_margin), fabs(bottom_margin));
		widget_focal_point.ry() += movement;
	} else if (bottom_margin < 0.0 && top_margin > 0.0) {
		// Move image up so that either top_margin or bottom_margin
		// becomes zero, whichever requires less movement.
		double const movement = std::min(fabs(top_margin), fabs(bottom_margin));
		widget_focal_point.ry() -= movement;
	}
	
	return widget_focal_point;
}

void
ImageViewBase::setNewWidgetFP(QPointF const widget_fp, bool const update)
{
	if (widget_fp != m_widgetFocalPoint) {
		m_widgetFocalPoint = widget_fp;
		updateWidgetTransform();
		if (update) {
			this->update();
		}
	}
}

/**
 * Used when dragging the image.  It adjusts the movement to disallow
 * dragging it away from the ideal position (determined by
 * getIdealWidgetFocalPoint()).  Movement towards the ideal position
 * is permitted.  This works independently in horizontal and vertical
 * direction.
 *
 * \param proposed_widget_fp The proposed value for m_widgetFocalPoint.
 * \param update Whether to call this->update() in case the focal point
 *        has changed.
 */
void
ImageViewBase::adjustAndSetNewWidgetFP(
	QPointF const proposed_widget_fp, bool const update)
{
	// We first apply the proposed focal point, and only then
	// calculate the ideal one.  That's done because
	// the ideal focal point is the current focal point when
	// no widget space is wasted (image covers the whole widget).
	// We don't want the ideal focal point to be equal to the current
	// one, as that would disallow any movements.
	QPointF const old_widget_fp(m_widgetFocalPoint);
	setNewWidgetFP(proposed_widget_fp, update);
	
	QPointF const ideal_widget_fp(getIdealWidgetFocalPoint(CENTER_IF_FITS));
	
	QPointF const towards_ideal(ideal_widget_fp - old_widget_fp);
	QPointF const towards_proposed(proposed_widget_fp - old_widget_fp);
	
	QPointF movement(towards_proposed);
	
	// Horizontal movement.
	if (towards_ideal.x() * towards_proposed.x() < 0.0) {
		// Wrong direction - no movement at all.
		movement.setX(0.0);
	} else if (fabs(towards_proposed.x()) > fabs(towards_ideal.x())) {
		// Too much movement - limit it.
		movement.setX(towards_ideal.x());
	}
	
	// Vertical movement.
	if (towards_ideal.y() * towards_proposed.y() < 0.0) {
		// Wrong direction - no movement at all.
		movement.setY(0.0);
	} else if (fabs(towards_proposed.y()) > fabs(towards_ideal.y())) {
		// Too much movement - limit it.
		movement.setY(towards_ideal.y());
	}
	
	QPointF const adjusted_widget_fp(old_widget_fp + movement);
	if (adjusted_widget_fp != m_widgetFocalPoint) {
		m_widgetFocalPoint = adjusted_widget_fp;
		updateWidgetTransform();
		if (update) {
			this->update();
		}
	}
}

/**
 * Returns the center point of the available display area.
 */
QPointF
ImageViewBase::centeredWidgetFocalPoint() const
{
	return maxViewportRect().center();
}

void
ImageViewBase::setWidgetFocalPointWithoutMoving(QPointF const new_widget_fp)
{
	m_widgetFocalPoint = new_widget_fp;
	m_pixmapFocalPoint = m_virtualToImage.map(
		m_widgetToVirtual.map(m_widgetFocalPoint)
	);
}

/**
 * Returns true if m_hqPixmap is valid and up to date.
 */
bool
ImageViewBase::validateHqPixmap() const
{
	if (!m_hqTransformEnabled) {
		return false;
	}

	if (m_hqPixmap.isNull()) {
		return false;
	}

	if (m_hqSourceId != m_image.cacheKey()) {
		return false;
	}

	if (m_hqXform != m_imageToVirtual * m_virtualToWidget) {
		return false;
	}

	return true;
}

void
ImageViewBase::scheduleHqVersionRebuild()
{
	QTransform const xform(m_imageToVirtual * m_virtualToWidget);

	if (!m_timer.isActive() || m_potentialHqXform != xform) {
		if (m_ptrHqTransformTask.get()) {
			m_ptrHqTransformTask->cancel();
			m_ptrHqTransformTask.reset();
		}
		m_potentialHqXform = xform;
	}
	m_timer.start();
}

void
ImageViewBase::initiateBuildingHqVersion()
{
	if (validateHqPixmap()) {
		return;
	}

	m_hqPixmap = QPixmap();

	if (m_ptrHqTransformTask.get()) {
		m_ptrHqTransformTask->cancel();
		m_ptrHqTransformTask.reset();
	}

	QTransform const xform(m_imageToVirtual * m_virtualToWidget);
	IntrusivePtr<HqTransformTask> const task(
		new HqTransformTask(this, m_image, xform, viewport()->size())
	);
	
	backgroundExecutor().enqueueTask(task);
	
	m_ptrHqTransformTask = task;
	m_hqXform = xform;
	m_hqSourceId = m_image.cacheKey();
}

/**
 * Gets called from HqTransformationTask::Result.
 */
void
ImageViewBase::hqVersionBuilt(
	QPoint const& origin, QImage const& image)
{
	if (!m_hqTransformEnabled) {
		return;
	}
	
	m_hqPixmap = QPixmap::fromImage(image);
	m_hqPixmapPos = origin;
	m_ptrHqTransformTask.reset();
	update();
}

void
ImageViewBase::updateStatusTipAndCursor()
{
	updateStatusTip();
	updateCursor();
}

void
ImageViewBase::updateStatusTip()
{
	ensureStatusTip(m_interactionState.statusTip());
}

void
ImageViewBase::updateCursor()
{
	viewport()->setCursor(m_interactionState.cursor());
}

void
ImageViewBase::maybeQueueRedraw()
{
	if (m_interactionState.redrawRequested()) {
		m_interactionState.setRedrawRequested(false);
		update();
	}
}

BackgroundExecutor&
ImageViewBase::backgroundExecutor()
{
	static BackgroundExecutor executor;
	return executor;
}


/*==================== ImageViewBase::HqTransformTask ======================*/

ImageViewBase::HqTransformTask::HqTransformTask(
	ImageViewBase* image_view,
	QImage const& image, QTransform const& xform,
	QSize const& target_size)
:	m_ptrResult(new Result(image_view)),
	m_image(image),
	m_xform(xform),
	m_targetSize(target_size)
{
}

IntrusivePtr<AbstractCommand0<void> >
ImageViewBase::HqTransformTask::operator()()
{
	if (isCancelled()) {
		return IntrusivePtr<AbstractCommand0<void> >();
	}
	
	QRect const target_rect(
		m_xform.map(
			QRectF(m_image.rect())
		).boundingRect().toRect().intersected(
			QRect(QPoint(0, 0), m_targetSize)
		)
	);
	
	QImage hq_image(
		transform(
			m_image, m_xform, target_rect,
			OutsidePixels::assumeWeakColor(Qt::white), QSizeF(0.0, 0.0)
		)
	);
#if defined(Q_WS_X11)
	// ARGB32_Premultiplied is an optimal format for X11 + XRender.
	hq_image = hq_image.convertToFormat(QImage::Format_ARGB32_Premultiplied);
#endif
	m_ptrResult->setData(target_rect.topLeft(), hq_image);
	
	return m_ptrResult;
}


/*================ ImageViewBase::HqTransformTask::Result ================*/

ImageViewBase::HqTransformTask::Result::Result(
	ImageViewBase* image_view)
:	m_ptrImageView(image_view)
{
}

void
ImageViewBase::HqTransformTask::Result::setData(
	QPoint const& origin, QImage const& hq_image)
{
	m_hqImage = hq_image;
	m_origin = origin;
}

void
ImageViewBase::HqTransformTask::Result::operator()()
{
	if (m_ptrImageView && !isCancelled()) {
		m_ptrImageView->hqVersionBuilt(m_origin, m_hqImage);
	}
}


/*================= ImageViewBase::TempFocalPointAdjuster =================*/

ImageViewBase::TempFocalPointAdjuster::TempFocalPointAdjuster(ImageViewBase& obj)
:	m_rObj(obj),
	m_origWidgetFP(obj.getWidgetFocalPoint())
{
	obj.setWidgetFocalPointWithoutMoving(obj.centeredWidgetFocalPoint());
}

ImageViewBase::TempFocalPointAdjuster::TempFocalPointAdjuster(
	ImageViewBase& obj, QPointF const temp_widget_fp)
:	m_rObj(obj),
	m_origWidgetFP(obj.getWidgetFocalPoint())
{
	obj.setWidgetFocalPointWithoutMoving(temp_widget_fp);
}

ImageViewBase::TempFocalPointAdjuster::~TempFocalPointAdjuster()
{
	m_rObj.setWidgetFocalPointWithoutMoving(m_origWidgetFP);
}


/*================== ImageViewBase::TransformChangeWatcher ================*/

ImageViewBase::TransformChangeWatcher::TransformChangeWatcher(ImageViewBase& owner)
:	m_rOwner(owner),
	m_imageToVirtual(owner.m_imageToVirtual),
	m_virtualToWidget(owner.m_virtualToWidget),
	m_virtualDisplayArea(owner.m_virtualDisplayArea)
{
	++m_rOwner.m_transformChangeWatchersActive;
}

ImageViewBase::TransformChangeWatcher::~TransformChangeWatcher()
{
	if (--m_rOwner.m_transformChangeWatchersActive == 0) {
		if (m_imageToVirtual != m_rOwner.m_imageToVirtual ||
				m_virtualToWidget != m_rOwner.m_virtualToWidget ||
				m_virtualDisplayArea != m_rOwner.m_virtualDisplayArea) {
			m_rOwner.transformChanged();
		}
	}
}
