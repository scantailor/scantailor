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

#ifndef OUTPUT_ZONE_VERTEX_DRAG_INTERACTION_H_
#define OUTPUT_ZONE_VERTEX_DRAG_INTERACTION_H_

#include "BasicSplineVisualizer.h"
#include "Spline.h"
#include "InteractionHandler.h"
#include "InteractionState.h"
#include <QPointF>
#include <QCoreApplication>

class ImageViewBase;

namespace output
{

class ZoneVertexDragInteraction : public InteractionHandler
{
	Q_DECLARE_TR_FUNCTIONS(ZoneVertexDragInteraction)
public:
	ZoneVertexDragInteraction(
		ImageViewBase& image_view, std::vector<Spline::Ptr>& splines,
		Spline::Ptr const& spline, SplineVertex::Ptr const& vertex,
		InteractionState& interaction, QPointF const& screen_mouse_pos);
protected:
	virtual void onPaint(QPainter& painter, InteractionState const& interaction);

	virtual void onMouseReleaseEvent(QMouseEvent* event, InteractionState& interaction);

	virtual void onMouseMoveEvent(QMouseEvent* event, InteractionState& interaction);
private:
	void checkProximity(InteractionState const& interaction);

	ImageViewBase& m_rImageView;
	std::vector<Spline::Ptr>& m_rSplines;
	Spline::Ptr m_ptrSpline;
	SplineVertex::Ptr m_ptrVertex;
	InteractionState::Captor m_interaction;
	BasicSplineVisualizer m_visualizer;
	QPointF m_dragOffset;
};

} // namespace output

#endif
