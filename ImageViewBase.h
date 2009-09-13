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

#ifndef IMAGEVIEWBASE_H_
#define IMAGEVIEWBASE_H_

#include "Margins.h"
#include "IntrusivePtr.h"
#include <QTimer>
#include <QWidget>
#include <QPixmap>
#include <QImage>
#include <QString>
#include <QTransform>
#include <QPoint>
#include <QPointF>
#include <QSizeF>
#include <QRectF>
#include <Qt>

class QPainter;
class BackgroundExecutor;

/**
 * \brief The base class for widgets that display and manipulate images.
 *
 * This class operates with 3 coordinate systems:
 * \li Image coordinates, where m_image.rect() is defined.
 * \li Pixmap coordinates, where m_pixmap.rect() is defined.
 *     m_pixmap is constructed from a downscaled version of m_image.
 * \li Virtual image coordinates.  We need them because we are not
 *     displaying m_image as is.  Instead, we display a pre-transformed
 *     version of it.  So, the virtual image coordinates reference the
 *     pixels of an imaginary image that we would get if we actually
 *     pre-transformed m_image the way we want.
 * \li Widget coordinates, where this->rect() is defined.
 *
 * \see m_pixmapToImage, m_imageToVirt, m_virtualToWidget, m_widgetToVirtual.
 */
class ImageViewBase : public QWidget
{
	Q_OBJECT
public:
	/**
	 * \brief ImageViewBase constructor.
	 *
	 * \param image The image to display.
	 * \param downscaled_image The downscaled version of \p image.
	 *        If it's null, it will be created automatically.
	 *        The exact scale doesn't matter.
	 *        The whole idea of having a downscaled version is
	 *        to speed up real-time rendering of high-resolution
	 *        images.  Note that the delayed high quality transform
	 *        operates on the original image, not the downscaled one.
	 * \param image_to_virt Virtual image transformation.
	 * \param virt_display_area The area of the virtual image to be displayed.
	 * \param margins Reserve extra space near the widget borders.
	 *        The units are widget pixels.  This reserved area may
	 *        still be covered by parts of the image that are outside
	 *        of pre_transform.resultingRect().
	 */
	ImageViewBase(
		QImage const& image, QImage const& downscaled_image,
		QTransform const& image_to_virt, QPolygonF const& virt_display_area,
		Margins const& margins = Margins());
	
	virtual ~ImageViewBase();
	
	/**
	 * \brief Enable or disable the high-quality transform.
	 */
	void hqTransformSetEnabled(bool enabled);
	
	/**
	 * \brief A stand-alone function to create a downscaled image
	 *        to be passed to the constructor.
	 *
	 * The point of using this function instead of letting
	 * the constructor do the job is that this function may
	 * be called from a background thread, while the constructor
	 * can't.
	 *
	 * \param image The input image, not null, and with DPI set correctly.
	 * \return The image downscaled by an unspecified degree.
	 */
	static QImage createDownscaledImage(QImage const& image);
protected:
	enum ZoomFocus { ZOOM_FOCUS_CENTER, ZOOM_FOCUS_CURSOR };
	
	enum FocalPointMode { CENTER_IF_FITS, DONT_CENTER };
	
	/**
	 * \brief Repaint the widget.
	 *
	 * \note Don't override this one.  Override paintOverImage() instead.
	 */
	virtual void paintEvent(QPaintEvent* event);
	
	virtual void paintOverImage(QPainter& painter);

	/**
	 * \brief Called when any of the transformations change.
	 */
	virtual void transformChanged() {}
	
	/**
	 * \brief Handle widget resizing.
	 *
	 * \note If overriden, call this version first!
	 */
	virtual void resizeEvent(QResizeEvent* event);
	
	/**
	 * \brief Cancels ongoing image dragging.
	 *
	 * \note If overriden, call this version first!
	 */
	virtual void hideEvent(QHideEvent* event);
	
	/**
	 * \brief Performs image zooming.
	 *
	 * To be called from subclasses.
	 */
	void handleZooming(QWheelEvent* event, ZoomFocus = ZOOM_FOCUS_CURSOR);
	
