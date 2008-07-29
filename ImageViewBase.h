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

#ifndef IMAGEVIEWBASE_H_
#define IMAGEVIEWBASE_H_

#include "ImageTransformation.h"
#include "Margins.h"
#include <QWidget>
#include <QPixmap>
#include <QImage>
#include <QTransform>
#include <QPoint>
#include <QPointF>
#include <QSizeF>
#include <QRectF>
#include <Qt>

class QPainter;

/**
 * \brief The base class for widgets that display and manipulate images.
 *
 * This class operates with 3 coordinate systems:
 * \li Physical image coordinates, where m_pixmap.rect() is defined.
 * \li Virtual image coordinates.  We need them because we are not
 *     displaying m_pixmap as is, instead we display a pre-transformed
 *     version of it.  So, the virtual image coordinates reference the
 *     pixels of an imaginary image that we would get if we actually
 *     transformed m_pixmap the way we want.
 * \li Widget coordinates, where this->rect() is defined.
 *
 * \see m_physToVirt, m_virtualToWidget, m_widgetToVirtual.
 */
class ImageViewBase : public QWidget
{
	Q_OBJECT
public:
	ImageViewBase(QImage const& image);
	
	ImageViewBase(
		QImage const& image, ImageTransformation const& pre_transform,
		Margins const& margins = Margins());
	
	virtual ~ImageViewBase();
protected:
	enum FocalPointMode { CENTER_IF_FITS, DONT_CENTER };
	
	/**
	 * \brief Repaint the widget.
	 *
	 * \note Don't override this one.  Override paintOverImage() instead.
	 */
	virtual void paintEvent(QPaintEvent* event);
	
	virtual void paintOverImage(QPainter& painter);
	
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
	void handleZooming(QWheelEvent* event);
	
	/**
	 * \brief Possibly starts or stops image dragging.
	 *
	 * To be called from subclasses on the following events:
	 * MouseButtonPress, MouseButtonRelease, MouseMove.
	 */
	void handleImageDragging(QMouseEvent* event);
	
	ImageTransformation const& physToVirt() const { return m_physToVirt; }
	
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
	 * \brief Get the focal point in physical image coordinates.
	 *
	 * The focal point is the image point that will be displayed at the
	 * center of the widget.
	 */
	QPointF getFocalPoint() const { return m_focalPoint; }
	
	/**
	 * \brief Set the focal point in physical image coordinates.
	 *
	 * The focal point is the image point that will be displayed at the
	 * center of the widget.
	 */
	void setFocalPoint(QPointF const& focal_point);
	
	/**
	 * \brief Resets the zoom to the default value.
	 *
	 * The default zoom is such that the image can fit into the widget.
	 */
	void resetZoom();
	
	/**
	 * \brief Replaces the current image with another one.
	 *
	 * The new image must have the same dimensions and the same DPI.
	 */
	void updateImage(QImage const& image);
	
	/**
	 * \brief Updates physical-to-virtual and recalculates
	 *        virtual-to-physical transformations.
	 */
	void updateTransform(ImageTransformation const& phys_to_virt);
	
	/**
	 * \brief Same as updateTransform(), but adjusts the focal point
	 *        to improve screen space usage.
	 */
	void updateTransformAndFixFocalPoint(
		ImageTransformation const& phys_to_virt, FocalPointMode mode);
	
	/**
	 * \brief Same as updateTransform(), but preserves the visual image scale.
	 */
	void updateTransformPreservingScale(ImageTransformation const& phys_to_virt);
	
	/**
	 * \brief A faster version of setCursor().
	 *
	 * This method checks if the shape we want to set is already set,
	 * and if so, does nothing.
	 * \note Calls to this method must not be mixed with calls to setCursor()
	 *       and unsetCursor().
	 */
	void ensureCursorShape(Qt::CursorShape cursor_shape);
private:
	void updateWidgetTransform();
	
	void updateWidgetTransformAndFixFocalPoint(FocalPointMode mode);
	
	QPointF getIdealFocalPoint(FocalPointMode mode) const;
	
	void adjustAndSetNewFocalPoint(QPointF new_focal_point);
	
	/**
	 * The image handle.  Note that the actual data of a QPixmap lives
	 * in another process on most platforms.
	 */
	QPixmap m_pixmap;
	
	/**
	 * A set of logical transformations on m_pixmap, to translate
	 * from physical to virtual image coordinates and back.
	 */
	ImageTransformation m_physToVirt;
	
	/**
	 * Transformation from virtual image coordinates to widget coordinates.
	 */
	QTransform m_virtualToWidget;
	
	/**
	 * Transformation from widget coordinates to virtual image coordinates.
	 */
	QTransform m_widgetToVirtual;
	
	/**
	 * The point in m_pixmap that is supposed to be at the center of the widget.
	 */
	QPointF m_focalPoint;
	
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
	
	/**
	 * The current cursor shape, cached to improve performance.
	 */
	Qt::CursorShape m_currentCursorShape;
	
	bool m_isDraggingInProgress;
};

#endif
