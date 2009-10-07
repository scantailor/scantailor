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

#ifndef PAGE_SPLIT_SPLIT_LINE_OBJECT_H_
#define PAGE_SPLIT_SPLIT_LINE_OBJECT_H_

#include "DraggableObject.h"

namespace page_split
{

class SplitLineObject : public DraggableObject
{
protected:
	virtual Proximity lineProximity
		(QPointF const& widget_mouse_pos, InteractionState const& interaction) const = 0;

	virtual QPointF linePosition(InteractionState const& interaction) const = 0;

	virtual void lineMoveRequest(QPointF const& widget_pos) = 0;

	virtual Proximity proximity(
			QPointF const& widget_mouse_pos, InteractionState const& interaction) {
		return lineProximity(widget_mouse_pos, interaction);
	}

	virtual QPointF position(InteractionState const& interaction) const {
		return linePosition(interaction);
	}

	virtual void moveRequest(QPointF const& widget_pos) {
		return lineMoveRequest(widget_pos);
	}
};

} // namespace page_split

#endif
