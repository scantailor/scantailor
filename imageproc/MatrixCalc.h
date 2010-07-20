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

#ifndef IMAGEPROC_MATRIX_CALC_H_
#define IMAGEPROC_MATRIX_CALC_H_

#include "NonCopyable.h"
#include "StaticPool.h"
#include <QtGlobal> // for qAbs
#include <stdexcept>
#include <string>
#include <stddef.h>
#include <assert.h>
#include <math.h>

namespace imageproc
{

template<typename T, size_t TSize, size_t PSize> class MatrixCalc;

namespace mcalc
{

template<typename T>
class AbstractStaticAllocator
{
public:
	virtual T* allocT(size_t size) = 0;
	
	virtual size_t* allocP(size_t size) = 0;
};


template<typename T>
class Mat
{
	template<typename OT, size_t TSize, size_t PSize> friend class MatrixCalc;
	template<typename OT> friend Mat<OT> operator+(Mat<OT> const& m1, Mat<OT> const& m2);
	template<typename OT> friend Mat<OT> operator-(Mat<OT> const& m1, Mat<OT> const& m2);
	template<typename OT> friend Mat<OT> operator*(Mat<OT> const& m1, Mat<OT> const& m2);
	template<typename OT> friend Mat<OT> operator*(OT scalar, Mat<OT> const& m);
	template<typename OT> friend Mat<OT> operator*(Mat<OT> const& m, OT scalar);
	template<typename OT> friend Mat<OT> operator/(Mat<OT> const& m, OT scalar);
public:
	Mat inv() const;

	Mat solve(Mat const& b) const;

	Mat trans() const;

	void write(T* buf) const;

	void transWrite(T* buf) const;
private:
	Mat(AbstractStaticAllocator<T>* alloc, T const* data, int rows, int cols)
		: alloc(alloc), data(data), rows(rows), cols(cols) {}

	AbstractStaticAllocator<T>* alloc;
	T const* data;
	int rows;
	int cols;
};

} // namespace mcalc


template<typename T, size_t TSize = 128, size_t PSize = 9>
class MatrixCalc : private mcalc::AbstractStaticAllocator<T>
{
	DECLARE_NON_COPYABLE(MatrixCalc)
public:
	MatrixCalc() {}

	mcalc::Mat<T> operator()(int rows, int cols, T const* data) {
		return mcalc::Mat<T>(this, data, rows, cols);
	}
private:
	virtual T* allocT(size_t size) { return m_poolT.alloc(size); }

	virtual size_t* allocP(size_t size) { return m_poolP.alloc(size); }

	StaticPool<size_t, PSize> m_poolP;
	StaticPool<T, TSize> m_poolT;
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

	if (rows < cols) {
		throw std::runtime_error("Can's solve underdetermined systems");
	}

	T const epsilon(1e-6);

	T* const lu_data = alloc->allocT(rows * cols);
	Mat lu(alloc, lu_data, rows, cols);

	// Copy this matrix to lu.
	int const num_elements = rows * cols;
	for (int i = 0; i < num_elements; ++i) {
		lu_data[i] = data[i];
	}
	
	// Maps virtual row numbers to physical ones.
	size_t* perm = alloc->allocP(rows);
	for (int i = 0; i < rows; ++i) {
		perm[i] = i;
	}

	T* p_col = lu_data;
	for (int i = 0; i < cols; ++i, p_col += rows) {
		// Find the largest pivot.
		int virt_pivot_row = i;
		T largest_abs_pivot(qAbs(p_col[perm[i]]));
		for (int j = i + 1; j < rows; ++j) {
			T const abs_pivot(qAbs(p_col[perm[j]]));
			if (abs_pivot > largest_abs_pivot) {
				largest_abs_pivot = abs_pivot;
				virt_pivot_row = j;
			}
		}
		
		if (largest_abs_pivot <= epsilon) {
			throw std::runtime_error("Matrix is not full rank");
		}

		size_t const phys_pivot_row(perm[virt_pivot_row]);
		perm[virt_pivot_row] = perm[i];
		perm[i] = phys_pivot_row;
		
		T const* const p_pivot = p_col + phys_pivot_row;
		T const r_pivot(1.0 / *p_pivot);
		
		// Eliminate entries below the pivot.
		for (int j = i + 1; j < rows; ++j) {
			T const* p1 = p_pivot;
			T* p2 = p_col + perm[j];
			if (qAbs(*p2) <= epsilon) {
				// We consider it's already zero.
				*p2 = T();
				continue;
			}

			T const factor(*p2 * r_pivot);
			*p2 = factor; // Factor goes into L, zero goes into U.

			// Transform the rest of the row.
			for (int col = i + 1; col < cols; ++col) {
				p1 += rows;
				p2 += rows;
				*p2 -= *p1 * factor;
			}
		}
	}

#if 0
	qDebug() << "L =";
	for (int row = 0; row < rows; ++row) {
		QDebug dbg(qDebug());
		for (int col = 0; col < cols; ++col) {
			if (col == row) {
				dbg << 1;
			} else if (col > row) {
				dbg << 0;
			} else {
				dbg << lu_data[col * rows + perm[row]];
			}
		}
	}

