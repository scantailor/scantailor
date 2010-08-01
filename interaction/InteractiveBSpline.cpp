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

#include "InteractiveBSpline.h"
#include "Proximity.h"
#include "VecNT.h"
#include "imageproc/MatrixCalc.h"
#include <QCursor>
#include <Qt>
#include <boost/bind.hpp>
#include <algorithm>

using namespace imageproc;

struct InteractiveBSpline::NoOp
{
	void operator()() const {}
};

struct InteractiveBSpline::IdentTransform
{
	QPointF operator()(QPointF const& pt) const { return pt; }
};

InteractiveBSpline::InteractiveBSpline()
:	m_modifiedCallback(NoOp()),
	m_dragFinishedCallback(NoOp()),
	m_fromStorage(IdentTransform()),
	m_toStorage(IdentTransform()),
	m_lastProximity(false)
{
	m_curveProximity.setProximityCursor(Qt::PointingHandCursor);
}

void
InteractiveBSpline::setSpline(imageproc::CubicBSpline const& spline)
{
	CubicBSpline new_spline(spline);
	new_spline.makeValid();
	SegmentList new_segments;

	for (int todo = new_spline.numSegments(); todo > 0; --todo) {
		SegmentList::iterator const it(
			new_segments.insert(new_segments.end(), BezierSegment())
		);
		for (int i = 0; i < 4; ++i) {
			it->point[i].setPositionCallback(
				boost::bind(&InteractiveBSpline::bezierPointPosition, this, it, i)
			);
			it->point[i].setMoveRequestCallback(
				boost::bind(&InteractiveBSpline::bezierPointMoveRequest, this, it, i, _1)
			);
			it->point[i].setDragFinishedCallback(
				boost::bind(&InteractiveBSpline::dragFinished, this)
			);
			if (i == 1 && i == 2) {
				// Intermediate bezier point are considered to be "atop" the spline.
				it->point[i].setProximityPriority(1);
			}

			it->handler[i].setInteractionCursor(Qt::BlankCursor);
			it->handler[i].setObject(&it->point[i]);

			makeLastFollower(it->handler[i]);
		}
	}

	m_spline.swap(new_spline);
	m_bezierSegments.swap(new_segments);
}

void
InteractiveBSpline::setStorageTransform(
	Transform const& from_storage, Transform const& to_storage)
{
	m_fromStorage = from_storage;
	m_toStorage = to_storage;
}

void
InteractiveBSpline::setModifiedCallback(ModifiedCallback const& callback)
{
	m_modifiedCallback = callback;
}

void
InteractiveBSpline::setDragFinishedCallback(DragFinishedCallback const& callback)
{
	m_dragFinishedCallback = callback;
}

bool
InteractiveBSpline::curveHighlighted(InteractionState const& state, QPointF* pt) const
{
	if (state.proximityLeader(m_curveProximity)) {
		if (pt) {
			*pt = m_curveProximityPoint;
		}
		return true;
	}

	return false;
}

void
InteractiveBSpline::onProximityUpdate(
	QPointF const& screen_mouse_pos, InteractionState& interaction)
{
	m_curveProximityPoint = m_fromStorage(
		m_spline.pointClosestTo(m_toStorage(screen_mouse_pos))
	);
	
	Proximity const proximity(screen_mouse_pos, m_curveProximityPoint);
	interaction.updateProximity(m_curveProximity, proximity, -1);
}

void
InteractiveBSpline::onMouseMoveEvent(
	QMouseEvent*, InteractionState& interaction)
{
	if (interaction.proximityLeader(m_curveProximity)) {
		// We need to redraw the highlighted point.
		interaction.setRedrawRequested(true);
		m_lastProximity = true;
	} else if (m_lastProximity) {
		// In this case we need to un-draw the highlighted point.
		interaction.setRedrawRequested(true);
		m_lastProximity = false;
	}
}

QPointF
InteractiveBSpline::bezierPointPosition(
	SegmentList::const_iterator it, size_t bezier_point_idx) const
{
	size_t const segment = std::distance(m_bezierSegments.begin(), it);
	QPointF const pt(m_spline.toBezierSegment(segment)[bezier_point_idx]);
	return m_fromStorage(pt);
}

void
InteractiveBSpline::bezierPointMoveRequest(
	SegmentList::iterator it, size_t bezier_point_idx, QPointF const& pos)
{
	size_t const segment = std::distance(m_bezierSegments.begin(), it);
	QPointF const storage_pt(m_toStorage(pos));

	if ((bezier_point_idx == 0 && segment == 0) ||
		(bezier_point_idx == 3 && segment == m_spline.numSegments() - 1)) {
		// An endpoint was moved.  Instead of moving it on its own,
		// we are going to rotate and / or scale all of the points
		// relative to the opposite endpoint.
		QPointF const origin(m_spline.eval(bezier_point_idx == 0 ? m_spline.maxT() : 0.0));
		boost::array<QPointF, 4> const bezier(m_spline.toBezierSegment(segment));
		Vec4d const mat(rotationAndScale(bezier[bezier_point_idx] - origin, storage_pt - origin));
		int const num_control_points = m_spline.controlPoints().size();
		for (int i = 0; i < num_control_points; ++i) {
			Vec2d pt(m_spline.controlPoints()[i] - origin);
			MatrixCalc<double> mc;
			(mc(2, 2, mat)*mc(2, 1, pt)).write(pt);
			m_spline.moveControlPoint(i, pt + origin);
		}
	} else {
		m_spline.moveBezierPoint(segment, bezier_point_idx, storage_pt);
	}

	m_modifiedCallback();
}

void
InteractiveBSpline::dragFinished()
{
	m_dragFinishedCallback();
}

Vec4d
InteractiveBSpline::rotationAndScale(QPointF const& from, QPointF const& to)
{
	Vec4d A;
	A[0] = from.x();
	A[1] = from.y();
	A[2] = from.y();
	A[3] = -from.x();

	Vec2d B(to.x(), to.y());

	Vec2d x;
	MatrixCalc<double> mc;
	mc(2, 2, A).solve(mc(2, 1, B)).write(x);
	
	A[0] = x[0];
	A[1] = -x[1];
	A[2] = x[1];
	A[3] = x[0];
	return A;
}
