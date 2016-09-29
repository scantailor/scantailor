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

#ifndef PAGE_SPLIT_IMAGEVIEW_H_
#define PAGE_SPLIT_IMAGEVIEW_H_

#include "ImageViewBase.h"
#include "DragHandler.h"
#include "ZoomHandler.h"
#include "ObjectDragHandler.h"
#include "DraggablePoint.h"
#include "DraggableLineSegment.h"
#include "PageLayout.h"
#include "UnremoveButton.h"
#include "ImageId.h"
#include "IntrusivePtr.h"
#include <QPixmap>

class ImageTransformation;
class ProjectPages;
class PageInfo;
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
		ImageTransformation const& xform, PageLayout const& layout,
		IntrusivePtr<ProjectPages> const& pages, ImageId const& image_id,
		bool left_half_removed, bool right_half_removed);
	
	virtual ~ImageView();
signals:
	void invalidateThumbnail(PageInfo const& page_info);

	void pageLayoutSetLocally(PageLayout const& layout);
public slots:
	void pageLayoutSetExternally(PageLayout const& layout);
protected:
	virtual void onPaint(QPainter& painter, InteractionState const& interaction);
private:
	void setupCuttersInteraction();

	QPointF handlePosition(int line_idx, int handle_idx) const;

	void handleMoveRequest(int line_idx, int handle_idx, QPointF const& pos);

	QLineF linePosition(int line_idx) const;

	void lineMoveRequest(int line_idx, QLineF line);

	void dragFinished();

	QPointF leftPageCenter() const;

	QPointF rightPageCenter() const;

	void unremoveLeftPage();

	void unremoveRightPage();

	/**
	 * \return Page layout in widget coordinates.
	 */
	PageLayout widgetLayout() const;

	/**
	 * \return A Cutter line in widget coordinates.
	 *
	 * Depending on the current interaction state, the line segment
	 * may end either shortly before the widget boundaries, or shortly
	 * before the image boundaries.
	 */
	QLineF widgetCutterLine(int line_idx) const;

	/**
	 * \return A Cutter line in virtual image coordinates.
	 *
	 * Unlike widgetCutterLine(), this one always ends shortly before
	 * the image boundaries.
	 */
	QLineF virtualCutterLine(int line_idx) const;

	/**
	 * Same as ImageViewBase::getVisibleWidgetRect(), except reduced
	 * vertically to accommodate the height of line endpoint handles.
	 */
	QRectF reducedWidgetArea() const;

	static QLineF customInscribedCutterLine(QLineF const& line, QRectF const& rect);

	IntrusivePtr<ProjectPages> m_ptrPages;
	ImageId m_imageId;
	DraggablePoint m_handles[2][2];
	ObjectDragHandler m_handleInteractors[2][2];
	DraggableLineSegment m_lineSegments[2];
	ObjectDragHandler m_lineInteractors[2];
	UnremoveButton m_leftUnremoveButton;
	UnremoveButton m_rightUnremoveButton;
	DragHandler m_dragHandler;
	ZoomHandler m_zoomHandler;

	QPixmap m_handlePixmap;

	/**
	 * Page layout in virtual image coordinates.
	 */
	PageLayout m_virtLayout;

	bool m_leftPageRemoved;
	bool m_rightPageRemoved;
};

} // namespace page_split

#endif
