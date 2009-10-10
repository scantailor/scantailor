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

class ImageViewBase;

class DraggableLineSegment : public DraggableObject
{
public:
	DraggableLineSegment(int id, int proximity_priority = 0);

	virtual int proximityPriority() const;

	virtual Proximity proximity(
		QPointF const& widget_mouse_pos, InteractionState const& interaction);

	virtual QPointF position(InteractionState const& interaction) const;

	virtual void moveRequest(QPointF const& widget_pos);
protected:
	virtual QLineF lineSegment(int id, InteractionState const& interaction) const = 0;

	virtual QPointF lineSegmentPosition(int id, InteractionState const& interaction) const = 0;

	virtual void lineSegmentMoveRequest(int id, QPointF const& widget_pos) = 0;
private:
	int m_id;
	int m_proximityPriority;
};


template<int T>
class TaggedDraggableLineSegment : public DraggableLineSegment
{
public:
	enum { Tag = T };

	TaggedDraggableLineSegment(int proximity_priority = 0)
			: DraggableLineSegment(T, proximity_priority) {}
};

#endif
