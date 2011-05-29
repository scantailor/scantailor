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

#ifndef HOMOGRAPHIC_TRANSFORM_H_
#define HOMOGRAPHIC_TRANSFORM_H_

#include "VecNT.h"
#include "MatrixCalc.h"
#include <stddef.h>

template<size_t N, typename T> class HomographicTransform;

template<size_t N, typename T>
class HomographicTransformBase
{
public:
	typedef VecNT<N, T> Vec;
	typedef VecNT<(N+1)*(N+1), T> Mat;

	explicit HomographicTransformBase(Mat const& mat) : m_mat(mat) {}

	HomographicTransform<N, T> inv() const;

	Vec operator()(Vec const& from) const;

	Mat const& mat() const { return m_mat; }
private:
	Mat m_mat;
};


template<size_t N, typename T>
class HomographicTransform : public HomographicTransformBase<N, T>
{
public:
	explicit HomographicTransform(
		typename HomographicTransformBase<N, T>::Mat const& mat) : HomographicTransformBase<N, T>(mat) {}
};


/** An optimized, both in terms of API and performance, 1D version. */
template<typename T>
class HomographicTransform<1, T> : public HomographicTransformBase<1, T>
{
public:
	explicit HomographicTransform(
		typename HomographicTransformBase<1, T>::Mat const& mat)
			: HomographicTransformBase<1, T>(mat) {}

	T operator()(T from) const;

	// Prevent it's shadowing by the above one.
	using HomographicTransformBase<1, T>::operator();
};


template<size_t N, typename T>
HomographicTransform<N, T>
HomographicTransformBase<N, T>::inv() const
{
	StaticMatrixCalc<T, 4*(N+1)*(N+1), N+1> mc;
	Mat inv_mat;
	mc(m_mat, N+1, N+1).inv().write(inv_mat);
	return HomographicTransform<N, T>(inv_mat);
}

template<size_t N, typename T>
typename HomographicTransformBase<N, T>::Vec
HomographicTransformBase<N, T>::operator()(Vec const& from) const
{
	StaticMatrixCalc<T, N+1, 1> mc;
	VecNT<N+1, T> const hsrc(from, T(1));
	VecNT<N+1, T> hdst;
	(mc(m_mat, N+1, N+1)*mc(hsrc, N+1, 1)).write(hdst);
	VecNT<N, T> res(&hdst[0]);
	res /= hdst[N];
	return res;
}

template<typename T>
T
HomographicTransform<1, T>::operator()(T from) const
{
	// Optimized version for 1D case.
	T const* m = this->mat().data();
	return (from * m[0] + m[2]) / (from * m[1] + m[3]);
}

#endif