	qDebug() << "U =";
	for (int row = 0; row < rows; ++row) {
		QDebug dbg(qDebug());
		for (int col = 0; col < cols; ++col) {
			if (col < row) {
				dbg << 0;
			} else {
				dbg << lu_data[col * rows + perm[row]];
			}
		}
	}
#endif
	
	// First solve Ly = b
	T* const y_data = alloc->allocT(cols * b.cols);
	Mat y(alloc, y_data, cols, b.cols);
	T* p_y_col = y_data;
	T const* p_b_col = b.data;
	for (int y_col = 0; y_col < y.cols; ++y_col) {
		int virt_row = 0;
		for (; virt_row < cols; ++virt_row) {
			int const phys_row = perm[virt_row];
			T right(p_b_col[phys_row]);
			
			// Move already calculated factors to the right side.
			T const* p_lu = lu_data + phys_row;
			// Go left to right, stop at diagonal.
			for (int lu_col = 0; lu_col < virt_row; ++lu_col) {
				right -= *p_lu * p_y_col[lu_col];
				p_lu += rows;
			}

			// We assume L has ones on the diagonal, so no division here.
			p_y_col[virt_row] = right;
		}

		// Continue below the square part (if any).
		for (; virt_row < rows; ++virt_row) {
			int const phys_row = perm[virt_row];
			T right(p_b_col[phys_row]);

			// Move everything to the right side, then verify it's zero.
			T const* p_lu = lu_data + phys_row;
			// Go left to right all the way.
			for (int lu_col = 0; lu_col < cols; ++lu_col) {
				right -= *p_lu * p_y_col[lu_col];
				p_lu += rows;
			}
			if (qAbs(right) > epsilon) {
				throw std::runtime_error("Inconsistent overdetermined system");
			}
		}

		p_y_col += cols;
		p_b_col += rows;
	}

	// Now solve Ux = y
	T* const x_data = alloc->allocT(cols * b.cols);
	Mat x(alloc, x_data, cols, b.cols);
	T* p_x_col = x_data;
	p_y_col = y_data;
	T const* p_lu_last_col = lu_data + (cols - 1) * rows;
	for (int x_col = 0; x_col < x.cols; ++x_col) {
		for (int virt_row = cols - 1; virt_row >= 0; --virt_row) {
			T right(p_y_col[virt_row]);
			
			// Move already calculated factors to the right side.
			T const* p_lu = p_lu_last_col + perm[virt_row];
			// Go right to left, stop at diagonal.
			for (int lu_col = cols - 1; lu_col > virt_row; --lu_col) {
				right -= *p_lu * p_x_col[lu_col];
				p_lu -= rows;
			}
			p_x_col[virt_row] = right / *p_lu; 
		}

		p_x_col += cols;
		p_y_col += cols;
	}

	return x;
}

template<typename T>
Mat<T>
Mat<T>::trans() const
{
	T* p_trans = alloc->allocT(cols * rows);
	transWrite(p_trans);
	return Mat(alloc, p_trans, cols, rows);
}

template<typename T>
void
Mat<T>::write(T* buf) const
{
	int const todo = rows * cols;
	for (int i = 0; i < todo; ++i) {
		buf[i] = data[i];
	}
}

template<typename T>
void
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
}

template<typename T>
Mat<T> operator+(Mat<T> const& m1, Mat<T> const& m2)
{
	assert(m1.rows == m2.rows && m1.cols == m2.cols);

	T* p_res = m1.alloc->allocT(m1.rows * m1.cols);
	Mat<T> res(m1.alloc, p_res, m1.rows, m1.cols);
	
	T const* p_m1 = m1.data;
	T const* p_m2 = m2.data;
	
	int const todo = m1.rows * m1.cols;
	for (int i = 0; i < todo; ++i) {
		res.data[i] = m1.data[i] + m2.data[i];
	}
}

template<typename T>
Mat<T> operator-(Mat<T> const& m1, Mat<T> const& m2)
{
	assert(m1.rows == m2.rows && m1.cols == m2.cols);

	T const* p_res = m1.alloc->allocT(m1.rows * m1.cols);
	Mat<T> res(m1.alloc, p_res, m1.rows, m1.cols);

	T const* p_m1 = m1.data;
	T const* p_m2 = m2.data;
	
	int const todo = m1.rows * m1.cols;
	for (int i = 0; i < todo; ++i) {
		res.data[i] = m1.data[i] - m2.data[i];
	}
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
	T const* p_m = m.data;
	
	int const todo = m.rows * m.cols;
	for (int i = 0; i < todo; ++i) {
		res.data[i] = m.data[i] * scalar;
	}
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

} // namespace imageproc

#endif
