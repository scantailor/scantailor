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
#include "DragHandler.h"
#include "ZoomHandler.h"
#include "DraggablePoint.h"
#include "DraggableLineSegment.h"
#include "ObjectDragHandler.h"
#include <QRectF>
#include <QSizeF>
#include <QString>

class ImageTransformation;
class QMenu;

namespace select_content
{

class ImageView :
	public ImageViewBase,
	private InteractionHandler
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
private slots:
	void createContentBox();
	
	void removeContentBox();
private:
	enum Edge { LEFT = 1, RIGHT = 2, TOP = 4, BOTTOM = 8 };

	virtual void onPaint(QPainter& painter, InteractionState const& interaction);

	void onContextMenuEvent(QContextMenuEvent* event, InteractionState& interaction);

	QPointF cornerPosition(int edge_mask) const;

	void cornerMoveRequest(int edge_mask, QPointF const& pos);

	QLineF edgePosition(int edge) const;

	void edgeMoveRequest(int edge, QLineF const& line);

	void dragFinished();

	void forceInsideImage(QRectF& widget_rect, int edge_mask) const;
	
	DraggablePoint m_corners[4];
	ObjectDragHandler m_cornerHandlers[4];

	DraggableLineSegment m_edges[4];
	ObjectDragHandler m_edgeHandlers[4];

	DragHandler m_dragHandler;
	ZoomHandler m_zoomHandler;
	
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

	QSizeF m_minBoxSize;
};

} // namespace select_content

#endif
