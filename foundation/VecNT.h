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

#ifndef VEC_NT_H_
#define VEC_NT_H_

#include <QPointF>
#include <stddef.h>

template<size_t N, typename T> class VecNT;
typedef VecNT<1, float> Vec1f;
typedef VecNT<1, double> Vec1d;
typedef VecNT<2, float> Vec2f;
typedef VecNT<2, double> Vec2d;
typedef VecNT<3, float> Vec3f;
typedef VecNT<3, double> Vec3d;
typedef VecNT<4, float> Vec4f;
typedef VecNT<4, double> Vec4d;

template<size_t N, typename T>
class VecNT
{
public:
	typedef T type;
	enum { SIZE = N }; 

	/**
	 * \brief Initializes vector elements to T().
	 */
	VecNT();

	/**
	 * \brief Construction from an array of elements of possibly different type.
	 *
	 * Conversion is done by static casts.
	 */
	template<typename OT>
	explicit VecNT(OT const* data);

	/**
	 * \brief Construction from a vector of same dimension but another type.
	 *
	 * Conversion is done by static casts.
	 */
	template<typename OT>
	VecNT(VecNT<N, OT> const& other);

	/**
	 * \brief Construction from a one-less dimensional
	 *        vector and the last element value.
	 */
	template<typename OT>
	VecNT(VecNT<N-1, OT> const& lesser, T last);

	/**
	 * \brief 1D vector constructor.
	 *
	 * Will not compile for different dimensions.
	 */
	explicit VecNT(T x);

	/**
	 * \brief 2D vector constructor.
	 *
	 * Will not compile for different dimensions.
	 */
	VecNT(T x, T y);

	/**
	 * \brief 3D vector constructor.
	 *
	 * Will not compile for different dimensions.
	 */
	VecNT(T x, T y, T z);

	/**
	 * \brief 4D vector constructor.
	 *
	 * Will not compile for different dimensions.
	 */
	VecNT(T x, T y, T z, T w);

	/**
	 * \brief Construction from a QPointF.
	 *
	 * Will not compile for N != 2.  Will compile for any T's that
	 * are convertible from qreal by a static cast.
	 */
	VecNT(QPointF const& pt);

	/**
	 * \brief Implicit conversion to QPointF.
	 *
	 * Will not compile for N != 2.  Will compile for any T's that
	 * are convertible to qreal by a static cast.
	 */
	operator QPointF() const; 

	/**
	 * \brief Assignment from a vector of same dimension but another type.
	 *
	 * Conversion is done by static casts.
	 */
	template<typename OT>
	VecNT& operator=(VecNT<N, OT> const& other);

	T& operator[](size_t idx) { return m_data[idx]; }

	T const& operator[](size_t idx) const { return m_data[idx]; }
	
	VecNT& operator+=(T scalar);

	VecNT& operator+=(VecNT const& other);

	VecNT& operator-=(T scalar);

	VecNT& operator-=(VecNT const& other);

	VecNT& operator*=(T scalar);

	VecNT& operator/=(T scalar);

	T const* data() const { return m_data; }

	T* data() { return m_data; }

	/**
	 * \brief Sums elements in the vector.
	 */
	T sum() const;

	T dot(VecNT const& other) const;

	T squaredNorm() const { return dot(*this); }
private:
	T m_data[N];
};


namespace vecnt
{

template<size_t N, typename T> struct SizeSpecific;

template<typename T>
struct SizeSpecific<1, T>
{
	static void assign(T* data, T x) {
		data[0] = x;
	}
};

template<typename T>
struct SizeSpecific<2, T>
{
	static void assign(T* data, T x, T y) {
		data[0] = x;
		data[1] = y;
	}

	static void assign(T* data, QPointF const& pt) {
		data[0] = static_cast<T>(pt.x());
		data[1] = static_cast<T>(pt.y());
	}

	static QPointF toQPointF(T const* data) {
		return QPointF(static_cast<qreal>(data[0]), static_cast<qreal>(data[1]));
	}
};

template<typename T>
struct SizeSpecific<3, T>
{
	static void assign(T* data, T x, T y, T z) {
		data[0] = x;
		data[1] = y;
		data[2] = z;
	}
};

template<typename T>
struct SizeSpecific<4, T>
{
	static void assign(T* data, T x, T y, T z, T w) {
		data[0] = x;
		data[1] = y;
		data[2] = z;
		data[3] = w;
	}
};

} // namespace vecnt


template<size_t N, typename T>
VecNT<N, T>::VecNT()
{
	for (size_t i = 0; i < N; ++i) {
		m_data[i] = T();
	}
}

template<size_t N, typename T>
template<typename OT>
VecNT<N, T>::VecNT(OT const* data)
{
	for (size_t i = 0; i < N; ++i) {
		m_data[i] = static_cast<T>(data[i]);
	}
}

