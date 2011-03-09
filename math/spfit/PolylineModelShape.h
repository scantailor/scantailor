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

#ifndef SPFIT_POLYLINE_MODEL_SHAPE_H_
#define SPFIT_POLYLINE_MODEL_SHAPE_H_

#include "NonCopyable.h"
#include "ModelShape.h"
#include "SqDistApproximant.h"
#include "XSpline.h"
#include "VecNT.h"
#include <QPointF>
#include <QRectF>
#include <vector>

namespace spfit
{

class PolylineModelShape : public ModelShape
{
	DECLARE_NON_COPYABLE(PolylineModelShape)
public:	
	PolylineModelShape(std::vector<QPointF> const& polyline);

	virtual QRectF boundingBox() const;

	virtual SqDistApproximant localSqDistApproximant(QPointF const& pt, int flags = 0) const;
private:
	static SqDistApproximant calcApproximant(
		Vec2d const& region_origin, Vec2d const& frenet_frame_origin,
		Vec2d const& unit_tangent, Vec2f const& unit_normal, double curvature);

	std::vector<XSpline::PointAndDerivs> m_vertices;
	QRectF m_boundingBox;
};

} // namespace spfit

#endif
