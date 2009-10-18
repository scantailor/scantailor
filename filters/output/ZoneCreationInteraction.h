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

#ifndef OUTPUT_ZONE_CREATION_INTERACTION_H_
#define OUTPUT_ZONE_CREATION_INTERACTION_H_

#include "InteractionHandler.h"
#include "InteractionState.h"
#include "BasicSplineVisualizer.h"
#include "Spline.h"
#include <QPointF>
#include <vector>

class ImageViewBase;

namespace output
{

class ZoneCreationInteraction : public InteractionHandler
{
public:
	ZoneCreationInteraction(
		ImageViewBase& image_view, std::vector<Spline::Ptr>& splines,
		InteractionState& interaction, QPointF const& first_image_point);
protected:
	virtual void onPaint(QPainter& painter, InteractionState const& interaction);

	virtual void onKeyPressEvent(QKeyEvent* event, InteractionState& interaction);

	virtual void onMouseReleaseEvent(QMouseEvent* event, InteractionState& interaction);

	virtual void onMouseMoveEvent(QMouseEvent* event, InteractionState& interaction);
private:
	ImageViewBase& m_rImageView;
	std::vector<Spline::Ptr>& m_rSplines;
	BasicSplineVisualizer m_visualizer;
	InteractionState::Captor m_interaction;
	Spline::Ptr m_ptrSpline;
	QPointF m_nextVertexImagePos;
};

} // namespace output

#endif