template<size_t N, typename T>
template<typename OT>
VecNT<N, T>::VecNT(VecNT<N, OT> const& other)
{
	for (size_t i = 0; i < N; ++i) {
		m_data[i] = static_cast<T>(other[i]);
	}
}

template<size_t N, typename T>
template<typename OT>
VecNT<N, T>::VecNT(VecNT<N-1, OT> const& lesser, T last)
{
	for (size_t i = 0; i < N-1; ++i) {
		m_data[i] = static_cast<T>(lesser[i]);
	}
	m_data[N-1] = last;
}

template<size_t N, typename T>
VecNT<N, T>::VecNT(T x)
{
	vecnt::SizeSpecific<N, T>::assign(m_data, x);
}

template<size_t N, typename T>
VecNT<N, T>::VecNT(T x, T y)
{
	vecnt::SizeSpecific<N, T>::assign(m_data, x, y);
}

template<size_t N, typename T>
VecNT<N, T>::VecNT(T x, T y, T z)
{
	vecnt::SizeSpecific<N, T>::assign(m_data, x, y, z);
}

template<size_t N, typename T>
VecNT<N, T>::VecNT(T x, T y, T z, T w)
{
	vecnt::SizeSpecific<N, T>::assign(m_data, x, y, z, w);
}

template<size_t N, typename T>
VecNT<N, T>::VecNT(QPointF const& pt)
{
	vecnt::SizeSpecific<N, T>::assign(m_data, pt);
}

template<size_t N, typename T>
VecNT<N, T>::operator QPointF() const
{
	return vecnt::SizeSpecific<N, T>::toQPointF(m_data);
}

template<size_t N, typename T>
template<typename OT>
VecNT<N, T>&
VecNT<N, T>::operator=(VecNT<N, OT> const& other)
{
	for (size_t i = 0; i < N; ++i) {
		m_data[i] = static_cast<T>(other[i]);
	}
	return *this;
}

template<size_t N, typename T>
VecNT<N, T>&
VecNT<N, T>::operator+=(T scalar)
{
	for (size_t i = 0; i < N; ++i) {
		m_data[i] += scalar;
	}
	return *this;
}

template<size_t N, typename T>
VecNT<N, T>&
VecNT<N, T>::operator+=(VecNT const& other)
{
	for (size_t i = 0; i < N; ++i) {
		m_data[i] += other[i];
	}
	return *this;
}

template<size_t N, typename T>
VecNT<N, T>&
VecNT<N, T>::operator-=(T scalar)
{
	for (size_t i = 0; i < N; ++i) {
		m_data[i] -= scalar;
	}
	return *this;
}

template<size_t N, typename T>
VecNT<N, T>&
VecNT<N, T>::operator-=(VecNT<N, T> const& other)
{
	for (size_t i = 0; i < N; ++i) {
		m_data[i] -= other[i];
	}
	return *this;
}

template<size_t N, typename T>
VecNT<N, T>&
VecNT<N, T>::operator*=(T scalar)
{
	for (size_t i = 0; i < N; ++i) {
		m_data[i] *= scalar;
	}
	return *this;
}

template<size_t N, typename T>
VecNT<N, T>&
VecNT<N, T>::operator/=(T scalar)
{
	return (*this *= (T(1) / scalar));
}

template<size_t N, typename T>
T
VecNT<N, T>::sum() const
{
	T sum = T();
	for (size_t i = 0; i < N; ++i) {
		sum += m_data[i];
	}
	return sum;
}

template<size_t N, typename T>
T
VecNT<N, T>::dot(VecNT const& other) const
{
	T sum = T();
	for (size_t i = 0; i < N; ++i) {
		sum += m_data[i] * other[i];
	}
	return sum;
}

template<size_t N, typename T>
VecNT<N, T> operator+(VecNT<N, T> const& lhs, VecNT<N, T> const& rhs)
{
	VecNT<N, T> res(lhs);
	res += rhs;
	return res;
}

template<size_t N, typename T>
VecNT<N, T> operator-(VecNT<N, T> const& lhs, VecNT<N, T> const& rhs)
{
	VecNT<N, T> res(lhs);
	res -= rhs;
	return res;
}

template<size_t N, typename T>
VecNT<N, T> operator-(VecNT<N, T> const& vec)
{
	VecNT<N, T> res(vec);
	for (size_t i = 0; i < N; ++i) {
		res[i] = -res[i];
	}
	return res;
}

template<size_t N, typename T>
VecNT<N, T> operator*(VecNT<N, T> const& vec, T scalar)
{
	VecNT<N, T> res(vec);
	res *= scalar;
	return res;
}

template<size_t N, typename T>
VecNT<N, T> operator*(T scalar, VecNT<N, T> const& vec)
{
	VecNT<N, T> res(vec);
	res *= scalar;
	return res;
}

#endif
