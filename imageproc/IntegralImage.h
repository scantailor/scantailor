/*
    Scan Tailor - Interactive post-processing tool for scanned pages.
    Copyright (C) 2007-2008  Joseph Artsimovich <joseph_a@mail.ru>

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

#ifndef IMAGEPROC_INTEGRALIMAGE_H_
#define IMAGEPROC_INTEGRALIMAGE_H_

#include "NonCopyable.h"
#include <QSize>
#include <QRect>
#include <new>

namespace imageproc
{

/**
 * \brief Provides the sum of values in a sub-rectangle of a 2D array in constant time.
 *
 * Suppose you have a MxN array of some values.  Now if we are not going to
 * alter it, but are going to calculate the sum of values in various
 * rectangular sub-regions, it's best to use an integral image for this purpose.
 * We construct it once, with complexity of O(M*N), and then obtain the sum
 * of values in a rectangular sub-region in O(1).
 *
 * \note Template parameter T must be large enough to hold the sum of all
 *       values in the array.
 */
template<typename T>
class IntegralImage
{
	DECLARE_NON_COPYABLE(IntegralImage)
public:
	IntegralImage(int width, int height);
	
	explicit IntegralImage(QSize const& size);
	
	~IntegralImage();
	
	/**
	 * \brief To be called before pushing new row data.
	 */
	void beginRow();
	
	/**
	 * \brief Push a single value to the integral image.
	 *
	 * Values must be pushed row by row, starting from row 0, and from
	 * column to column within each row, starting from column 0.
	 * At the beginning of a row, a call to beginRow() must be made.
	 *
	 * \note Pushing more than width * height values results in undefined
	 *       behavior.
	 */
	void push(T val);
	
	/**
	 * \brief Calculate the sum of values in the given rectangle.
	 *
	 * \note If the rectangle exceeds the image area, the behaviour is
	 *       undefined.
	 */
	T sum(QRect const& rect) const;
private:
	void init(int width, int height);
	
	T* m_pData;
	T* m_pCur;
	T* m_pAbove;
	T m_lineSum;
	int m_width;
	int m_height;
	
};

template<typename T>
IntegralImage<T>::IntegralImage(int const width, int const height)
:	m_lineSum() // init with 0 or with default constructor.
{
	// The first row and column are fake.
	init(width + 1, height + 1);
}

template<typename T>
IntegralImage<T>::IntegralImage(QSize const& size)
:	m_lineSum() // init with 0 or with default constructor.
{
	// The first row and column are fake.
	init(size.width() + 1, size.height() + 1);
}

template<typename T>
IntegralImage<T>::~IntegralImage()
{
	delete[] m_pData;
}

template<typename T>
void
IntegralImage<T>::init(int const width, int const height)
{
	m_width = width;
	m_height = height;
	
	m_pData = new T[width * height];
	
	// Initialize the first (fake) row.
	// As for the fake column, we initialize its elements in beginRow().
	T* p = m_pData;
	for (int i = 0; i < width; ++i, ++p) {
		*p = T();
	}
	
	m_pAbove = m_pData;
	m_pCur = m_pAbove + width; // Skip the first row.
}

template<typename T>
void
IntegralImage<T>::push(T const val)
{
	m_lineSum += val;
	*m_pCur = *m_pAbove + m_lineSum;
	++m_pCur;
	++m_pAbove;
}

template<typename T>
void
IntegralImage<T>::beginRow()
{
	m_lineSum = T();
	
	// Initialize and skip the fake column.
	*m_pCur = T();
	++m_pCur;
	++m_pAbove;
}

template<typename T>
inline T
IntegralImage<T>::sum(QRect const& rect) const
{
	// Keep in mind that row 0 and column 0 are fake.
	int const pre_left = rect.left();
	int const pre_right = rect.right() + 1; // QRect::right() is inclusive.
	int const pre_top = rect.top();
	int const pre_bottom = rect.bottom() + 1; // QRect::bottom() is inclusive.
	T sum(m_pData[pre_bottom * m_width + pre_right]);
	sum -= m_pData[pre_top * m_width + pre_right];
	sum += m_pData[pre_top * m_width + pre_left];
	sum -= m_pData[pre_bottom * m_width + pre_left];
	return sum;
}

} // namespace imageproc

#endif
