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

#ifndef SPFIT_FRENET_FRAME_H_
#define SPFIT_FRENET_FRAME_H_

#include "VecNT.h"

namespace spfit
{

/**
 * Origin + unit tangent + unit normal vectors.
 */
class FrenetFrame
{
	// Member-wise copying is OK.
public:
	enum YAxisDirection { Y_POINTS_UP, Y_POINTS_DOWN };

	/**
	 * \brief Builds a Frenet frame from an origin and a (non-unit) tangent vector.
	 *
	 * The direction of the normal vector is chosen according to \p ydir,
	 * considering the tangent vector to be pointing to the right.  The normal direction
	 * does matter, as we want the unit normal vector divided by signed curvature give
	 * us the center of the curvature.  For that to be the case, normal vector's direction
	 * relative to the unit vector's direction must be the same as the Y axis direction
	 * relative to the X axis direction in the coordinate system from which we derive
	 * the curvature.  For 2D computer graphics, the right direction is Y_POINTS_DOWN.
	 */
	FrenetFrame(Vec2d const& origin, Vec2d const& tangent_vector, YAxisDirection ydir = Y_POINTS_DOWN);

	Vec2d const& origin() const { return m_origin; }

	Vec2d const& unitTangent() const { return m_unitTangent; }

	Vec2d const& unitNormal() const { return m_unitNormal; }
private:
	Vec2d m_origin;
	Vec2d m_unitTangent;
	Vec2d m_unitNormal;
};

} // namespace spfit

#endif
