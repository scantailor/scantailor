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

#ifndef SPFIT_CONSTRAINT_SET_H_
#define SPFIT_CONSTRAINT_SET_H_

#include "LinearFunction.h"
#include <QPointF>
#include <QLineF>
#include <list>
#include <stddef.h>

namespace spfit
{

class FittableSpline;

class ConstraintSet
{
	// Member-wise copying is OK.
public:
	ConstraintSet(FittableSpline const* spline);

	std::list<LinearFunction> const& constraints() const { return m_constraints; }

	void constrainControlPoint(int cp_idx, QPointF const& pos);

	void constrainControlPoint(int cp_idx, QLineF const& line);

	void constrainSplinePoint(double t, QPointF const& pos);

	void constrainSplinePoint(double t, QLineF const& line);
private:
	FittableSpline const* m_pSpline;
	std::list<LinearFunction> m_constraints;	
};

} // namespace spfit

#endif
