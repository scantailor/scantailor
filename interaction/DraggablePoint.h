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

#ifndef DRAGGABLE_POINT_H_
#define DRAGGABLE_POINT_H_

#include "DraggableObject.h"
#include <QPointF>
#ifndef Q_MOC_RUN
#include <boost/function.hpp>
#endif

class DraggablePoint : public DraggableObject
{
public:
	typedef boost::function<
		QPointF ()
	> PositionCallback;

	typedef boost::function<
//begin of modified by monday2000
//Blue_Dewarp_Line_Vert_Drag
		//void (QPointF const&)
		void (QPointF const&, Qt::KeyboardModifiers mask)
//end of modified by monday2000
	> MoveRequestCallback;

	DraggablePoint();

	/**
	 * Returns the hit area radius, with zero indicating the global
	 * proximity threshold of InteractionState is to be used.
	 */
	double hitRadius() const { return m_hitAreaRadius; }

	void setHitRadius(double radius) { m_hitAreaRadius = radius; }

	virtual Proximity proximityThreshold(InteractionState const& interaction) const;

	void setProximityPriority(int priority) { m_proximityPriority = priority; }

	virtual int proximityPriority() const;

	virtual Proximity proximity(QPointF const& mouse_pos);

	virtual void dragInitiated(QPointF const& mouse_pos);
//begin of modified by monday2000
//Blue_Dewarp_Line_Vert_Drag
	//virtual void dragContinuation(QPointF const& mouse_pos);
	virtual void dragContinuation(QPointF const& mouse_pos, Qt::KeyboardModifiers mask);
//end of modified by monday2000

	void setPositionCallback(PositionCallback const& callback) {
		m_positionCallback = callback;
	}

	void setMoveRequestCallback(MoveRequestCallback const& callback) {
		m_moveRequestCallback = callback;
	}
protected:
	virtual QPointF pointPosition() const {
		return m_positionCallback();
	}
//begin of modified by monday2000
//Blue_Dewarp_Line_Vert_Drag
	//virtual void pointMoveRequest(QPointF const& widget_pos) {
	virtual void pointMoveRequest(QPointF const& widget_pos, Qt::KeyboardModifiers mask) {
		//m_moveRequestCallback(widget_pos);
		m_moveRequestCallback(widget_pos, mask);
//end of modified by monday2000
	}
private:
	PositionCallback m_positionCallback;
	MoveRequestCallback m_moveRequestCallback;
	QPointF m_pointRelativeToMouse;
	double m_hitAreaRadius;
	int m_proximityPriority;
};

#endif