	/**
	 * \brief Possibly starts or stops image dragging.
	 *
	 * To be called from subclasses on the following events:
	 * MouseButtonPress, MouseButtonRelease, MouseMove.
	 */
	void handleImageDragging(QMouseEvent* event);
	
	QPolygonF const& virtualDisplayArea() const { return m_virtualDisplayArea; }

	QRectF const virtualDisplayRect() const {
		return m_virtualDisplayArea.boundingRect();
	}

	QTransform const& imageToVirtual() const { return m_imageToVirtual; }
	
	QTransform const& virtualToImage() const { return m_virtualToImage; }

	QTransform const& virtualToWidget() const { return m_virtualToWidget; }
	
	QTransform const& widgetToVirtual() const { return m_widgetToVirtual; }
	
	bool isDraggingInProgress() const { return m_isDraggingInProgress; }
	
	/**
	 * Returns true if any part of the image is off-screen.
	 */
	bool isDraggingPossible() const;
	
	/**
	 * Returns the widget area reduced by margins.
	 */
	QRectF marginsRect() const;
	
	/**
	 * Get the bounding box of the image as it appears on the screen,
	 * in widget coordinates.
	 */
	QRectF getVisibleWidgetRect() const;
	
	/**
	 * \brief Get the focal point in widget coordinates.
	 *
	 * The typical usage pattern for this function is:
	 * \code
	 * QPointF fp(obj.getWidgetFocalPoint());
	 * obj.setWidgetFocalPoint(fp + delta);
	 * \endcode
	 * As a result, the image will be moved by delta widget pixels.
	 */
	QPointF getWidgetFocalPoint() const { return m_widgetFocalPoint; }
	
	/**
	 * \brief Set the focal point in widget coordinates.
	 *
	 * \see getWidgetFocalPoint()
	 */
	void setWidgetFocalPoint(QPointF const& widget_fp);
	
	/**
	 * \brief Resets the zoom to the default value.
	 *
	 * The default zoom is such that the image can fit into the widget.
	 */
	void resetZoom();
	
	/**
	 * \brief Updates image-to-virtual and recalculates
	 *        virtual-to-widget transformations.
	 */
	void updateTransform(
		QTransform const& image_to_virt,
		QPolygonF const& virt_display_area);
	
	/**
	 * \brief Same as updateTransform(), but adjusts the focal point
	 *        to improve screen space usage.
	 */
	void updateTransformAndFixFocalPoint(
		QTransform const& image_to_virt,
		QPolygonF const& virt_display_area, FocalPointMode mode);
	
	/**
	 * \brief Same as updateTransform(), but preserves the visual image scale.
	 */
	void updateTransformPreservingScale(
		QTransform const& image_to_virt, QPolygonF const& virt_display_area);
	
	/**
	 * \brief A faster version of setCursor().
	 *
	 * This method checks if the shape we want to set is already set,
	 * and if so, does nothing.
	 * \note Calls to this method must not be mixed with calls to setCursor()
	 *       and unsetCursor().
	 */
	void ensureCursorShape(Qt::CursorShape cursor_shape);
	
	/**
	 * \brief A better version of setStatusTip().
	 *
	 * Unlike setStatusTip(), this method will display the tooltip
	 * immediately, not when the mouse enters the widget next time.
	 */
	void ensureStatusTip(QString const& status_tip);
	
	/**
	 * \brief Returns the status tip to be associated with this widget
	 *        in its default state.
	 *
	 * Subclasses may reimplement this method.
	 */
	virtual QString defaultStatusTip() const;

	static BackgroundExecutor& backgroundExecutor();

	virtual void enterEvent(QEvent* event);
private slots:
	void initiateBuildingHqVersion();
private:
	class HqTransformTask;
	class TempFocalPointAdjuster;
	class TransformChangeWatcher;

	void updateWidgetTransform();
	
	void updateWidgetTransformAndFixFocalPoint(FocalPointMode mode);
	
	QPointF getIdealWidgetFocalPoint(FocalPointMode mode) const;
	
