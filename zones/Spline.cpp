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

#include "Spline.h"
#include "SplineSegment.h"
#include <assert.h>

Spline::Spline()
{
}

void
Spline::appendVertex(QPointF const& pt)
{
	m_sentinel.insertBefore(pt);
}

bool
Spline::hasAtLeastSegments(int num) const
{
	for (SegmentIterator it((Spline&)*this); num > 0 && it.hasNext(); it.next()) {
		--num;
	}

	return num == 0;
}

QPolygonF
Spline::toPolygon() const
{
	QPolygonF poly;

	SplineVertex::Ptr vertex(firstVertex());
	for (; vertex; vertex = vertex->next(SplineVertex::NO_LOOP)) {
		poly.push_back(vertex->point());
	}

	vertex = lastVertex()->next(SplineVertex::LOOP_IF_BRIDGED);
	if (vertex) {
		poly.push_back(vertex->point());
	}

	return poly;
}


/*======================== Spline::SegmentIterator =======================*/

bool
Spline::SegmentIterator::hasNext() const
{
	return m_ptrNextVertex && m_ptrNextVertex->next(SplineVertex::LOOP_IF_BRIDGED);
}

SplineSegment
Spline::SegmentIterator::next()
{
	assert(hasNext());

	SplineVertex::Ptr origin(m_ptrNextVertex);
	m_ptrNextVertex = m_ptrNextVertex->next(SplineVertex::NO_LOOP);
	if (!m_ptrNextVertex) {
		return SplineSegment(origin, origin->next(SplineVertex::LOOP_IF_BRIDGED));
	} else {
		return SplineSegment(origin, m_ptrNextVertex);
	}
}
