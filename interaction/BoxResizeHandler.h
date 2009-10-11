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

#ifndef BOX_RESIZE_HANDLER_H_
#define BOX_RESIZE_HANDLER_H_

#include "NonCopyable.h"
#include "InteractionHandler.h"
#include "InteractionState.h"
#include "ObjectDragHandler.h"
#include "DraggablePoint.h"
#include "DraggableLineSegment.h"
#include <QPointF>
#include <QRectF>
#include <QSizeF>

class QCursor;
class QString;

class BoxResizeHandler :
	public InteractionHandler,
	private DraggablePoint,
	private DraggableLineSegment
{
	DECLARE_NON_COPYABLE(BoxResizeHandler)
public:
	/**
	 * \param base_proximity_priority The proximity priority to assign
	 *        to edges.  Corners will get base_proximity_priority + 1.
	 *
	 * \see InteractionState::updateProximity()
	 */
	BoxResizeHandler(int base_proximity_priority = 0);

	void setMinBoxSize(QSizeF const& min_size) { m_minBoxSize = min_size; }

	QSizeF const& minBoxSize() const { return m_minBoxSize; }

	void setProximityStatusTip(QString const& tip);

	void setInteractionStatusTip(QString const& tip);

	bool interactionInProgress(InteractionState const& interaction) const;

	/**
	 * \see InteractionState::proximityLeader()
	 */
	bool proximityLeader(InteractionState const& interaction) const;
protected:
	virtual QPointF pointPosition(
		ObjectDragHandler const* handler, InteractionState const& interaction) const;

	virtual void pointMoveRequest(
		ObjectDragHandler const* handler, QPointF const& widget_pos,
		InteractionState const& interaction);

	virtual QLineF lineSegment(
		ObjectDragHandler const* handler, InteractionState const& interaction) const;

	virtual QPointF lineSegmentPosition(
		ObjectDragHandler const* handler, InteractionState const& interaction) const;

	virtual void lineSegmentMoveRequest(
		ObjectDragHandler const* handler, QPointF const& widget_pos,
		InteractionState const& interaction);

	virtual void dragFinished(ObjectDragHandler const* handler);

	virtual QRectF boxPosition(
		BoxResizeHandler const* handler, InteractionState const& interaction) const = 0;

	virtual void boxResizeRequest(
		BoxResizeHandler const* handler, QRectF const& rect,
		InteractionState const& interaction) = 0;

	virtual void boxResizeFinished(BoxResizeHandler const* handler) = 0;
private:
	ObjectDragHandler m_cornerHandlers[4]; // Top-left, then clockwise.
	ObjectDragHandler m_edgeHandlers[4]; // Top edge, then clockwise.
	QSizeF m_minBoxSize;
};

#endif
