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

#ifndef PROXIMITY_H_
#define PROXIMITY_H_

#include <limits>
#include <math.h>

class QPointF;
class QLineF;

class Proximity
{
public:
	Proximity() : m_sqDist(std::numeric_limits<double>::max()) {}

	Proximity(QPointF const& p1, QPointF const& p2);

	static Proximity fromDist(double dist) { return Proximity(dist * dist); }

	static Proximity fromSqDist(double sqDist) { return Proximity(sqDist); }

	static Proximity pointAndLineSegment(
		QPointF const& pt, QLineF const& segment, QPointF* point_on_segment = 0);

	double dist() const { return sqrt(m_sqDist); }

	double sqDist() const { return m_sqDist; }

	bool operator==(Proximity const& rhs) const {
		return m_sqDist == rhs.m_sqDist;
	}

	bool operator!=(Proximity const& rhs) const {
		return m_sqDist != rhs.m_sqDist;
	}

	bool operator<(Proximity const& rhs) const {
		return m_sqDist < rhs.m_sqDist;
	}

	bool operator>(Proximity const& rhs) const {
		return m_sqDist > rhs.m_sqDist;
	}

	bool operator<=(Proximity const& rhs) const {
		return m_sqDist <= rhs.m_sqDist;
	}

	bool operator>=(Proximity const& rhs) const {
		return m_sqDist >= rhs.m_sqDist;
	}
private:
	Proximity(double sqDist) : m_sqDist(sqDist) {}

	double m_sqDist;
};

#endif
