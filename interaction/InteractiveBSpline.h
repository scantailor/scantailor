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

#ifndef INTERACTIVE_BSPLINE_H_
#define INTERACTIVE_BSPLINE_H_

#include "imageproc/CubicBSpline.h"
#include "DraggablePoint.h"
#include "ObjectDragHandler.h"
#include "InteractionState.h"
#include "VecNT.h"
#include <QPointF>
#include <boost/function.hpp>
#include <list>
#include <stddef.h>

class InteractiveBSpline : public InteractionHandler
{
public:
	typedef boost::function<QPointF (QPointF const&)> Transform;
	typedef boost::function<void()> ModifiedCallback;
	typedef boost::function<void()> DragFinishedCallback;

	InteractiveBSpline();

	void setSpline(imageproc::CubicBSpline const& spline);

	imageproc::CubicBSpline const& spline() const { return m_spline; }

	void setStorageTransform(Transform const& from_storage, Transform const& to_storage);

	void setModifiedCallback(ModifiedCallback const& callback);

	void setDragFinishedCallback(DragFinishedCallback const& callback);

	/**
	 * \brief Returns true if the curve is a proximity leader.
	 *
	 * If requested, the point on the curve closest to the cursor
	 * (in widget coordinates) is returned.
	 */
	bool curveHighlighted(InteractionState const& state, QPointF* pt) const;
protected:
	virtual void onProximityUpdate(
		QPointF const& screen_mouse_pos, InteractionState& interaction);

	virtual void onMouseMoveEvent(
		QMouseEvent* event, InteractionState& interaction);
private:
	struct NoOp;
	struct IdentTransform;

	struct BezierSegment
	{
		DraggablePoint point[4];
		ObjectDragHandler handler[4];

		BezierSegment() {}

		// Fake copy construction as construction without copying.
		// Necessary because ObjectDragHandler is non copyable.
		BezierSegment(BezierSegment const&) {}
	};

	typedef std::list<BezierSegment> SegmentList;

	QPointF bezierPointPosition(
		SegmentList::const_iterator it, size_t bezier_point_idx) const;

	void bezierPointMoveRequest(
		SegmentList::iterator it, size_t bezier_point_idx, QPointF const& pos);

	void dragFinished();

	static Vec4d rotationAndScale(QPointF const& from, QPointF const& to);

	ModifiedCallback m_modifiedCallback;
	DragFinishedCallback m_dragFinishedCallback;
	Transform m_fromStorage;
	Transform m_toStorage;
	imageproc::CubicBSpline m_spline;
	SegmentList m_bezierSegments;
	InteractionState::Captor m_curveProximity;
	QPointF m_curveProximityPoint;
	bool m_lastProximity;
};

#endif
