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

#ifndef SPLINE_SEGMENT_H_
#define SPLINE_SEGMENT_H_

#include "SplineVertex.h"
#include <QPointF>
#include <QLineF>

class SplineSegment
{
public:
	SplineVertex::Ptr prev;
	SplineVertex::Ptr next;

	SplineSegment() {}

	SplineSegment(SplineVertex::Ptr const& prev, SplineVertex::Ptr const& next);

	SplineVertex::Ptr splitAt(QPointF const& pt);

	bool isValid() const;

	bool operator==(SplineSegment const& other) const {
		return prev == other.prev && next == other.next;
	}

	QLineF toLine() const { return QLineF(prev->point(), next->point()); }
};

#endif
