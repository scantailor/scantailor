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

#ifndef OUTPUT_DEWARPING_VIEW_H_
#define OUTPUT_DEWARPING_VIEW_H_

#include "ImageViewBase.h"
#include "ImagePixmapUnion.h"
#include "InteractionHandler.h"
#include "DraggablePoint.h"
#include "ObjectDragHandler.h"
#include "DragHandler.h"
#include "ZoomHandler.h"
#include "imageproc/XSpline.h"
#include <QTransform>
#include <QPointF>
#include <QRectF>

namespace output
{

class DewarpingView : public ImageViewBase, protected InteractionHandler
{
	Q_OBJECT
public:
	DewarpingView(QImage const& image, ImagePixmapUnion const& downscaled_image,
		QTransform const& source_to_virt, QRectF const& virt_content_rect);
	
	virtual ~DewarpingView();
protected:
	virtual void onPaint(QPainter& painter, InteractionState const& interaction);
private:
	QPointF curvePointPosition(int curve_idx, int point_idx) const;

	void curvePointMoveRequest(int curve_idx, int point_idx, QPointF const& pos);

	void dragFinished();

	QTransform m_sourceToVirt;
	QTransform m_virtToSource;
	imageproc::XSpline m_topSpline;
	imageproc::XSpline m_bottomSpline;
	DraggablePoint m_curvePoints[2][4];
	ObjectDragHandler m_curvePointInteractors[2][4];
	DragHandler m_dragHandler;
	ZoomHandler m_zoomHandler;
};

} // namespace output

#endif
