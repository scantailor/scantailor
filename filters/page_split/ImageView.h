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

#ifndef PAGE_SPLIT_IMAGEVIEW_H_
#define PAGE_SPLIT_IMAGEVIEW_H_

#include "ImageViewBase.h"
#include "DragHandler.h"
#include "ZoomHandler.h"
#include "ObjectDragHandler.h"
#include "DraggablePoint.h"
#include "DraggableLineSegment.h"
#include "PageLayout.h"
#include <QPixmap>

class ImageTransformation;
class QPointF;
class QRectF;
class QLineF;

namespace page_split
{

class ImageView :
	public ImageViewBase,
	private InteractionHandler
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
	virtual void onPaint(QPainter& painter, InteractionState const& interaction);
private:
	QPointF handlePosition(int id) const;

	void handleMoveRequest(int id, QPointF const& pos);

	QLineF linePosition() const;

	void lineMoveRequest(QLineF line);

	void dragFinished();

	/**
	 * \return Page layout in widget coordinates.
	 */
	PageLayout widgetLayout() const;

	/**
	 * \return Split line in widget coordinates.
	 *
	 * Depending on the current interaction state, the line segment
	 * may end either shortly before the widget boundaries, or shortly
	 * before the image boundaries.
	 */
	QLineF widgetSplitLine() const;

	/**
	 * \return Split line in virtual image coordinates.
	 *
	 * Unlike widgetSplitLine(), this one always ends shortly before
	 * the image boundaries.
	 */
	QLineF virtualSplitLine() const;

	/**
	 * Same as ImageViewBase::getVisibleWidgetRect(), except reduced
	 * vertically to accomodate the height of line endpoint handles.
	 */
	QRectF reducedWidgetArea() const;

	static QLineF customInscribedSplitLine(QLineF const& line, QRectF const& rect);

	DraggablePoint m_handles[2];
	ObjectDragHandler m_handleInteractors[2];
	DraggableLineSegment m_lineSegment;
	ObjectDragHandler m_lineInteractor;
	DragHandler m_dragHandler;
	ZoomHandler m_zoomHandler;

	QPixmap m_handlePixmap;

	/**
	 * Page layout in virtual image coordinates.
	 */
	PageLayout m_virtLayout;
};

} // namespace page_split

#endif
