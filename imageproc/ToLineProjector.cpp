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

#include "ToLineProjector.h"

namespace imageproc
{

ToLineProjector::ToLineProjector(QLineF const& line)
:	m_origin(line.p1()),
	m_vec(line.p2() - line.p1()),
	m_mat(m_vec)
{
	// At*A*x = At*b
	// x = (At*A)-1 * At
	m_mat /= m_mat.dot(m_mat);
}

double
ToLineProjector::projectionScalar(QPointF const& pt) const
{
	Vec2d const b(pt - m_origin);
	return m_mat.dot(b);
}

QPointF
ToLineProjector::projectionPoint(QPointF const& pt) const
{
	return m_origin + m_vec * projectionScalar(pt);
}

} // namespace imageproc
