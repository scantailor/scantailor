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

#ifndef PAGE_SPLIT_IMAGEVIEW_H_
#define PAGE_SPLIT_IMAGEVIEW_H_

#include "ImageViewBase.h"
#include "PageLayout.h"
#include <QPoint>
#include <QPointF>
#include <QRectF>
#include <QString>

class QRect;
class QMenu;
class ImageTransformation;

namespace page_split
{

class ImageView : public ImageViewBase
{
	Q_OBJECT
public:
	ImageView(QImage const& image, QImage const& downscaled_image,
		ImageTransformation const& xform, PageLayout const& layout);
	
	virtual ~ImageView();
signals:
	void pageLayoutSetLocally(PageLayout const& layout);
public slots:
	void pageLayoutSetExternally(PageLayout const& layout);
protected:
	virtual void paintOverImage(QPainter& painter);
	
	virtual void wheelEvent(QWheelEvent* event);
	
	virtual void mousePressEvent(QMouseEvent* event);
	
	virtual void mouseReleaseEvent(QMouseEvent* event);
	
	virtual void mouseMoveEvent(QMouseEvent* event);
	
	virtual void hideEvent(QHideEvent* event);
private:
	enum State { DEFAULT_STATE, DRAGGING_SPLIT_LINE, SKEWING_SPLIT_LINE };
	
	static void extendToContain(QRectF& rect, QPointF const& point);
	
	static void forcePointIntoRect(QPointF& point, QRectF const& rect);
	
	static double distanceSquared(QPointF const p1, QPointF const p2);
	
	static QPointF projectPointToLine(QPointF const& point, QLineF const& line);
	
	bool isCursorNearSplitLine(
		QPointF const& cursor_pos,
		QPointF* touchpoint = 0, QPointF* far_end = 0) const;
	
	/**
	 * Cotangent representing the maximum skew angle of the split line,
	 * so that the following assertion is true:
	 * \code
	 * fabs(xdiff / ydiff) <= m_maxSkewAngleCtg
	 * \endcode
	 * Note that a vertical split line is considered to have a zero skew,
	 * which is why we have xdiff / ydiff and not the other way around.
	 */
	static double const m_maxSkewAngleCtg;
	
	QPixmap m_imgSkewingHandle;
	
	/**
	 * The status tip coming from ImageViewBase.
	 */
	QString m_baseStatusTip;
	
	QString m_dragHandleStatusTip;
	
	QString m_dragLineStatusTip;
	
	/**
	 * Page layout in virtual image coordinates.
	 */
	PageLayout m_pageLayout;
	
	/**
	 * The dragging point when skewing or moving the split line.
	 * This point belongs to the line (in widget coordinates),
	 * but is not updated while the line is being moved or skewed.
	 */
	QPointF m_splitLineTouchPoint;
	
	/**
	 * Like m_splitLineTouchPoint, this one belongs to the split line.
	 * In case of skewing the split line, this point is the rotation origin.
	 * Just like m_splitLineTouchPoint, this point is not updated in the
	 * process of moving / skewing.
	 */
	QPointF m_splitLineOtherPoint;
	
	/**
	 * When dragging or skewing the split line, this limits the range
	 * of allowed m_splitLineTouchPoint values.
	 */
	QRectF m_touchPointRange;
	
	/**
	 * The mouse cursor position (in widget coordinates) when dragging
	 * or skewing the split line was initiated.
	 */
	QPoint m_initialMousePos;
	
	/**
	 * The rectangle (in widget coordinates) of the top dragging handle.
	 */
	QRectF m_topHandleRect;
	
	/**
	 * The rectangle (in widget coordinates) of the bottom dragging handle.
	 */
	QRectF m_bottomHandleRect;
	
	State m_state;
};

} // namespace page_split

#endif
