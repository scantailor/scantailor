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

#ifndef DESKEW_IMAGEVIEW_H_
#define DESKEW_IMAGEVIEW_H_

#include "ImageViewBase.h"
#include <QPolygonF>
#include <QPoint>
#include <QPointF>
#include <QRectF>
#include <QPixmap>
#include <QString>
#include <utility>

class QRect;
class ImageTransformation;

namespace deskew
{

class ImageView : public ImageViewBase
{
	Q_OBJECT
public:
	ImageView(
		QImage const& image, QImage const& downscaled_image,
		ImageTransformation const& xform);
	
	virtual ~ImageView();
signals:
	void manualDeskewAngleSet(double degrees);
public slots:
	void manualDeskewAngleSetExternally(double degrees);
protected:
	virtual void paintOverImage(QPainter& painter);
	
	virtual void wheelEvent(QWheelEvent* event);
	
	virtual void mousePressEvent(QMouseEvent* event);
	
	virtual void mouseReleaseEvent(QMouseEvent* event);
	
	virtual void mouseMoveEvent(QMouseEvent* event);
	
	virtual void hideEvent(QHideEvent* event);
private:
	enum State { DEFAULT_STATE, DRAGGING_LEFT_HANDLE, DRAGGING_RIGHT_HANDLE };
	
	static double const m_maxRotationDeg;
	
	static double const m_maxRotationSin;
	
	static int const m_cellSize;
	
	QPointF getImageRotationOrigin() const;
	
	QRectF getRotationArcSquare() const;
	
	std::pair<QPointF, QPointF> getRotationHandles(
		QRectF const& arc_square) const;
	
	QPixmap m_imgRotationHandle;
	
	QRectF m_leftRotationHandle;
	
	QRectF m_rightRotationHandle;
	
	QString m_dragHandleStatusTip;
	
	double m_mouseVertOffset;
	
	State m_state;
};

} // namespace deskew

#endif
