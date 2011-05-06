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

#include "FrenetFrame.h"
#include <math.h>

namespace spfit
{

FrenetFrame::FrenetFrame(Vec2d const& origin, Vec2d const& tangent_vector, YAxisDirection ydir)
: m_origin(origin)
{
	double const sqlen = tangent_vector.squaredNorm();
	if (sqlen > 1e-6) {
		m_unitTangent = tangent_vector / sqrt(sqlen);	
		if (ydir == Y_POINTS_UP) {
			m_unitNormal[0] = -m_unitTangent[1];
			m_unitNormal[1] = m_unitTangent[0];
		} else {
			m_unitNormal[0] = m_unitTangent[1];
			m_unitNormal[1] = -m_unitTangent[0];
		}
	}
}

} // namespace spfit
