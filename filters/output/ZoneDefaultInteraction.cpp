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

#include "ZoneDefaultInteraction.h"
#include "ZoneCreationInteraction.h"
#include "ZoneVertexDragInteraction.h"
#include "ImageViewBase.h"
#include <QTransform>
#include <QPolygon>
#include <QPointF>
#include <QPen>
#include <QPainter>
#include <QPainterPath>
#include <QColor>
#include <QLinearGradient>
#include <Qt>
#include <QMouseEvent>
#include <boost/foreach.hpp>
#include <assert.h>

namespace output
{

ZoneDefaultInteraction::ZoneDefaultInteraction(
	ImageViewBase& image_view, std::vector<Spline::Ptr>& splines)
:	m_rImageView(image_view),
	m_rSplines(splines)
{
}

void
ZoneDefaultInteraction::onPaint(QPainter& painter, InteractionState const& interaction)
{
	painter.setWorldMatrixEnabled(false);
	painter.setRenderHint(QPainter::Antialiasing);

	QTransform const to_screen(m_rImageView.imageToVirtual() * m_rImageView.virtualToWidget());

	BOOST_FOREACH(Spline::Ptr const& spline, m_rSplines) {
		m_visualizer.prepareForSpline(painter, spline);
		QPolygonF points;

		if (!interaction.captured() && interaction.proximityLeader(m_vertexProximity)
				&& spline == m_ptrNearestVertexSpline) {
			SplineVertex::Ptr vertex(m_ptrNearestVertex->next(SplineVertex::LOOP));
			for (; vertex != m_ptrNearestVertex; vertex = vertex->next(SplineVertex::LOOP)) {
				points.push_back(to_screen.map(vertex->point()));
			}
			painter.drawPolyline(points);
		} else if (!interaction.captured() && interaction.proximityLeader(m_segmentProximity)
				&& spline == m_ptrNearestSegmentSpline) {
			SplineVertex::Ptr vertex(m_nearestSegment.prev);
			do {
				vertex = vertex->next(SplineVertex::LOOP);
				points.push_back(to_screen.map(vertex->point()));
			} while (vertex != m_nearestSegment.prev);
			painter.drawPolyline(points);
		} else {
			m_visualizer.drawSpline(painter, to_screen, spline);
		}
	}

	if (interaction.proximityLeader(m_vertexProximity)) {
		// Draw the two adjacent edges in gradient red-to-orange.
		QLinearGradient gradient; // From inactive to active point.
		gradient.setColorAt(0.0, m_visualizer.solidColor());
		gradient.setColorAt(1.0, m_visualizer.highlightDarkColor());

		QPen pen(painter.pen());

		QPointF const prev(to_screen.map(m_ptrNearestVertex->prev(SplineVertex::LOOP)->point()));
		QPointF const pt(to_screen.map(m_ptrNearestVertex->point()));
		QPointF const next(to_screen.map(m_ptrNearestVertex->next(SplineVertex::LOOP)->point()));

		gradient.setStart(prev);
		gradient.setFinalStop(pt);
		pen.setBrush(gradient);
		painter.setPen(pen);
		painter.drawLine(prev, pt);

		gradient.setStart(next);
		pen.setBrush(gradient);
		painter.setPen(pen);
		painter.drawLine(next, pt);

		// Visualize the highlighted vertex.
		QPointF const screen_vertex(to_screen.map(m_ptrNearestVertex->point()));
		m_visualizer.drawVertex(painter, screen_vertex, m_visualizer.highlightBrightColor());
	} else if (interaction.proximityLeader(m_segmentProximity)) {
		QLineF const line(to_screen.map(m_nearestSegment.toLine()));

		// Draw the highglighed edge in orange.
		QPen pen(painter.pen());
		pen.setColor(m_visualizer.highlightDarkColor());
		painter.setPen(pen);
		painter.drawLine(line);

		m_visualizer.drawVertex(painter, m_screenPointOnSegment, m_visualizer.highlightBrightColor());
	} else {
		m_visualizer.drawVertex(painter, m_screenMousePos, m_visualizer.solidColor());
	}
}

void
ZoneDefaultInteraction::onProximityUpdate(QPointF const& mouse_pos, InteractionState& interaction)
{
	m_screenMousePos = mouse_pos;

	QTransform const to_screen(m_rImageView.imageToVirtual() * m_rImageView.virtualToWidget());
	QTransform const from_screen(m_rImageView.widgetToVirtual() * m_rImageView.virtualToImage());
	QPointF const image_mouse_pos(from_screen.map(mouse_pos));

	m_ptrNearestVertex.reset();
	m_ptrNearestVertexSpline.reset();
	m_nearestSegment = SplineSegment();
	m_ptrNearestSegmentSpline.reset();

	Proximity best_vertex_proximity;
	Proximity best_segment_proximity;

	bool has_zone_under_mouse = false;

	BOOST_FOREACH(Spline::Ptr const& spline, m_rSplines) {
		if (!has_zone_under_mouse) {
			QPainterPath path;
			path.setFillRule(Qt::WindingFill);
			path.addPolygon(spline->toPolygon());
			has_zone_under_mouse = path.contains(image_mouse_pos);
		}

		// Process vertices.
		for (SplineVertex::Ptr vert(spline->firstVertex());
				vert; vert = vert->next(SplineVertex::NO_LOOP)) {

			Proximity const proximity(mouse_pos, to_screen.map(vert->point()));
			if (proximity < best_vertex_proximity) {
				m_ptrNearestVertex = vert;
				m_ptrNearestVertexSpline = spline;
				best_vertex_proximity = proximity;
			}
		}

		// Process segments.
		for (Spline::SegmentIterator it(*spline); it.hasNext(); ) {
			SplineSegment const segment(it.next());
			QLineF const line(to_screen.map(segment.toLine()));
			QPointF point_on_segment;
			Proximity const proximity(Proximity::pointAndLineSegment(mouse_pos, line, &point_on_segment));
			if (proximity < best_segment_proximity) {
				m_nearestSegment = segment;
				m_ptrNearestSegmentSpline = spline;
				best_segment_proximity = proximity;
				m_screenPointOnSegment = point_on_segment;
			}
		}
	}

	interaction.updateProximity(m_vertexProximity, best_vertex_proximity, 1);
	interaction.updateProximity(m_segmentProximity, best_segment_proximity, 0);
#if 0
	if (m_ptrHighlightedVertex) {
		// Vertex selection takes preference over edge selection.
		m_highlightedEdge = Edge();
		m_ptrHighlightedSpline = highlighted_vertex_spline;
		ensureStatusTip(tr("Drag the vertex."));
	} else if (highlighted_edge_spline) {
		m_ptrHighlightedSpline = highlighted_edge_spline;
		ensureStatusTip(tr("Click to create a new vertex here."));
	} else if (has_zone_under_mouse) {
		ensureStatusTip(tr("Right click to edit zone properties."));
	} else {
		ensureStatusTip(tr("Click to start creating a new picture zone."));
	}
#endif
}

void
ZoneDefaultInteraction::onMousePressEvent(QMouseEvent* event, InteractionState& interaction)
{
	if (interaction.captured()) {
		return;
	}
	if (event->button() != Qt::LeftButton) {
		return;
	}

	if (interaction.proximityLeader(m_vertexProximity)) {
		makePeerPreceeder(
			*new ZoneVertexDragInteraction(
				m_rImageView, m_rSplines, m_ptrNearestVertexSpline,
				m_ptrNearestVertex, interaction, event->pos() + QPointF(0.5, 0.5)
			)
		);
		delete this;
	} else if (interaction.proximityLeader(m_segmentProximity)) {
		//Vertex::Ptr vertex(m_highlightedEdge.splitAt(fromScreen().map(m_screenPointOnEdge)));
		//handlerPushFront(new VertexDragHandler(m_rOwner, m_ptrHighlightedSpline, vertex))->mousePressEvent(event);
		//delete this;
	}
}

void
ZoneDefaultInteraction::onMouseReleaseEvent(QMouseEvent* event, InteractionState& interaction)
{
	if (interaction.captured()) {
		return;
	}
	if (event->button() != Qt::LeftButton) {
		return;
	}

	makePeerFollower(
		*new ZoneCreationInteraction(
			m_rImageView, m_rSplines, interaction, event->pos() + QPointF(0.5, 0.5)
		)
	);
	delete this;
}

void
ZoneDefaultInteraction::onMouseMoveEvent(QMouseEvent* event, InteractionState& interaction)
{
	QTransform const to_screen(m_rImageView.imageToVirtual() * m_rImageView.virtualToWidget());

	m_screenMousePos = to_screen.map(event->pos() + QPointF(0.5, 0.5));
	//update();
	m_rImageView.update();
}

void
ZoneDefaultInteraction::onContextMenuEvent(QContextMenuEvent* event, InteractionState& interaction)
{
	QTransform const to_screen(m_rImageView.imageToVirtual() * m_rImageView.virtualToWidget());
	QTransform const from_screen(m_rImageView.widgetToVirtual() * m_rImageView.virtualToImage());

	// Find splines containing this point.
	std::vector<unsigned> splines;
	for (unsigned i = 0; i < m_rSplines.size(); ++i) {
		QPainterPath path;
		path.setFillRule(Qt::WindingFill);
		path.addPolygon(m_rSplines[i]->toPolygon());
		if (path.contains(from_screen.map(event->pos() + QPointF(0.5, 0.5)))) {
			splines.push_back(i);
		}
	}

	if (splines.empty()) {
		return;
	}

	event->accept();

	//handlerPushFront(new ContextMenuHandler(m_rOwner, splines, event->globalPos()));
	//delete this;
}

} // namespace output
