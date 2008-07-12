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

#ifndef PAGE_LAYOUT_IMAGEVIEW_H_
#define PAGE_LAYOUT_IMAGEVIEW_H_

#include "ImageViewBase.h"
#include "ImageTransformation.h"
#include <QSizeF>
#include <QRectF>
#include <QPointF>
#include <QPoint>

class Margins;

namespace page_layout
{

class OptionsWidget;

class ImageView : public ImageViewBase
{
	Q_OBJECT
public:
	ImageView(
		QImage const& image, ImageTransformation const& xform,
		QRectF const& content_rect, QSizeF const& aggregate_content_size_mm,
		OptionsWidget const& opt_widget);
	
	virtual ~ImageView();
signals:
	void marginsSetManually(Margins const& margins_mm);
public slots:
	void leftRightLinkToggled(bool linked);
	
	void topBottomLinkToggled(bool linked);
protected:
	virtual void paintOverImage(QPainter& painter);
	
	virtual void wheelEvent(QWheelEvent* event);
	
	virtual void mousePressEvent(QMouseEvent* event);
	
	virtual void mouseReleaseEvent(QMouseEvent* event);
	
	virtual void mouseMoveEvent(QMouseEvent* event);
	
	virtual void hideEvent(QHideEvent* event);
private:
	enum { TOP_EDGE = 1, BOTTOM_EDGE = 2, LEFT_EDGE = 4, RIGHT_EDGE = 8 };
	
	struct StateBeforeResizing
	{
		/**
		 * Transformation from widget coordinates to m_origXform coordinates
		 */
		QTransform widgetToOrig;
		
		/**
		 * m_contentPlusMargins in widget coordinates.
		 */
		QRectF outerWidgetRect;
		
		/**
		 * Mouse pointer position in widget coordinates.
		 */
		QPoint mousePos;
		
		/**
		 * The point in image that is to be centered on the screen
		 * in physical image coordinates.
		 */
		QPointF focalPoint;
	};
	
	void resizeInnerRect(QPoint delta);
	
	void resizeOuterRect(QPoint delta);
	
	void fitMarginBox(FocalPointMode fp_mode);
	
	void calcAndFitMarginBox(Margins const& margins_mm, FocalPointMode fp_mode);
	
	void updatePresentationTransform();
	
	int cursorLocationMask(QPoint const& cursor_pos, QRectF const& orig_rect) const;
	
	void forceNonNegativeMargins(QRectF& content_plus_margins) const;
	
	Margins calculateMarginsMM() const;
	
	/**
	 * Image transformation, as provided by the previous filter.
	 * We pass another transformation to ImageViewBase, which we call
	 * "presentation transform" in order to be able to display margins that
	 * may be outside the image area.
	 */
	ImageTransformation const m_origXform;
	
	/**
	 * Content box in m_origXform coordinates.
	 */
	QRectF const m_contentRect;
	
	/**
	 * Content + margins box in m_origXform coordinates.
	 */
	QRectF m_contentPlusMargins;
	
	/**
	 * Some data saved at the beginning of a resizing operation.
	 */
	StateBeforeResizing m_beforeResizing;
	
	/**
	 * A bitwise OR of *_EDGE values.  If non-zero, it means
	 * we are currently resizing the inner edge of the margin zone.
	 * \note Both m_innerResizingMask and m_outerResizingMask can't
	 * be non-zero at the same time.
	 */
	int m_innerResizingMask;
	
	/**
	 * A bitwise OR of *_EDGE values.  If non-zero, it means
	 * we are currently resizing the outer edge of the margin zone.
	 * \note Both m_innerResizingMask and m_outerResizingMask can't
	 * be non-zero at the same time.
	 */
	int m_outerResizingMask;
	
	bool m_leftRightLinked;
	
	bool m_topBottomLinked;
};

} // namespace page_layout

#endif
