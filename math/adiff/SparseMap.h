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

#ifndef ADIFF_SPARSITY_H_
#define ADIFF_SPARSITY_H_

#include "MatT.h"
#include <stddef.h>

namespace adiff
{

/**
 * Specifies which derivatives are non-zero and therefore need to be calculated.
 * Each such non-zero derivative is assigned an index in [0, total_nonzero_derivs).
 */
template<int ORD> class SparseMap;

/**
 * The second order sparse map specified which elements of the Hessian
 * matrix are non-zero.
 */
template<>
class SparseMap<2>
{
	// Member-wise copying is OK.
public:
	static size_t const ZERO_ELEMENT;

	/**
	 * Creates a structure for a (num_vars)x(num_vars) Hessian
	 * with all elements being initially considered as zero.
	 */
	explicit SparseMap(size_t num_vars);

	/**
	 * Returns N for an NxN Hessian.
	 */
	size_t numVars() const { return m_numVars; }

	/**
	 * \brief Marks an element at (i, j) as non-zero.
	 *
	 * Calling this function on an element already marked non-zero
	 * has no effect.
	 */
	void markNonZero(size_t i, size_t j);

	/**
	 * \brief Marks all elements as non-zero.
	 *
	 * The caller shouldn't assume any particular pattern of index
	 * assignment when using this function.
	 */
	void markAllNonZero();

	/**
	 * Returns the number of elements marked as non-zero.
	 */
	size_t numNonZeroElements() const { return m_numNonZeroElements; } 

	/**
	 * Returns an index in the range of [0, numNonZeroElements)
	 * associated with the element, or ZERO_ELEMENT, if the element
	 * wasn't marked non-zero.
	 */
	size_t nonZeroElementIdx(size_t i, size_t j) const;
private:
	size_t m_numVars;
	size_t m_numNonZeroElements;
	MatT<size_t> m_map;
};

} // namespace adiff

#endif
