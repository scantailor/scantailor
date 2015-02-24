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

#ifndef MATRIX_CALC_H_
#define MATRIX_CALC_H_

#include "NonCopyable.h"
#include "StaticPool.h"
#include "DynamicPool.h"
#include "LinearSolver.h"
#include "MatMNT.h"
#include "MatT.h"
#include "VecNT.h"
#include "VecT.h"
#include <stddef.h>
#include <assert.h>

template<typename T, typename Alloc> class MatrixCalc;

namespace mcalc
{

template<typename T>
class AbstractAllocator
{
public:
	virtual T* allocT(size_t size) = 0;
	
	virtual size_t* allocP(size_t size) = 0;
};


template<typename T, size_t TSize, size_t PSize>
class StaticPoolAllocator : public AbstractAllocator<T>
{
public:
	virtual T* allocT(size_t size) { return m_poolT.alloc(size); }

	virtual size_t* allocP(size_t size) { return m_poolP.alloc(size); }
private:
	StaticPool<size_t, PSize> m_poolP;
	StaticPool<T, TSize> m_poolT;
};


template<typename T>
class DynamicPoolAllocator : public AbstractAllocator<T>
{
public:
	virtual T* allocT(size_t size) { return m_poolT.alloc(size); }

	virtual size_t* allocP(size_t size) { return m_poolP.alloc(size); }
private:
	DynamicPool<size_t> m_poolP;
	DynamicPool<T> m_poolT;
};


template<typename T>
class Mat
{
	template<typename OT, typename Alloc> friend class ::MatrixCalc;
	template<typename OT> friend Mat<OT> operator+(Mat<OT> const& m1, Mat<OT> const& m2);
	template<typename OT> friend Mat<OT> operator-(Mat<OT> const& m1, Mat<OT> const& m2);
	template<typename OT> friend Mat<OT> operator*(Mat<OT> const& m1, Mat<OT> const& m2);
	template<typename OT> friend Mat<OT> operator*(OT scalar, Mat<OT> const& m);
	template<typename OT> friend Mat<OT> operator*(Mat<OT> const& m, OT scalar);
	template<typename OT> friend Mat<OT> operator/(Mat<OT> const& m, OT scalar);
public:
	Mat inv() const;

	Mat solve(Mat const& b) const;

	Mat solve(T const* data, int rows, int cols) const;

	Mat trans() const;

	Mat write(T* buf) const;

	template<size_t N>
	Mat write(VecNT<N, T>& vec) const;

	Mat transWrite(T* buf) const;

	template<size_t N>
	Mat transWrite(VecNT<N, T>& vec) const;

	Mat operator-() const;

	T const* rawData() const { return data; }
private:
	Mat(AbstractAllocator<T>* alloc, T const* data, int rows, int cols)
		: alloc(alloc), data(data), rows(rows), cols(cols) {}

	AbstractAllocator<T>* alloc;
	T const* data;
	int rows;
	int cols;
};

} // namespace mcalc


template<typename T, typename Alloc = mcalc::StaticPoolAllocator<T, 128, 9> >
class MatrixCalc
{
	DECLARE_NON_COPYABLE(MatrixCalc)
public:
	MatrixCalc() {}

	mcalc::Mat<T> operator()(T const* data, int rows, int cols) {
		return mcalc::Mat<T>(&m_alloc, data, rows, cols);
	}

	template<size_t N>
	mcalc::Mat<T> operator()(VecNT<N, T> const& vec, int rows, int cols) {
		return mcalc::Mat<T>(&m_alloc, vec.data(), rows, cols);
	}

	template<size_t M, size_t N>
	mcalc::Mat<T> operator()(MatMNT<M, N, T> const& mat) {
		return mcalc::Mat<T>(&m_alloc, mat.data(), mat.ROWS, mat.COLS);
	}

	mcalc::Mat<T> operator()(MatT<T> const& mat) {
		return mcalc::Mat<T>(&m_alloc, mat.data(), mat.rows(), mat.cols());
	}

	template<size_t N>
	mcalc::Mat<T> operator()(VecNT<N, T> const& vec) {
		return mcalc::Mat<T>(&m_alloc, vec.data(), vec.SIZE, 1);
	}

	mcalc::Mat<T> operator()(VecT<T> const& vec) {
		return mcalc::Mat<T>(&m_alloc, vec.data(), vec.size(), 1);
	}
private:
	Alloc m_alloc;
};


template<typename T, size_t TSize = 128, size_t PSize = 9>
class StaticMatrixCalc : public MatrixCalc<T, mcalc::StaticPoolAllocator<T, TSize, PSize> >
{
};


template<typename T>
class DynamicMatrixCalc : public MatrixCalc<T, mcalc::DynamicPoolAllocator<T> >
{
};


/*========================== Implementation =============================*/

