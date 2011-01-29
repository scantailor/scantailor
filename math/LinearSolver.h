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

#ifndef LINEAR_SOLVER_H_
#define LINEAR_SOLVER_H_

#include "NonCopyable.h"
#include "StaticPool.h"
#include <cmath>
#include <limits>
#include <stdexcept>
#include <string>
#include <boost/scoped_array.hpp>
#include <stddef.h>
#include <assert.h>

/**
 * \brief Solves Ax = b using LU decomposition.
 *
 * Overdetermined systems are supported.  Solving them will succeed
 * provided the system is consistent.
 *
 * \note All matrices are assumed to be in column-major order.
 *
 * \see MatrixCalc
 */
class LinearSolver
{
	// Member-wise copying is OK.
public:
	/*
	 * \throw std::runtime_error If rows_AB < cols_A_rows_X.
	 */
	LinearSolver(size_t rows_AB, size_t cols_A_rows_X, size_t cols_BX);

	/**
	 * \brief Solves Ax = b
	 *
	 * \param A Matrix A.
	 * \param X Matrix (or vector) X.  Results will be written here.
	 * \param B Matrix (or vector) B.  It's allowed to pass the same pointer for X and B.
	 * \param tbuffer Temporary buffer of at least "cols(A) * (rows(B) + cols(B))" T elements.
	 * \param pbuffer Temporary buffer of at least "rows(B)" size_t elements.
	 *
	 * \throw std::runtime_error If the system can't be solved.
	 */
	template<typename T>
	void solve(T const* A, T* X, T const* B, T* tbuffer, size_t* pbuffer) const;

	/**
	 * \brief A simplified version of the one above.
	 *
	 * In this version, buffers are allocated internally.
	 */
	template<typename T>
	void solve(T const* A, T* X, T const* B) const;
private:
	size_t m_rowsAB;
	size_t m_colsArowsX;
	size_t m_colsBX;
};


template<typename T>
void
LinearSolver::solve(T const* A, T* X, T const* B, T* tbuffer, size_t* pbuffer) const
{
	using namespace std; // To catch different overloads of abs()

	T const epsilon(sqrt(numeric_limits<T>::epsilon()));

	size_t const num_elements_A = m_rowsAB * m_colsArowsX;

	T* const lu_data = tbuffer; // Dimensions: m_rowsAB, m_colsArowsX
	tbuffer += num_elements_A;

	// Copy this matrix to lu.
	for (size_t i = 0; i < num_elements_A; ++i) {
		lu_data[i] = A[i];
	}
	
	// Maps virtual row numbers to physical ones.
	size_t* const perm = pbuffer;
	for (size_t i = 0; i < m_rowsAB; ++i) {
		perm[i] = i;
	}

	T* p_col = lu_data;
	for (size_t i = 0; i < m_colsArowsX; ++i, p_col += m_rowsAB) {
		// Find the largest pivot.
		size_t virt_pivot_row = i;
		T largest_abs_pivot(abs(p_col[perm[i]]));
		for (size_t j = i + 1; j < m_rowsAB; ++j) {
			T const abs_pivot(abs(p_col[perm[j]]));
			if (abs_pivot > largest_abs_pivot) {
				largest_abs_pivot = abs_pivot;
				virt_pivot_row = j;
			}
		}
		
		if (largest_abs_pivot <= epsilon) {
			throw std::runtime_error("LinearSolver: not a full rank matrix");
		}

		size_t const phys_pivot_row(perm[virt_pivot_row]);
		perm[virt_pivot_row] = perm[i];
		perm[i] = phys_pivot_row;
		
		T const* const p_pivot = p_col + phys_pivot_row;
		T const r_pivot(T(1) / *p_pivot);
		
		// Eliminate entries below the pivot.
		for (size_t j = i + 1; j < m_rowsAB; ++j) {
			T const* p1 = p_pivot;
			T* p2 = p_col + perm[j];
			if (abs(*p2) <= epsilon) {
				// We consider it's already zero.
				*p2 = T();
				continue;
			}

			T const factor(*p2 * r_pivot);
			*p2 = factor; // Factor goes into L, zero goes into U.

			// Transform the rest of the row.
			for (size_t col = i + 1; col < m_colsArowsX; ++col) {
				p1 += m_rowsAB;
				p2 += m_rowsAB;
				*p2 -= *p1 * factor;
			}
		}
	}
	
	// First solve Ly = b
	T* const y_data = tbuffer; // Dimensions: m_colsArowsX, m_colsBX
	//tbuffer += m_colsArowsX * m_colsBX;
	T* p_y_col = y_data;
	T const* p_b_col = B;
	for (size_t y_col = 0; y_col < m_colsBX; ++y_col) {
		size_t virt_row = 0;
		for (; virt_row < m_colsArowsX; ++virt_row) {
			int const phys_row = perm[virt_row];
			T right(p_b_col[phys_row]);
			
			// Move already calculated factors to the right side.
			T const* p_lu = lu_data + phys_row;
			// Go left to right, stop at diagonal.
			for (size_t lu_col = 0; lu_col < virt_row; ++lu_col) {
				right -= *p_lu * p_y_col[lu_col];
				p_lu += m_rowsAB;
			}

			// We assume L has ones on the diagonal, so no division here.
			p_y_col[virt_row] = right;
		}

		// Continue below the square part (if any).
		for (; virt_row < m_rowsAB; ++virt_row) {
			int const phys_row = perm[virt_row];
			T right(p_b_col[phys_row]);

			// Move everything to the right side, then verify it's zero.
			T const* p_lu = lu_data + phys_row;
			// Go left to right all the way.
			for (size_t lu_col = 0; lu_col < m_colsArowsX; ++lu_col) {
				right -= *p_lu * p_y_col[lu_col];
				p_lu += m_rowsAB;
			}
			if (abs(right) > epsilon) {
				throw std::runtime_error("LinearSolver: inconsistent overdetermined system");
			}
		}

		p_y_col += m_colsArowsX;
		p_b_col += m_rowsAB;
	}

	// Now solve Ux = y
	T* p_x_col = X;
	p_y_col = y_data;
	T const* p_lu_last_col = lu_data + (m_colsArowsX - 1) * m_rowsAB;
	for (size_t x_col = 0; x_col < m_colsBX; ++x_col) {
		for (int virt_row = m_colsArowsX - 1; virt_row >= 0; --virt_row) {
			T right(p_y_col[virt_row]);
			
			// Move already calculated factors to the right side.
			T const* p_lu = p_lu_last_col + perm[virt_row];
			// Go right to left, stop at diagonal.
			for (int lu_col = m_colsArowsX - 1; lu_col > virt_row; --lu_col) {
				right -= *p_lu * p_x_col[lu_col];
				p_lu -= m_rowsAB;
			}
			p_x_col[virt_row] = right / *p_lu; 
		}

		p_x_col += m_colsArowsX;
		p_y_col += m_colsArowsX;
	}
}

template<typename T>
void
LinearSolver::solve(T const* A, T* X, T const* B) const
{
	boost::scoped_array<T> tbuffer(new T[m_colsArowsX * (m_rowsAB + m_colsBX)]);
	boost::scoped_array<size_t> pbuffer(new size_t[m_rowsAB]);

	solve(A, X, B, tbuffer.get(), pbuffer.get());
}

#endif
