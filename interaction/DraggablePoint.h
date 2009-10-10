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

#ifndef DRAGGABLE_POINT_H_
#define DRAGGABLE_POINT_H_

#include "DraggableObject.h"
#include <QPointF>

class ImageViewBase;

class DraggablePoint : public DraggableObject
{
public:
	DraggablePoint(int id, int proximity_priority = 1);

	/**
	 * Returns the hit area radius, with zero indicating the global
	 * proximity threshold of InteractionState is to be used.
	 */
	double hitRadius() const { return m_hitAreaRadius; }

	void setHitRadius(double radius) { m_hitAreaRadius = radius; }

	virtual Proximity proximityThreshold(InteractionState const& interaction) const;

	virtual int proximityPriority() const;

	virtual Proximity proximity(
		QPointF const& widget_mouse_pos, InteractionState const& interaction);

	virtual QPointF position(InteractionState const& interaction) const;

	virtual void moveRequest(QPointF const& widget_pos);
protected:
	virtual QPointF pointPosition(int id, InteractionState const& interaction) const = 0;

	virtual void pointMoveRequest(int id, QPointF const& widget_pos) = 0;
private:
	double m_hitAreaRadius;
	int m_id;
	int m_proximityPriority;
};


template<int T>
class TaggedDraggablePoint : public DraggablePoint
{
public:
	enum { Tag = T };

	TaggedDraggablePoint(int proximity_priority = 1)
			: DraggablePoint(T, proximity_priority) {}
};

#endif