namespace mcalc
{

template<typename T>
Mat<T>
Mat<T>::inv() const
{
	assert(cols == rows);

	T* ident_data = alloc->allocT(rows * cols);
	Mat ident(alloc, ident_data, rows, cols);
	int const todo = rows * cols;
	for (int i = 0; i < todo; ++i) {
		ident_data[i] = T();
	}
	for (int i = 0; i < todo; i += rows + 1) {
		ident_data[i] = T(1);
	}

	return solve(ident);
}

template<typename T>
Mat<T>
Mat<T>::solve(Mat const& b) const
{
	assert(rows == b.rows);

	T* x_data = alloc->allocT(cols * b.cols);
	T* tbuffer = alloc->allocT(cols * (b.rows + b.cols));
	size_t* pbuffer = alloc->allocP(rows);
	LinearSolver(rows, cols, b.cols).solve(data, x_data, b.data, tbuffer, pbuffer);

	return Mat(alloc, x_data, cols, b.cols);
}

template<typename T>
Mat<T>
Mat<T>::solve(T const* data, int rows, int cols) const
{
	return solve(Mat(alloc, data, rows, cols));
}

template<typename T>
Mat<T>
Mat<T>::trans() const
{
	if (cols == 1 || rows == 1) {
		return Mat<T>(alloc, data, cols, rows);
	}

	T* p_trans = alloc->allocT(cols * rows);
	transWrite(p_trans);
	return Mat(alloc, p_trans, cols, rows);
}

template<typename T>
Mat<T>
Mat<T>::write(T* buf) const
{
	int const todo = rows * cols;
	for (int i = 0; i < todo; ++i) {
		buf[i] = data[i];
	}

	return *this;
}

template<typename T>
template<size_t N>
Mat<T>
Mat<T>::write(VecNT<N, T>& vec) const
{
        assert(N >= size_t(rows * cols));
	return write(vec.data());
}

template<typename T>
Mat<T>
Mat<T>::transWrite(T* buf) const
{
	T* p_trans = buf;
	for (int i = 0; i < rows; ++i) {
		T const* p_src = data + i;
		for (int j = 0; j < cols; ++j) {
			*p_trans = *p_src;
			++p_trans;
			p_src += rows;
		}
	}

	return *this;
}

template<typename T>
template<size_t N>
Mat<T>
Mat<T>::transWrite(VecNT<N, T>& vec) const
{
	assert(N >= rows * cols);
	return transWrite(vec.data());
}

/** Unary minus. */
template<typename T>
Mat<T>
Mat<T>::operator-() const
{
	T* p_res = alloc->allocT(rows * cols);
	Mat<T> res(alloc, p_res, rows, cols);

	int const todo = rows * cols;
	for (int i = 0; i < todo; ++i) {
		p_res[i] = -data[i];
	}

	return res;
}

template<typename T>
Mat<T> operator+(Mat<T> const& m1, Mat<T> const& m2)
{
	assert(m1.rows == m2.rows && m1.cols == m2.cols);

	T* p_res = m1.alloc->allocT(m1.rows * m1.cols);
	Mat<T> res(m1.alloc, p_res, m1.rows, m1.cols);

	int const todo = m1.rows * m1.cols;
	for (int i = 0; i < todo; ++i) {
		p_res[i] = m1.data[i] + m2.data[i];
	}

	return res;
}

template<typename T>
Mat<T> operator-(Mat<T> const& m1, Mat<T> const& m2)
{
	assert(m1.rows == m2.rows && m1.cols == m2.cols);

	T* p_res = m1.alloc->allocT(m1.rows * m1.cols);
	Mat<T> res(m1.alloc, p_res, m1.rows, m1.cols);
	
	int const todo = m1.rows * m1.cols;
	for (int i = 0; i < todo; ++i) {
		p_res[i] = m1.data[i] - m2.data[i];
	}

	return res;
}

template<typename T>
Mat<T> operator*(Mat<T> const& m1, Mat<T> const& m2)
{
	assert(m1.cols == m2.rows);

	T* p_res = m1.alloc->allocT(m1.rows * m2.cols);
	Mat<T> res(m1.alloc, p_res, m1.rows, m2.cols);

	for (int rcol = 0; rcol < res.cols; ++rcol) {
		for (int rrow = 0; rrow < res.rows; ++rrow) {
			T const* p_m1 = m1.data + rrow;
			T const* p_m2 = m2.data + rcol * m2.rows;
			T sum = T();
			for (int i = 0; i < m1.cols; ++i) {
				sum += *p_m1 * *p_m2;
				p_m1 += m1.rows;
				++p_m2;
			}
			*p_res = sum;
			++p_res;
		}
	}

	return res;
}

template<typename T>
Mat<T> operator*(T scalar, Mat<T> const& m)
{
	T* p_res = m.alloc->allocT(m.rows * m.cols);
	Mat<T> res(m.alloc, p_res, m.rows, m.cols);
	
	int const todo = m.rows * m.cols;
	for (int i = 0; i < todo; ++i) {
		p_res[i] = m.data[i] * scalar;
	}

	return res;
}

template<typename T>
Mat<T> operator*(Mat<T> const& m, T scalar)
{
	return scalar * m;
}

template<typename T>
Mat<T> operator/(Mat<T> const& m, T scalar)
{
	return m * (1.0f / scalar);
}

} // namespace mcalc

#endif
