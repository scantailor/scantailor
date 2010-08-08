/*
    Scan Tailor - Interactive post-processing tool for scanned pages.
    Copyright (C)  Joseph Artsimovich <joseph.artsimovich@gmail.com>

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

#include "PolylineIntersector.h"
#include "ToLineProjector.h"
#include <math.h>

PolylineIntersector::Hint::Hint()
:	m_lastSegment(0),
	m_direction(1)
{
}

void
PolylineIntersector::Hint::update(int new_segment)
{
	m_direction = new_segment < m_lastSegment ? -1 : 1;
	m_lastSegment = new_segment;
}
	
PolylineIntersector::PolylineIntersector(std::vector<QPointF> const& polyline)
:	m_polyline(polyline),
	m_numSegments(polyline.size() - 1)
{
}

QPointF
PolylineIntersector::intersect(QLineF const& line, Hint& hint) const
{
	QLineF const normal(line.normalVector());
	
	if (intersectsSegment(normal, hint.m_lastSegment)) {
		return intersectWithSegment(line, hint.m_lastSegment);
	}

	int segment;

	// Check the next segment in direction provided by hint.
	if (intersectsSegment(normal, (segment = hint.m_lastSegment + hint.m_direction))) {
		hint.update(segment);
		return intersectWithSegment(line, segment);
	}

	// Check the next segment in opposite direction.
	if (intersectsSegment(normal, (segment = hint.m_lastSegment - hint.m_direction))) {
		hint.update(segment);
		return intersectWithSegment(line, segment);
	}

	// Does the whole polyline intersect our line?
	QPointF intersection;
	if (tryIntersectingOutsideOfPolyline(line, intersection, hint)) {
		return intersection;
	}

	// OK, let's do a binary search then.
	QPointF const origin(normal.p1());
	Vec2d const nv(normal.p2() - normal.p1());
	int left_idx = 0;
	int right_idx = m_polyline.size() - 1;
	double left_dot = nv.dot(m_polyline[left_idx] - origin);

	while (left_idx + 1 < right_idx) {
		int const mid_idx = (left_idx + right_idx) >> 1;
		double const mid_dot = nv.dot(m_polyline[mid_idx] - origin);
		
		if (mid_dot * left_dot <= 0) {
			// Note: <= 0 vs < 0 is actually important for this branch.
			// 0 would indicate either left or mid point is our exact answer.
			right_idx = mid_idx;
		} else {
			left_idx = mid_idx;
			left_dot = mid_dot;
		}
	}

	hint.update(left_idx);
	return intersectWithSegment(line, left_idx);
}

bool
PolylineIntersector::intersectsSegment(QLineF const& normal, int segment) const
{
	if (segment < 0 || segment >= m_numSegments) {
		return false;
	}

	QLineF const seg_line(m_polyline[segment], m_polyline[segment + 1]);
	return intersectsSpan(normal, seg_line);
}

bool
PolylineIntersector::intersectsSpan(QLineF const& normal, QLineF const& span) const
{
	Vec2d const v1(normal.p2() - normal.p1());
	Vec2d const v2(span.p1() - normal.p1());
	Vec2d const v3(span.p2() - normal.p1());
	return v1.dot(v2) * v1.dot(v3) <= 0;
}

QPointF
PolylineIntersector::intersectWithSegment(QLineF const& line, int segment) const
{
	QLineF const seg_line(m_polyline[segment], m_polyline[segment + 1]);
	QPointF intersection;
	if (line.intersect(seg_line, &intersection) == QLineF::NoIntersection) {
		// Considering we were called for a reason, the segment must
		// be on the same line as our subject line.  Just return segment
		// midpoint in this case.
		return seg_line.pointAt(0.5);
	}

	return intersection;
}

bool
PolylineIntersector::tryIntersectingOutsideOfPolyline(
	QLineF const& line, QPointF& intersection, Hint& hint) const
{
	QLineF const normal(line.normalVector());
	QPointF const origin(normal.p1());
	Vec2d const nv(normal.p2() - normal.p1());

	Vec2d const front_vec(m_polyline.front() - origin);
	Vec2d const back_vec(m_polyline.back() - origin);
	double const front_dot = nv.dot(front_vec);
	double const back_dot = nv.dot(back_vec);
	
	if (front_dot * back_dot <= 0) {
		return false;
	}

	ToLineProjector const proj(line);

	if (fabs(front_dot) < fabs(back_dot)) {
		hint.update(-1);
		intersection = proj.projectionPoint(m_polyline.front());
	} else {
		hint.update(m_numSegments);
		intersection = proj.projectionPoint(m_polyline.back());
	}

	return true;
}
