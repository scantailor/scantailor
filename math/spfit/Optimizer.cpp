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

#include "Optimizer.h"
#include "MatrixCalc.h"
#include <boost/foreach.hpp>
#include <stdexcept>
#include <algorithm>
#include <assert.h>

namespace spfit
{

Optimizer::Optimizer(size_t num_vars)
: m_numVars(num_vars)
, m_A(num_vars, num_vars)
, m_b(num_vars)
, m_x(num_vars)
, m_externalForce(num_vars)
, m_internalForce(num_vars)
{
}

void
Optimizer::setConstraints(std::list<LinearFunction> const& constraints)
{
	size_t const num_constraints = constraints.size();
	size_t const num_dimensions = m_numVars + num_constraints;

	MatT<double> A(num_dimensions, num_dimensions);
	VecT<double> b(num_dimensions);
	// Matrix A and vector b will have the following layout:
	//     |N N N L L|      |-D|
	//     |N N N L L|      |-D|
	// A = |N N N L L|  b = |-D|
	//     |C C C 0 0|      |-J|
	//     |C C C 0 0|      |-J|
	// N: non-constant part of the gradient of the function we are minimizing.
	// C: non-constant part of constraint functions (one per line).
	// L: coefficients of Lagrange multipliers.  These happen to be equal
	//    to the symmetric C values.
	// D: constant part of the gradient of the function we are optimizing.
	// J: constant part of constraint functions.

	std::list<LinearFunction>::const_iterator ctr(constraints.begin());
	for (size_t i = m_numVars; i < num_dimensions; ++i, ++ctr) {
		b[i] = -ctr->b;
		for (size_t j = 0; j < m_numVars; ++j) {
			A(i, j) = A(j, i) = ctr->a[j];
		}
	}

	VecT<double>(num_dimensions).swap(m_x);
	m_A.swap(A);
	m_b.swap(b);
}

void
Optimizer::addExternalForce(QuadraticFunction const& force)
{
	m_externalForce += force;
}

void
Optimizer::addExternalForce(QuadraticFunction const& force, std::vector<int> const& sparse_map)
{
	size_t const num_vars = force.numVars();
	for (size_t i = 0; i < num_vars; ++i) {
		int const ii = sparse_map[i];
		for (size_t j = 0; j < num_vars; ++j) {
			int const jj = sparse_map[j];
			m_externalForce.A(ii, jj) += force.A(i, j);
		}
		m_externalForce.b[ii] += force.b[i];
	}
	m_externalForce.c += force.c;
}

void
Optimizer::addInternalForce(QuadraticFunction const& force)
{
	m_internalForce += force;
}

void
Optimizer::addInternalForce(
	QuadraticFunction const& force, std::vector<int> const& sparse_map)
{
	size_t const num_vars = force.numVars();
	for (size_t i = 0; i < num_vars; ++i) {
		int const ii = sparse_map[i];
		for (size_t j = 0; j < num_vars; ++j) {
			int const jj = sparse_map[j];
			m_internalForce.A(ii, jj) += force.A(i, j);
		}
		m_internalForce.b[ii] += force.b[i];
	}
	m_internalForce.c += force.c;
}

OptimizationResult
Optimizer::optimize(double internal_force_weight)
{
	// Note: because we are supposed to reset the forces anyway,
	// we re-use m_internalForce to store the cumulative force.
	m_internalForce *= internal_force_weight;
	m_internalForce += m_externalForce;
	
	// For the layout of m_A and m_b, see setConstraints()
	QuadraticFunction::Gradient const grad(m_internalForce.gradient());
	for (size_t i = 0; i < m_numVars; ++i) {
		m_b[i] = -grad.b[i];
		for (size_t j = 0; j < m_numVars; ++j) {
			m_A(i, j) = grad.A(i, j);
		}
	}

	double const total_force_before = m_internalForce.c;
	DynamicMatrixCalc<double> mc;

	try {
		mc(m_A).solve(mc(m_b)).write(m_x.data());
	} catch (std::runtime_error const&) {
		m_externalForce.reset();
		m_internalForce.reset();
		m_x.fill(0); // To make undoLastStep() work as expected.
		return OptimizationResult(total_force_before, total_force_before);
	}

	double const total_force_after = m_internalForce.evaluate(m_x.data());
	m_externalForce.reset(); // Now it's finally safe to reset these.
	m_internalForce.reset();

	// The last thing remaining is to adjust constraints,
	// as they depend on the current variables.
	adjustConstraints(1.0);

	return OptimizationResult(total_force_before, total_force_after);
}

void
Optimizer::undoLastStep()
{
	adjustConstraints(-1.0);
	m_x.fill(0);
}

/**
 * direction == 1 is used for forward adjustment,
 * direction == -1 is used for undoing the last step.
 */
void
Optimizer::adjustConstraints(double direction)
{
	size_t const num_dimensions = m_b.size();
	for (size_t i = m_numVars; i < num_dimensions; ++i) {
		// See setConstraints() for more information
		// on the layout of m_A and m_b.
		double c = 0;
		for (size_t j = 0; j < m_numVars; ++j) {
			c += m_A(i, j) * m_x[j];
		}
		m_b[i] -= c * direction;
	}
}

void
Optimizer::swap(Optimizer& other)
{
	m_A.swap(other.m_A);
	m_b.swap(other.m_b);
	m_x.swap(other.m_x);
	m_externalForce.swap(other.m_externalForce);
	m_internalForce.swap(other.m_internalForce);
	std::swap(m_numVars, other.m_numVars);
}

} // namespace spfit
