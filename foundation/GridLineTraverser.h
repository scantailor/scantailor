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

#ifndef GRID_LINE_TRAVERSER_H_
#define GRID_LINE_TRAVERSER_H_

#include <QLineF>

/**
 * \brief Traverses a grid along a line segment.
 *
 * Think about drawing a line on an image.
 */
class GridLineTraverser
{
	// Member-wise copying is OK.
public:
	GridLineTraverser(QLineF const& line);
	
	bool hasNext() const { return m_stopsDone < m_totalStops; }

	QPoint next();
private:
	QLineF m_line;
	double m_dt;
	int m_totalStops;
	int m_stopsDone;
};

#endif
