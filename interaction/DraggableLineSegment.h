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

#ifndef DRAGGABLE_LINE_SEGMENT_H_
#define DRAGGABLE_LINE_SEGMENT_H_

#include "DraggableObject.h"
#include <QPointF>
#include <QLineF>
#include <boost/function.hpp>

class ObjectDragHandler;

class DraggableLineSegment : public DraggableObject
{
public:
	typedef boost::function<
		QLineF ()
	> PositionCallback;

	typedef boost::function<
//Blue_Dewarp_Line_Vert_Drag
		//void (QLineF const& line)
		void (QLineF const& line, Qt::KeyboardModifiers mask)
	> MoveRequestCallback;

	DraggableLineSegment();

	void setProximityPriority(int priority) { m_proximityPriority = priority; }

	virtual int proximityPriority() const;

	virtual Proximity proximity(QPointF const& mouse_pos);

	virtual void dragInitiated(QPointF const& mouse_pos);
//Blue_Dewarp_Line_Vert_Drag
	//virtual void dragContinuation(QPointF const& mouse_pos);
	virtual void dragContinuation(QPointF const& mouse_pos, Qt::KeyboardModifiers mask);
	void setPositionCallback(PositionCallback const& callback) {
		m_positionCallback = callback;
	}

	void setMoveRequestCallback(MoveRequestCallback const& callback) {
		m_moveRequestCallback = callback;
	}
protected:
	virtual QLineF lineSegmentPosition() const {
		return m_positionCallback();
	}
//Blue_Dewarp_Line_Vert_Drag
	//virtual void lineSegmentMoveRequest(QLineF const& line) {
	virtual void lineSegmentMoveRequest(QLineF const& line, Qt::KeyboardModifiers mask) { 
		//m_moveRequestCallback(line);
		m_moveRequestCallback(line, mask);
	}
private:
	PositionCallback m_positionCallback;
	MoveRequestCallback m_moveRequestCallback;
	QPointF m_initialMousePos;
	QLineF m_initialLinePos;
	int m_proximityPriority;
};

#endif
