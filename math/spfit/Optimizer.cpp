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
#include <string.h>

namespace spfit
{

Optimizer::Optimizer(int num_control_points)
:	m_origTotalSqDist(0),
	m_optimizedTotalSqDist(0),
	m_A(boost::extents[num_control_points*2][num_control_points*2], boost::fortran_storage_order()),
	m_b(num_control_points*2, 0.0),
	m_numControlPoints(num_control_points)
{
}

void
Optimizer::reset()
{
	memset(m_A.data(), 0, sizeof(m_A[0][0])*4*m_numControlPoints*m_numControlPoints);
	memset(&m_b[0], 0, sizeof(m_b[0])*2*m_numControlPoints);
	m_origTotalSqDist = 0;
	m_optimizedTotalSqDist = 0;
}

void
Optimizer::addSample(Vec2d const& spline_point,
	std::vector<FittableSpline::LinearCoefficient> const& coeffs,
	SqDistApproximant const& sqdist_approx)
{
	m_origTotalSqDist += sqdist_approx.evaluate(spline_point);

	// Update matrix m_A which stands for A in "C^T*A*C + b^T*C + c" in [1].
	for (int i = 0; i < 2; ++i) {
		for (int j = 0; j < 2; ++j) {
			double const a = sqdist_approx.A(i, j);

			// Now we'll be multiplying two polynomials: P[i] * P[j]
			// where P[0] = sum(k[m]*c[m].x]) and P[1] = sum(k[n]*c[n].y])
			// where c[i] represents a control point displacement vector
			// and k[i] represents the corresponding linear coefficient.
			// m and n belong to [0, numControlPoints)
			//
			// Note #1: we consider the vector c to be flat, with the following layout:
			// c = [x0 y0 x1 y1 x2 y2 ...]
			// Note #2: We don't have to iterate m and n all the way
			// from 0 to numControlPoints, as we know only a few linear
			// coefficients will be non-zero, and we already know which ones.
			BOOST_FOREACH(FittableSpline::LinearCoefficient const& c1, coeffs) {
				int const c1_idx = c1.controlPointIdx * 2 + i;
				BOOST_FOREACH(FittableSpline::LinearCoefficient const& c2, coeffs) {
					int const c2_idx = c2.controlPointIdx * 2 + j;	
					m_A[c1_idx][c2_idx] += a * c1.coeff * c2.coeff;
				}
			}
		}
	}

	StaticMatrixCalc<double, 10, 1> mc;

	Vec2d w; // Represents "2*Ak*sk + bk" in [1].
	(mc(sqdist_approx.A)*mc(spline_point, 2, 1)*2.0 + mc(sqdist_approx.b, 2, 1)).write(w);
	
	// Update vector m_b which stands for b in "C^T*A*C + b^T*C + c" in [1].
	for (int i = 0; i < 2; ++i) {
		BOOST_FOREACH(FittableSpline::LinearCoefficient const& c, coeffs) {
			int const c_idx = c.controlPointIdx * 2 + i;
			m_b[c_idx] += w[i] * c.coeff;
		}
	}
}

void
Optimizer::optimize(VirtualFunction2<void, int, Vec2d>& displacement_vectors_sink)
{
	matrixMakeSymmetric();

	int const nc2 = m_numControlPoints*2;

	DynamicMatrixCalc<double> mc;
	double const* c = mc(m_A.data(), nc2, nc2).solve(-0.5*mc(&m_b[0], nc2, 1)).rawData();

	for (int i = 0; i < m_numControlPoints; ++i) {
		displacement_vectors_sink(i, Vec2d(c[i*2], c[i*2 + 1]));
	}

	// Now calculate the new total squared distance.
	m_optimizedTotalSqDist = m_origTotalSqDist;
	for (int i = 0; i < nc2; ++i) {
		for (int j = 0; j < nc2; ++j) {
			m_optimizedTotalSqDist += m_A[i][j] * c[i] * c[j];
		}
	}
	for (int i = 0; i < nc2; ++i) {
		m_optimizedTotalSqDist += m_b[i] * c[i];
	}
}

void
Optimizer::matrixMakeSymmetric()
{
	int const nc2 = m_numControlPoints*2;
	for (int i = 0; i < nc2; ++i) {
		for (int j = i + 1; j < nc2; ++j) {
			double avg = 0.5 * (m_A[i][j] + m_A[j][i]);
			m_A[i][j] = avg;
			m_A[j][i] = avg;
		}
	}
}

} // namespace spfit
