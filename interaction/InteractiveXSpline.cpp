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

#include "InteractiveXSpline.h"
#include "Proximity.h"
#include "VecNT.h"
#include "MatrixCalc.h"
#include <QCursor>
#include <QMouseEvent>
#include <Qt>
#ifndef Q_MOC_RUN
#include <boost/bind.hpp>
#endif

struct InteractiveXSpline::NoOp
{
	void operator()() const {}
};

struct InteractiveXSpline::IdentTransform
{
	QPointF operator()(QPointF const& pt) const { return pt; }
};

InteractiveXSpline::InteractiveXSpline()
:	m_modifiedCallback(NoOp()),
	m_dragFinishedCallback(NoOp()),
	m_fromStorage(IdentTransform()),
	m_toStorage(IdentTransform()),
	m_curveProximityT(),
	m_lastProximity(false)
{
	m_curveProximity.setProximityCursor(Qt::PointingHandCursor);
	m_curveProximity.setProximityStatusTip(tr("Click to create a new control point."));
}

void
InteractiveXSpline::setSpline(XSpline const& spline)
{
	int const num_control_points = spline.numControlPoints();

	XSpline new_spline(spline);
	boost::scoped_array<ControlPoint> new_control_points(
		new ControlPoint[num_control_points]
	);

	for (int i = 0; i < num_control_points; ++i) {
		new_control_points[i].point.setPositionCallback(
			boost::bind(&InteractiveXSpline::controlPointPosition, this, i)
		);
		new_control_points[i].point.setMoveRequestCallback(
			boost::bind(&InteractiveXSpline::controlPointMoveRequest, this, i, _1)
		);
		new_control_points[i].point.setDragFinishedCallback(
			boost::bind(&InteractiveXSpline::dragFinished, this)
		);

		if (i == 0 || i == num_control_points - 1) {
			// Endpoints can't be deleted.
			new_control_points[i].handler.setProximityStatusTip(tr("This point can be dragged."));
		} else {
			new_control_points[i].handler.setProximityStatusTip(tr("Drag this point or delete it by pressing Del or D."));
		}
		new_control_points[i].handler.setInteractionCursor(Qt::BlankCursor);
		new_control_points[i].handler.setObject(&new_control_points[i].point);

		makeLastFollower(new_control_points[i].handler);
	}

	m_spline.swap(new_spline);
	m_controlPoints.swap(new_control_points);

	m_modifiedCallback();
}

void
InteractiveXSpline::setStorageTransform(
	Transform const& from_storage, Transform const& to_storage)
{
	m_fromStorage = from_storage;
	m_toStorage = to_storage;
}

void
InteractiveXSpline::setModifiedCallback(ModifiedCallback const& callback)
{
	m_modifiedCallback = callback;
}

void
InteractiveXSpline::setDragFinishedCallback(DragFinishedCallback const& callback)
{
	m_dragFinishedCallback = callback;
}

bool
InteractiveXSpline::curveIsProximityLeader(
	InteractionState const& state, QPointF* pt, double* t) const
{
	if (state.proximityLeader(m_curveProximity)) {
		if (pt) {
			*pt = m_curveProximityPointScreen;
		}
		if (t) {
			*t = m_curveProximityT;
		}
		return true;
	}

	return false;
}

void
InteractiveXSpline::onProximityUpdate(
	QPointF const& screen_mouse_pos, InteractionState& interaction)
{
	m_curveProximityPointStorage = m_spline.pointClosestTo(
		m_toStorage(screen_mouse_pos), &m_curveProximityT
	);
	m_curveProximityPointScreen = m_fromStorage(m_curveProximityPointStorage);
	
	Proximity const proximity(screen_mouse_pos, m_curveProximityPointScreen);
	interaction.updateProximity(m_curveProximity, proximity, -1);
}

void
InteractiveXSpline::onMouseMoveEvent(
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

void
InteractiveXSpline::onMousePressEvent(
	QMouseEvent* event, InteractionState& interaction)
{
	if (interaction.captured()) {
		return;
	}

	if (interaction.proximityLeader(m_curveProximity)) {
		int const segment = int(m_curveProximityT * m_spline.numSegments());
		int const pnt_idx = segment + 1;
		
		m_spline.insertControlPoint(pnt_idx, m_curveProximityPointStorage, 1);
		setSpline(m_spline);

		m_controlPoints[pnt_idx].handler.forceEnterDragState(interaction, event->pos());
		event->accept();

		interaction.setRedrawRequested(true);
	}
}

void
InteractiveXSpline::onKeyPressEvent(
	QKeyEvent* event, InteractionState& interaction)
{
	if (interaction.captured()) {
		return;
	}

	switch (event->key()) {
		case Qt::Key_Delete:
		case Qt::Key_D:
			int const num_control_points = m_spline.numControlPoints();
			// Check if one of our control points is a proximity leader.
			// Note that we don't consider the endpoints.
			for (int i = 1; i < num_control_points - 1; ++i) {
				if (m_controlPoints[i].handler.proximityLeader(interaction)) {
					m_spline.eraseControlPoint(i);
					setSpline(m_spline);
					interaction.setRedrawRequested(true);
					event->accept();
					break;
				}
			}
			break;
	}

	
}

QPointF
InteractiveXSpline::controlPointPosition(int idx) const
{
	return m_fromStorage(m_spline.controlPointPosition(idx));
}

void
InteractiveXSpline::controlPointMoveRequest(int idx, QPointF const& pos)
{
	QPointF const storage_pt(m_toStorage(pos));

	int const num_control_points = m_spline.numControlPoints();
	if (idx > 0 && idx < num_control_points - 1) {
		// A midpoint - just move it.
		m_spline.moveControlPoint(idx, storage_pt);
	} else {
		// An endpoint was moved.  Instead of moving it on its own,
		// we are going to rotate and / or scale all of the points
		// relative to the opposite endpoint.
		int const origin_idx = idx == 0 ? num_control_points - 1 : 0;
		QPointF const origin(m_spline.controlPointPosition(origin_idx));
		QPointF const old_pos(m_spline.controlPointPosition(idx));
		if (Vec2d(old_pos - origin).squaredNorm() > 1.0) {
			// rotationAndScale() would throw an exception if old_pos == origin.
			Vec4d const mat(rotationAndScale(old_pos - origin, storage_pt - origin));
			for (int i = 0; i < num_control_points; ++i) {
				Vec2d pt(m_spline.controlPointPosition(i) - origin);
				MatrixCalc<double> mc;
				(mc(mat, 2, 2)*mc(pt, 2, 1)).write(pt);
				m_spline.moveControlPoint(i, pt + origin);
			}
		} else {
			// Move the endpoint and distribute midpoints uniformly.
			QLineF const line(origin, storage_pt);
			double const scale = 1.0 / (num_control_points - 1);
			for (int i = 0; i < num_control_points; ++i) {
				m_spline.moveControlPoint(i, line.pointAt(i * scale));
			}
		}
	}

	m_modifiedCallback();
}

void
InteractiveXSpline::dragFinished()
{
	m_dragFinishedCallback();
}

Vec4d
InteractiveXSpline::rotationAndScale(QPointF const& from, QPointF const& to)
{
	Vec4d A;
	A[0] = from.x();
	A[1] = from.y();
	A[2] = from.y();
	A[3] = -from.x();

	Vec2d B(to.x(), to.y());

	Vec2d x;
	MatrixCalc<double> mc;
	mc(A, 2, 2).solve(mc(B, 2, 1)).write(x);
	
	A[0] = x[0];
	A[1] = -x[1];
	A[2] = x[1];
	A[3] = x[0];
	return A;
}
