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

#ifndef LU_H_
#define LU_H_

#include <vector>

/**
 * \brief LU Decomposition.
 *
 * For an m-by-n matrix A with m >= n, the LU decomposition is an m-by-n
 * unit lower triangular matrix L, an n-by-n upper triangular matrix U,
 * and a permutation vector piv of length m so that A(piv,:) = L*U.
 * If m < n, then L is m-by-m and U is m-by-n.
 *
 * The LU decompostion with pivoting always exists, even if the matrix is
 * singular, so the constructor will never fail.  The primary use of the
 * LU decomposition is in the solution of square systems of simultaneous
 * linear equations.  This will fail if isNonsingular() returns false.
 */
class LU
{
public:
	LU(int m, int n, double const* A);

   bool isNonsingular() const;

   void getL(double* L) const;

   void getU(double* U) const;

   void getPivot(int* piv) const;

   /**
	* \brief Solve A*X = B
	*
	* \param nx The number of columns in X and B.
	* \param[out] X Resulting matrix, so that L*U*X = B(piv,:)
	* \param B A matrix in row-major order with as many rows as A and \p n number of columns.
	*
	* \exception std::runtime_error Matrix is singular.
	*/
   void solve(int nx, double* X, double const* B) const;
private:
   /**
	* Internal representation of LU decomposition, in row major order.
	*/
   std::vector<double> m_LU;

   /**
	* The pivot vector.
	*/
   std::vector<int> m_piv;

   /**
	* Pivot sign.
	*/
   int m_pivSign;

   /**
	* Matrix dimensions.
	*/
   int m_rows, m_cols;
};

#endif
