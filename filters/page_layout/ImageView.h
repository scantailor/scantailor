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
#include "Margins.h"
#include <QSizeF>
#include <QRectF>
#include <QPoint>

namespace page_layout
{

class ImageView : public ImageViewBase
{
	Q_OBJECT
public:
	ImageView(
		QImage const& image, ImageTransformation const& xform,
		QRectF const& content_rect, Margins const& margins_mm,
		QSizeF const& aggregate_content_size_mm);
	
	virtual ~ImageView();
signals:
	void marginsSetManually(Margins const& margins_mm);
protected:
	virtual void paintOverImage(QPainter& painter);
	
	virtual void wheelEvent(QWheelEvent* event);
	
	virtual void mousePressEvent(QMouseEvent* event);
	
	virtual void mouseReleaseEvent(QMouseEvent* event);
	
	virtual void mouseMoveEvent(QMouseEvent* event);
	
	virtual void hideEvent(QHideEvent* event);
private:
	enum { TOP_EDGE = 1, BOTTOM_EDGE = 2, LEFT_EDGE = 4, RIGHT_EDGE = 8 };
	
	void fitMarginBox(FocalPointMode fp_mode);
	
	void calcAndFitMarginBox(Margins const& margins_mm, FocalPointMode fp_mode);
	
	int cursorLocationMask(QPoint const& cursor_pos) const;
	
	void forceNonNegativeMargins(QRectF& content_plus_margins) const;
	
	Margins calculateMarginsMM() const;
	
	/**
	 * Image transformation, as provided by the previous filter.
	 * We pass another transformation to ImageViewBase, in order to
	 * be able to display margins, that may be outside the image area.
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
	
	QSizeF m_aggregateContentSizeMM;
	
	
	/**
	 * Transformation from widget coordinates to m_origXform coordinates
	 * in the beginning of resizing.
	 */
	QTransform m_widgetToOrigBeforeResizing;
	
	/**
	 * m_contentPlusMargins in widget coordinates in the beginning of resizing.
	 */
	QRectF m_widgetRectBeforeResizing;
	
	/**
	 * Cursor position in widget coordinates in the beginning of resizing.
	 */
	QPoint m_cursorPosBeforeResizing;
	
	/**
	 * A bitwise OR of *_EDGE values.  If non-zero, that means
	 * a resizing operation is in progress.
	 */
	int m_resizingMask;
};

} // namespace page_layout

#endif
