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

namespace page_split
{

class ImageView :
	public ImageViewBase,
	private InteractionHandler,
	private DraggablePoint,
	private DraggableLineSegment
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

	virtual QPointF pointPosition(
		ObjectDragHandler const* handler, InteractionState const& interaction) const;

	virtual void pointMoveRequest(
		ObjectDragHandler const* handler, QPointF const& widget_pos,
		InteractionState const& interaction);

	virtual QLineF lineSegment(
		ObjectDragHandler const* handler, InteractionState const& interaction) const;

	virtual QPointF lineSegmentPosition(
		ObjectDragHandler const* handler, InteractionState const& interaction) const;

	virtual void lineSegmentMoveRequest(
		ObjectDragHandler const* handler, QPointF const& widget_pos,
		InteractionState const& interaction);

	virtual void dragFinished(ObjectDragHandler const* handler);
private:
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
	QLineF widgetSplitLine(InteractionState const& interaction) const;

	/**
	 * \return Split line in virtual image coordinates.
	 *
	 * Unlike widgetSplitLine(), this one always ends shortly before
	 * the image boundaries.
	 */
	QLineF virtualSplitLine() const;

	/**
	 * \return Valid area for split line endpoints in widget coordinates.
	 */
	QRectF widgetValidArea() const;

	ObjectDragHandler m_handle1DragHandler;
	ObjectDragHandler m_handle2DragHandler;
	ObjectDragHandler m_lineDragHandler;
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
