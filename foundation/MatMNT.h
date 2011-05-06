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

#ifndef MAT_MNT_H_
#define MAT_MNT_H_

#include <stddef.h>

template<size_t M, size_t N, typename T> class MatMNT;
typedef MatMNT<2, 2, float> Mat22f;
typedef MatMNT<2, 2, double> Mat22d;
typedef MatMNT<3, 3, float> Mat33f;
typedef MatMNT<3, 3, double> Mat33d;
typedef MatMNT<4, 4, float> Mat44f;
typedef MatMNT<4, 4, double> Mat44d;

/**
 * \brief A matrix with pre-defined dimensions.
 *
 * \note The memory layout is always column-major, as that's what MatrixCalc uses.
 */
template<size_t M, size_t N, typename T>
class MatMNT
{
public:
	typedef T type;
	enum { ROWS = M, COLS = N };

	/**
	 * \brief Initializes matrix elements to T().
	 */
	MatMNT();

	/**
	 * \brief Construction from an array of elements of possibly different type.
	 *
	 * Conversion is done by static casts.  Data elements must be in column-major order.
	 */
	template<typename OT>
	explicit MatMNT(OT const* data);

	/**
	 * \brief Construction from a matrix of same dimensions but another type.
	 *
	 * Conversion is done by static casts.
	 */
	template<typename OT>
	MatMNT(MatMNT<M, N, OT> const& other);

	/**
	 * \brief Assignment from a matrix of same dimensions but another type.
	 *
	 * Conversion is done by static casts.
	 */
	template<typename OT>
	MatMNT& operator=(MatMNT<M, N, OT> const& other);

	T const* data() const { return m_data; }

	T* data() { return m_data; }

	T const& operator()(int i, int j) const {
		return m_data[i + j * M];
	}

	T& operator()(int i, int j) {
		return m_data[i + j * M];
	}
private:
	T m_data[M*N];
};


template<size_t M, size_t N, typename T>
MatMNT<M, N, T>::MatMNT()
{
	size_t const len = ROWS*COLS;
	for (size_t i = 0; i < len; ++i) {
		m_data[i] = T();
	}
}

template<size_t M, size_t N, typename T>
template<typename OT>
MatMNT<M, N, T>::MatMNT(OT const* data)
{
	size_t const len = ROWS*COLS;
	for (size_t i = 0; i < len; ++i) {
		m_data[i] = static_cast<T>(data[i]);
	}
}

template<size_t M, size_t N, typename T>
template<typename OT>
MatMNT<M, N, T>::MatMNT(MatMNT<M, N, OT> const& other)
{
	OT const* data = other.data();
	size_t const len = ROWS*COLS;
	for (size_t i = 0; i < len; ++i) {
		m_data[i] = static_cast<T>(data[i]);
	}
}

#endif
