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

#include "SplineVertex.h"
#include <assert.h>


/*============================= SplineVertex ============================*/

SplineVertex::SplineVertex(SplineVertex* prev, SplineVertex* next)
:	m_pPrev(prev),
	m_ptrNext(next)
{
}

void
SplineVertex::remove()
{
	// Be very careful here - don't let this object
	// be destroyed before we've finished working with it.

	m_pPrev->m_ptrNext.swap(m_ptrNext);
	assert(m_ptrNext.get() == this);

	m_pPrev->m_ptrNext->m_pPrev = m_pPrev;
	m_pPrev = 0;

	// This may or may not destroy this object,
	// depending on if there are other references to it.
	m_ptrNext.reset();
}

bool
SplineVertex::hasAtLeastSiblings(int const num)
{
	int todo = num;
	for (SplineVertex::Ptr node(this); (node = node->next(LOOP)).get() != this; ) {
		if (--todo == 0) {
			return true;
		}
	}
	return false;
}

SplineVertex::Ptr
SplineVertex::prev(Loop const loop)
{
	return m_pPrev->thisOrPrevReal(loop);
}

SplineVertex::Ptr
SplineVertex::next(Loop const loop)
{
	return m_ptrNext->thisOrNextReal(loop);
}

SplineVertex::Ptr
SplineVertex::insertBefore(QPointF const& pt)
{
	SplineVertex::Ptr new_vertex(new RealSplineVertex(pt, m_pPrev, this));
	m_pPrev->m_ptrNext = new_vertex;
	m_pPrev = new_vertex.get();
	return new_vertex;
}

SplineVertex::Ptr
SplineVertex::insertAfter(QPointF const& pt)
{
	SplineVertex::Ptr new_vertex(new RealSplineVertex(pt, this, m_ptrNext.get()));
	m_ptrNext->m_pPrev = new_vertex.get();
	m_ptrNext = new_vertex;
	return new_vertex;
}


/*========================= SentinelSplineVertex =======================*/

SentinelSplineVertex::SentinelSplineVertex()
:	SplineVertex(this, this),
	m_bridged(false)
{
}

SentinelSplineVertex::~SentinelSplineVertex()
{
	// Just releasing m_ptrNext is not enough, because in case some external
	// object holds a reference to a vertix of this spline, that vertex will
	// still (possibly indirectly) reference us through a chain of m_ptrNext
	// smart pointers.  Therefore, we explicitly unlink each node.
	while (m_ptrNext.get() != this) {
		m_ptrNext->remove();
	}
}

SplineVertex::Ptr
SentinelSplineVertex::thisOrPrevReal(Loop const loop)
{
	if (loop == LOOP || (loop == LOOP_IF_BRIDGED && m_bridged)) {
		return SplineVertex::Ptr(m_pPrev);
	} else {
		return SplineVertex::Ptr();
	}
}

SplineVertex::Ptr
SentinelSplineVertex::thisOrNextReal(Loop const loop)
{
	if (loop == LOOP || (loop == LOOP_IF_BRIDGED && m_bridged)) {
		return m_ptrNext;
	} else {
		return SplineVertex::Ptr();
	}
}

QPointF const
SentinelSplineVertex::point() const
{
	assert(!"Illegal call to SentinelSplineVertex::point()");
	return QPointF();
}

void
SentinelSplineVertex::setPoint(QPointF const& pt)
{
	assert(!"Illegal call to SentinelSplineVertex::setPoint()");
}

void
SentinelSplineVertex::remove()
{
	assert(!"Illegal call to SentinelSplineVertex::remove()");
}

SplineVertex::Ptr
SentinelSplineVertex::firstVertex() const
{
	if (m_ptrNext.get() == this) {
		return SplineVertex::Ptr();
	} else {
		return m_ptrNext;
	}
}

SplineVertex::Ptr
SentinelSplineVertex::lastVertex() const
{
	if (m_pPrev == this) {
		return SplineVertex::Ptr();
	} else {
		return SplineVertex::Ptr(m_pPrev);
	}
}


/*============================== RealSplineVertex ============================*/

RealSplineVertex::RealSplineVertex(
	QPointF const& pt, SplineVertex* prev, SplineVertex* next)
:	SplineVertex(prev, next),
	m_point(pt),
	m_refCounter(0)
{
}

void
RealSplineVertex::ref() const
{
	++m_refCounter;
}

void
RealSplineVertex::unref() const
{
	if (--m_refCounter == 0) {
		delete this;
	}
}

SplineVertex::Ptr
RealSplineVertex::thisOrPrevReal(Loop)
{
	return SplineVertex::Ptr(this);
}

SplineVertex::Ptr
RealSplineVertex::thisOrNextReal(Loop loop)
{
	return SplineVertex::Ptr(this);
}

QPointF const
RealSplineVertex::point() const
{
	return m_point;
}

void
RealSplineVertex::setPoint(QPointF const& pt)
{
	m_point = pt;
}
