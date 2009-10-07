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

#ifndef DRAGGABLE_PIXMAP_H_
#define DRAGGABLE_PIXMAP_H_

#include "DraggableObject.h"
#include <QPointF>
#include <QPixmap>

class ImageViewBase;

class DraggablePixmap : public DraggableObject
{
public:
	DraggablePixmap(int id, QPixmap const& pixmap, int proximity_priority);

	double handleRadius() const { return 0.5 * m_pixmap.width(); }

	double hitAreaRadius() const { return m_hitAreaRadius; }

	void setHitAreaRadius(double radius) { m_hitAreaRadius = radius; }

	virtual void paint(QPainter& painter, InteractionState const& interaction);

	virtual Proximity proximityThreshold(InteractionState const& interaction) const;

	virtual int proximityPriority() const;

	virtual Proximity proximity(
		QPointF const& widget_mouse_pos, InteractionState const& interaction);

	virtual QPointF position(InteractionState const& interaction) const;

	virtual void moveRequest(QPointF const& widget_pos);
protected:
	virtual bool isPixmapToBeDrawn(int id, InteractionState const& interaction) const = 0;

	virtual QPointF pixmapPosition(int id, InteractionState const& interaction) const = 0;

	virtual void pixmapMoveRequest(int id, QPointF const& widget_pos) = 0;
private:
	QPixmap m_pixmap;
	double m_hitAreaRadius;
	int m_id;
	int m_proximityPriority;
};


template<int Tag>
class TaggedDraggablePixmap : public DraggablePixmap
{
public:
	TaggedDraggablePixmap(QPixmap const& pixmap, int proximity_priority)
			: DraggablePixmap(Tag, pixmap, proximity_priority) {}
};

#endif
