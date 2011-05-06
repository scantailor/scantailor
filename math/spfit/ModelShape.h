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

#ifndef SPFIT_MODEL_SHAPE_H_
#define SPFIT_MODEL_SHAPE_H_

#include "SqDistApproximant.h"
#include "FittableSpline.h"
#include <QPointF>

namespace spfit
{

/**
 * \brief A shape we are trying to fit a spline to.
 *
 * Could be a polyline or maybe a point cloud.
 */
class ModelShape
{
public:
	virtual ~ModelShape() {}

	/**
	 * Returns a function that approximates the squared distance to the model.
	 * The function is only accurate in the neighbourhood of \p pt.
	 */
	virtual SqDistApproximant localSqDistApproximant(
		QPointF const& pt, FittableSpline::SampleFlags flags) const = 0;
};

} // namespace spfit

#endif
