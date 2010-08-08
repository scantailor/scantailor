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

#include "LinearSolver.h"

LinearSolver::LinearSolver(size_t rows_AB, size_t cols_A_rows_X, size_t cols_BX)
:	m_rowsAB(rows_AB),
	m_colsArowsX(cols_A_rows_X),
	m_colsBX(cols_BX)
{
	if (m_rowsAB < m_colsArowsX) {
		throw std::runtime_error("LinearSolver: can's solve underdetermined systems");
	}
}
