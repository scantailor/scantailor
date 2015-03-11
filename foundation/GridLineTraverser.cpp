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

#include "GridLineTraverser.h"
#include "LineIntersectionScalar.h"
#include <algorithm>
#include <cstdlib>

GridLineTraverser::GridLineTraverser(QLineF const& line)
{
	QPoint const p1(line.p1().toPoint());
	QPoint const p2(line.p2().toPoint());
	int h_spans, v_spans, num_spans;
	double s1 = 0.0, s2 = 0.0;
	if ((h_spans = abs(p1.x() - p2.x())) > (v_spans = abs(p1.y() - p2.y()))) {
		// Major direction: horizontal.
		num_spans = h_spans;
		lineIntersectionScalar(line, QLineF(p1, QPoint(p1.x(), p1.y() + 1)), s1);
		lineIntersectionScalar(line, QLineF(p2, QPoint(p2.x(), p2.y() + 1)), s2);
	} else {
		// Major direction: vertical.
		num_spans = v_spans;
		lineIntersectionScalar(line, QLineF(p1, QPoint(p1.x() + 1, p1.y())), s1);
		lineIntersectionScalar(line, QLineF(p2, QPoint(p2.x() + 1, p2.y())), s2);
	}
	
	m_dt = num_spans == 0 ? 0 : 1.0 / num_spans;
	m_line.setP1(line.pointAt(s1));
	m_line.setP2(line.pointAt(s2));
	m_totalStops = num_spans + 1;
	m_stopsDone = 0;
}

QPoint
GridLineTraverser::next()
{
	QPointF const pt(m_line.pointAt(m_stopsDone * m_dt));
	++m_stopsDone;
	return pt.toPoint();
}
