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

#ifndef SELECT_CONTENT_IMAGEVIEW_H_
#define SELECT_CONTENT_IMAGEVIEW_H_

#include "ImageViewBase.h"
#include <QRectF>
#include <QString>

class ImageTransformation;
class QMenu;

namespace select_content
{

class ImageView : public ImageViewBase
{
	Q_OBJECT
public:
	/**
	 * \p content_rect is in virtual image coordinates.
	 */
	ImageView(
		QImage const& image, QImage const& downscaled_image,
		ImageTransformation const& xform,
		QRectF const& content_rect);
	
	virtual ~ImageView();
signals:
	void manualContentRectSet(QRectF const& content_rect);
protected:
	virtual void paintOverImage(QPainter& painter);
	
	virtual void wheelEvent(QWheelEvent* event);
	
	virtual void mousePressEvent(QMouseEvent* event);
	
	virtual void mouseReleaseEvent(QMouseEvent* event);
	
	virtual void mouseMoveEvent(QMouseEvent* event);
	
	virtual void hideEvent(QHideEvent* event);
	
	virtual void contextMenuEvent(QContextMenuEvent* event);
	
	virtual QString defaultStatusTip() const;
private slots:
	void createContentBox();
	
	void removeContentBox();
private:
	enum { TOP_EDGE = 1, BOTTOM_EDGE = 2, LEFT_EDGE = 4, RIGHT_EDGE = 8 };
	
	int cursorLocationMask(QPoint const& cursor_pos) const;
	
	void forceMinWidthAndHeight(QRectF& widget_rect) const;
	
	void forceInsideImage(QRectF& widget_rect) const;
	
	QString m_defaultStatusTip;
	
	QString m_resizeStatusTip;
	
	/**
	 * The context menu to be shown if there is no content box.
	 */
	QMenu* m_pNoContentMenu;
	
	/**
	 * The context menu to be shown if there exists a content box.
	 */
	QMenu* m_pHaveContentMenu;
	
	/**
	 * Content box in virtual image coordinates.
	 */
	QRectF m_contentRect;
	
	/**
	 * Content box in widget coordinates in the beginning of resizing.
	 */
	QRectF m_widgetRectBeforeResizing;
	
	/**
	 * Cursor position in the beginning of resizing.
	 */
	QPoint m_cursorPosBeforeResizing;
	
	/**
	 * A bitwise OR of *_EDGE values.  If non-zero, that means
	 * a resizing operation is in progress.
	 */
	int m_resizingMask;
};

} // namespace select_content

#endif
