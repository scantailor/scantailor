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

#ifndef IMAGEPROC_HOMOGRAPHIC_TRANSFORM_H_
#define IMAGEPROC_HOMOGRAPHIC_TRANSFORM_H_

#include "VecNT.h"
#include "MatrixCalc.h"
#include <stddef.h>

namespace imageproc
{

template<size_t N, typename T>
class HomographicTransform
{
	typedef VecNT<N, T> Vec;
	typedef VecNT<(N+1)*(N+1), T> Mat;
public:
	explicit HomographicTransform(Mat const& mat) : m_mat(mat) {}

	HomographicTransform inv() const;

	Vec operator()(Vec const& from) const;

	Mat const& mat() const { return m_mat; }
private:
	Mat m_mat;
};

template<size_t N, typename T>
HomographicTransform<N, T>
HomographicTransform<N, T>::inv() const
{
	MatrixCalc<T, 4*(N+1)*(N+1), N+1> mc;
	Mat inv_mat;
	mc(N+1, N+1, m_mat).inv().write(inv_mat);
	return HomographicTransform(inv_mat);
}

template<size_t N, typename T>
typename HomographicTransform<N, T>::Vec
HomographicTransform<N, T>::operator()(Vec const& from) const
{
	MatrixCalc<T, N+1, 1> mc;
	VecNT<N+1, T> const hsrc(from, T(1));
	VecNT<N+1, T> hdst;
	(mc(N+1, N+1, m_mat)*mc(N+1, 1, hsrc)).write(hdst);
	VecNT<N, T> res(&hdst[0]);
	res /= hdst[N];
	return res;
}

} // namespace imageproc

#endif
