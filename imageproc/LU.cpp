/*
	Scan Tailor - Interactive post-processing tool for scanned pages.
	Copyright (C)  Joseph Artsimovich <joseph.artsimovich@gmail.com>

	Based on the public domain source code from:
	JAMA : A Java Matrix Package

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

#include "LU.h"
#include <algorithm>
#include <stdexcept>
#include <math.h>

LU::LU(int const m, int const n, double const* A)
:	m_LU(A, A + m * n),
	m_piv(m),
	m_pivSign(1),
	m_rows(m),
	m_cols(n)
{
	// Use a "left-looking", dot-product, Crout/Doolittle algorithm.

	for (int i = 0; i < m; ++i) {
		m_piv[i] = i;
	}

	std::vector<double> LUcolj(m);

	// Outer loop.

	for (int j = 0; j < n; ++j) {

		// Make a copy of the j-th column to localize references.

		for (int i = 0; i < m; ++i) {
			LUcolj[i] = m_LU[i * n + j];
		}

		// Apply previous transformations.

		for (int i = 0; i < m; ++i) {
			double* LUrowi = &m_LU[i * n];

			// Most of the time is spent in the following dot product.

			int kmax = std::min(i, j);
			double s = 0.0;
			for (int k = 0; k < kmax; ++k) {
				s += LUrowi[k]*LUcolj[k];
			}

			LUrowi[j] = LUcolj[i] -= s;
		}

		// Find pivot and exchange if necessary.

		int p = j;
		for (int i = j + 1; i < m; ++i) {
			if (fabs(LUcolj[i]) > fabs(LUcolj[p])) {
				p = i;
			}
		}
		if (p != j) {
			for (int k = 0; k < n; k++) {
				std::swap(m_LU[p * n + k], m_LU[j * n + k]);
			}
			std::swap(m_piv[p], m_piv[j]);
			m_pivSign = -m_pivSign;
		}

		// Compute multipliers.

		if (j < m && m_LU[j * n + j] != 0.0) {
			for (int i = j + 1; i < m; ++i) {
				m_LU[i * n + j] /= m_LU[j * n + j];
			}
		}
	}
}

bool
LU::isNonsingular() const
{
	for (int j = 0; j < m_cols; ++j) {
		if (m_LU[j * m_cols + j] == 0.0) {
			return false;
		}
	}
	return true;
}

void
LU::getL(double* L) const
{
	for (int i = 0; i < m_rows; ++i) {
		for (int j = 0; j < m_cols; ++j) {
			if (i > j) {
				L[i * m_cols + j] = m_LU[i * m_cols + j];
			} else if (i == j) {
				L[i * m_cols + j] = 1.0;
			} else {
				L[i * m_cols + j] = 0.0;
			}
		}
	}
}

void
LU::getU(double* U) const
{
	for (int i = 0; i < m_rows; ++i) {
		for (int j = 0; j < m_cols; ++j) {
			if (i <= j) {
				U[i * m_cols + j] = m_LU[i * m_cols + j];
			} else {
				U[i * m_cols + j] = 0.0;
			}
		}
	}
}

void
LU::getPivot(int* piv) const
{
	for (int i = 0; i < m_rows; ++i) {
		piv[i] = m_piv[i];
	}
}

void
LU::solve(int const nx, double* X, double const* B) const
{
	if (!isNonsingular()) {
		throw std::runtime_error("Matrix is singular.");
	}

	// Pivot B into X.
	for (int i = 0; i < m_rows; ++i) {
		for (int j = 0; j < nx; ++j) {
			X[i * nx + j] = B[m_piv[i] * nx + j];
		}
	}

	// Solve L*Y = B(piv,:)
	for (int k = 0; k < m_cols; ++k) {
		for (int i = k + 1; i < m_cols; ++i) {
			for (int j = 0; j < nx; ++j) {
				X[i * nx + j] -= X[k * nx + j] * m_LU[i * m_cols + k];
			}
		}
	}
	// Solve U*X = Y;
	for (int k = m_cols - 1; k >= 0; --k) {
		for (int j = 0; j < nx; ++j) {
			X[k * nx + j] /= m_LU[k * m_cols + k];
		}
		for (int i = 0; i < k; ++i) {
			for (int j = 0; j < nx; j++) {
				X[i * nx + j] -= X[k * nx + j] * m_LU[i * m_cols + k];
			}
		}
	}
}

