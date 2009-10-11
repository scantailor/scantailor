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

#ifndef DRAGGABLE_OBJECT_H_
#define DRAGGABLE_OBJECT_H_

#include "InteractionState.h"
#include "Proximity.h"

class ObjectDragHandler;
class QPointF;
class QPainter;

class DraggableObject
{
public:
	virtual ~DraggableObject() {}

	virtual void paint(QPainter& painter, InteractionState const& interaction) {}

	/**
	 * \return The maximum distance from the object (in widget coordinates) that
	 *         still allows to initiate a dragging operation.
	 */
	virtual Proximity proximityThreshold(
			ObjectDragHandler const*, InteractionState const& interaction) const {
		return interaction.proximityThreshold();
	}

	/**
	 * Sometimes a more distant object should be selected for dragging in favor of
	 * a closer one.  Consider for example a line segment with handles at its endpoints.
	 * In this example, you would assign higher priority to those handles.
	 */
	virtual int proximityPriority(ObjectDragHandler const* handler) const { return 0; }

	/**
	 * \return The proximity from the mouse position in widget coordinates to
	 *         any draggable part of the object.
	 */
	virtual Proximity proximity(
		ObjectDragHandler const* handler,
		QPointF const& widget_mouse_pos, InteractionState const& interaction) = 0;

	/**
	 * \return The current position of the object in widget coordinates.
	 */
	virtual QPointF position(
		ObjectDragHandler const* handler, InteractionState const& interaction) const = 0;

	/**
	 * \brief Handles a request to move to a particular position in widget coordinates.
	 *
	 * The object is free to act as it wishes upon this request.  It may move the
	 * object to the requested position, or it may leave it where it is,
	 * or it may move it to a different position.
	 */
	virtual void moveRequest(
		ObjectDragHandler const* handler, QPointF const& widget_pos,
		InteractionState const& interaction) = 0;

	/**
	 * \brief Called when dragging is finished, that is when the mouse button is released.
	 */
	virtual void dragFinished(ObjectDragHandler const* handler) = 0;
};

#endif