	void setNewWidgetFP(QPointF widget_fp);
	
	void adjustAndSetNewWidgetFP(QPointF proposed_widget_fp);
	
	QPointF centeredWidgetFocalPoint() const;
	
	void setWidgetFocalPointWithoutMoving(QPointF new_widget_fp);
	
	bool validateHqPixmap() const;

	void scheduleHqVersionRebuild();

	void hqVersionBuilt(QPoint const& origin, QImage const& image);

	QString m_defaultStatusTip;
	
	QString m_unrestrictedDragStatusTip;
	
	/**
	 * The client-side image.  Used to build a high-quality version
	 * for delayed rendering.
	 */
	QImage m_image;
	
	/**
	 * This timer is used for delaying the construction of
	 * a high quality image version.
	 */
	QTimer m_timer;
	
	/**
	 * The image handle.  Note that the actual data of a QPixmap lives
	 * in another process on most platforms.
	 */
	QPixmap m_pixmap;
	
	/**
	 * The high quality, pre-transformed version of m_pixmap.
	 */
	QPixmap m_hqPixmap;
	
	/**
	 * The position, in widget coordinates, where m_hqPixmap is to be drawn.
	 */
	QPoint m_hqPixmapPos;
	
	/**
	 * The transformation used to build m_hqPixmap.
	 * It's used to detect if m_hqPixmap needs to be rebuild.
	 */
	QTransform m_hqXform;

	/**
	 * Used to check if we need to extend the delay before building m_hqPixmap.
	 */
	QTransform m_potentialHqXform;
	
	/**
	 * The ID (QImage::cacheKey()) of the image that was used
	 * to build m_hqPixmap.  It's used to detect if m_hqPixmap
	 * needs to be rebuilt.
	 */
	qint64 m_hqSourceId;
	
	/**
	 * The pending (if any) high quality transformation task.
	 */
	IntrusivePtr<HqTransformTask> m_ptrHqTransformTask;

	/**
	 * Transformation from m_pixmap coordinates to m_image coordinates.
	 */
	QTransform m_pixmapToImage;
	
	/**
	 * The area of the image (in virtual coordinates) to display.
	 */
	QPolygonF m_virtualDisplayArea;

	/**
	 * A transformation from original to virtual image coordinates.
	 */
	QTransform m_imageToVirtual;
	
	/**
	 * A transformation from virtual to original image coordinates.
	 */
	QTransform m_virtualToImage;

	/**
	 * Transformation from virtual image coordinates to widget coordinates.
	 */
	QTransform m_virtualToWidget;
	
	/**
	 * Transformation from widget coordinates to virtual image coordinates.
	 */
	QTransform m_widgetToVirtual;
	
	/**
	 * An arbitrary point in widget coordinates that corresponds
	 * to m_pixmapFocalPoint in m_pixmap coordinates.
	 * Moving m_widgetFocalPoint followed by updateWidgetTransform()
	 * will cause the image to move on screen.
	 */
	QPointF m_widgetFocalPoint;
	
	/**
	 * An arbitrary point in m_pixmap coordinates that corresponds
	 * to m_widgetFocalPoint in widget coordinates.
	 * Unlike m_widgetFocalPoint, this one is not supposed to be
	 * moved independently.  It's supposed to moved together with
	 * m_widgetFocalPoint for zooming into a specific position.
	 */
	QPointF m_pixmapFocalPoint;
	
	/**
	 * Used for dragging the image.  Holds the last cursor position
	 * (in widget coordinates) that was processed in this dragging session.
	 */
	QPoint m_lastMousePos;
	
	/**
	 * The number of pixels to be left blank at each side of the widget.
	 */
	Margins m_margins;
	
	/**
	 * The zoom factor.  A value of 1.0 corresponds to fit-to-widget zoom.
	 */
	double m_zoom;
	
	int m_transformChangeWatchersActive;

	/**
	 * The current cursor shape, cached to improve performance.
	 */
	Qt::CursorShape m_currentCursorShape;
	
	bool m_isDraggingInProgress;
	
	bool m_hqTransformEnabled;
};

#endif
