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

#include "ConstraintSet.h"
#include "FittableSpline.h"
#include <boost/foreach.hpp>
#include <assert.h>

namespace spfit
{

ConstraintSet::ConstraintSet(FittableSpline const* spline)
: m_pSpline(spline)
{
	assert(spline);
}

void
ConstraintSet::constrainControlPoint(int cp_idx, QPointF const& pos)
{
	assert(cp_idx >= 0 && cp_idx < m_pSpline->numControlPoints());
	QPointF const cp(m_pSpline->controlPointPosition(cp_idx));

	// Fix x coordinate.
	LinearFunction f(m_pSpline->numControlPoints() * 2);
	f.a[cp_idx * 2] = 1;
	f.b = cp.x() - pos.x();
	m_constraints.push_back(f);

	// Fix y coordinate.
	f.a[cp_idx * 2] = 0;
	f.a[cp_idx * 2 + 1] = 1;
	f.b = cp.y() - pos.y();
	m_constraints.push_back(f);
}

void
ConstraintSet::constrainSplinePoint(double t, QPointF const& pos)
{
	std::vector<FittableSpline::LinearCoefficient> coeffs;
	m_pSpline->linearCombinationAt(t, coeffs);

	// Fix the x coordinate.
	LinearFunction f(m_pSpline->numControlPoints() * 2);
	f.b = -pos.x();
	BOOST_FOREACH(FittableSpline::LinearCoefficient const& coeff, coeffs) {
		int const cp_idx = coeff.controlPointIdx;
		f.a[cp_idx * 2] = coeff.coeff;

		// Because we want a function from control point displacements, not positions.
		f.b += m_pSpline->controlPointPosition(cp_idx).x() * coeff.coeff;
	}
	m_constraints.push_back(f);

	// Fix the y coordinate.
	f.a.fill(0);
	f.b = -pos.y();
	BOOST_FOREACH(FittableSpline::LinearCoefficient const& coeff, coeffs) {
		int const cp_idx = coeff.controlPointIdx;
		f.a[cp_idx * 2 + 1] = coeff.coeff;

		// Because we want a function from control point displacements, not positions.
		f.b += m_pSpline->controlPointPosition(cp_idx).y() * coeff.coeff;
	}
	m_constraints.push_back(f);
}

void
ConstraintSet::constrainControlPoint(int cp_idx, QLineF const& line)
{
	assert(cp_idx >= 0 && cp_idx < m_pSpline->numControlPoints());

	if (line.p1() == line.p2()) {
		constrainControlPoint(cp_idx, line.p1());
		return;
	}

	double const dx = line.p2().x() - line.p1().x();
	double const dy = line.p2().y() - line.p1().y();

	// Lx(cp) = p1.x + t * dx
	// Ly(cp) = p1.y + t * dy
	// Lx(cp) * dy = p1.x * dy + t * dx * dy
	// Ly(cp) * dx = p1.y * dx + t * dx * dy
	// Lx(cp) * dy - Ly(cp) * dx = p1.x * dy - p1.y * dx
	// L(cp) = Lx(cp) * dy - Ly(cp) * dx
	// L(cp) + (p1.y * dx - p1.x * dy) = 0

	LinearFunction f(m_pSpline->numControlPoints() * 2);
	f.a[cp_idx * 2] = dy;
	f.a[cp_idx * 2 + 1] = -dx;
	f.b = line.p1().y() * dx - line.p1().x() * dy;

	// Make it a function of control point displacements, not control points themselves.
	QPointF const cp(m_pSpline->controlPointPosition(cp_idx));
	f.b += cp.x() * dy;
	f.b += cp.y() * dx;

	m_constraints.push_back(f);
}

void
ConstraintSet::constrainSplinePoint(double t, QLineF const& line)
{
	if (line.p1() == line.p2()) {
		constrainSplinePoint(t, line.p1());
		return;
	}

	std::vector<FittableSpline::LinearCoefficient> coeffs;
	m_pSpline->linearCombinationAt(t, coeffs);

	double const dx = line.p2().x() - line.p1().x();
	double const dy = line.p2().y() - line.p1().y();

	// Lx(cp) = p1.x + t * dx
	// Ly(cp) = p1.y + t * dy
	// Lx(cp) * dy = p1.x * dy + t * dx * dy
	// Ly(cp) * dx = p1.y * dx + t * dx * dy
	// Lx(cp) * dy - Ly(cp) * dx = p1.x * dy - p1.y * dx
	// L(cp) = Lx(cp) * dy - Ly(cp) * dx
	// L(cp) + (p1.y * dx - p1.x * dy) = 0

	LinearFunction f(m_pSpline->numControlPoints() * 2);
	f.b = line.p1().y() * dx - line.p1().x() * dy;
	BOOST_FOREACH(FittableSpline::LinearCoefficient const& coeff, coeffs) {
		f.a[coeff.controlPointIdx * 2] = coeff.coeff * dy;
		f.a[coeff.controlPointIdx * 2 + 1] = -coeff.coeff * dx;

		// Because we want a function from control point displacements, not positions.
		QPointF const cp(m_pSpline->controlPointPosition(coeff.controlPointIdx));
		f.b += cp.x() * coeff.coeff * dy - cp.y() * coeff.coeff * dx;
	}
	m_constraints.push_back(f);
}

} // namespace spfit
