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

#include "SparseMap.h"

namespace adiff
{

size_t const SparseMap<2>::ZERO_ELEMENT = ~size_t(0);

SparseMap<2>::SparseMap(size_t num_vars)
: m_numVars(num_vars)
, m_numNonZeroElements(0)
, m_map(num_vars, num_vars, ZERO_ELEMENT)
{
}

void
SparseMap<2>::markNonZero(size_t i, size_t j)
{
	size_t& el = m_map(i, j);
	if (el == ZERO_ELEMENT) {
		el = m_numNonZeroElements;
		++m_numNonZeroElements;
	}
}

void
SparseMap<2>::markAllNonZero()
{
	for (size_t i = 0; i < m_numVars; ++i) {
		for (size_t j = 0; j < m_numVars; ++j) {
			markNonZero(i, j);
		}
	}
}

size_t
SparseMap<2>::nonZeroElementIdx(size_t i, size_t j) const
{
	return m_map(i, j);
}

} // namespace adiff
