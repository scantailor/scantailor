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

#ifndef INTRUSIVEPTR_H_
#define INTRUSIVEPTR_H_

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

template<typename T>
class IntrusivePtr
{
private:
	struct BooleanTestHelper
	{
		int dataMember;
	};
	typedef int BooleanTestHelper::*BooleanTest;
public:
	IntrusivePtr() : m_pObj(0) {}
	
	explicit
	IntrusivePtr(T* obj);
	
	IntrusivePtr(IntrusivePtr const& other);
	
	template<typename OT>
	IntrusivePtr(IntrusivePtr<OT> const& other);
	
	~IntrusivePtr();
	
	IntrusivePtr& operator=(IntrusivePtr const& rhs);
	
	template<typename OT>
	IntrusivePtr& operator=(IntrusivePtr<OT> const& rhs);
	
	T& operator*() const { return *m_pObj; }
	
	T* operator->() const { return m_pObj; }
	
	T* get() const { return m_pObj; }
	
	void reset(T* obj = 0);
	
	void swap(IntrusivePtr& other);

	/**
	 * Used for boolean tests, like:
	 * \code
	 * IntrusivePtr<T> ptr = ...;
	 * if (ptr) {
	 *   ...
	 * }
	 * if (!ptr) {
	 *   ...
	 * }
	 * \endcode
	 * This implementation insures that the following expressions fail to compile:
	 * \code
	 * IntrusivePtr<T> ptr = ...;
	 * int i = ptr;
	 * delete ptr;
	 * \endcode
	 */
	inline operator BooleanTest() const;
private:
	T* m_pObj;
};


/**
 * \brief Default implementation of intrusive referencing.
 *
 * May be specialized or overloaded.
 */
template<typename T>
inline void intrusive_ref(T& obj)
{
	obj.ref();
}


/**
 * \brief Default implementation of intrusive unreferencing.
 *
 * May be specialized or overloaded.
 */
template<typename T>
inline void intrusive_unref(T& obj)
{
	obj.unref();
}


template<typename T>
inline
IntrusivePtr<T>::IntrusivePtr(T* obj)
:	m_pObj(obj)
{
	if (obj)
		intrusive_ref(*obj);
}

template<typename T>
inline
IntrusivePtr<T>::IntrusivePtr(IntrusivePtr const& other)
:	m_pObj(other.m_pObj)
{
	if (m_pObj)
		intrusive_ref(*m_pObj);
}

template<typename T>
template<typename OT>
inline
IntrusivePtr<T>::IntrusivePtr(IntrusivePtr<OT> const& other)
:	m_pObj(other.get())
{
	if (m_pObj)
		intrusive_ref(*m_pObj);
}

template<typename T>
inline
IntrusivePtr<T>::~IntrusivePtr()
{
	if (m_pObj)
		intrusive_unref(*m_pObj);
}

template<typename T>
inline IntrusivePtr<T>&
IntrusivePtr<T>::operator=(IntrusivePtr const& rhs)
{
	IntrusivePtr(rhs).swap(*this);
	return *this;
}

template<typename T>
template<typename OT>
inline IntrusivePtr<T>&
IntrusivePtr<T>::operator=(IntrusivePtr<OT> const& rhs)
{
	IntrusivePtr(rhs).swap(*this);
	return *this;
}

template<typename T>
inline void
IntrusivePtr<T>::reset(T* obj)
{
	IntrusivePtr(obj).swap(*this);
}

template<typename T>
inline void
IntrusivePtr<T>::swap(IntrusivePtr& other)
{
	T* obj = other.m_pObj;
	other.m_pObj = m_pObj;
	m_pObj = obj;
}

template<typename T>
inline void swap(IntrusivePtr<T>& o1, IntrusivePtr<T>& o2)
{
	o1.swap(o2);
}

template<typename T>
IntrusivePtr<T>::operator BooleanTest() const
{
	return m_pObj ? &BooleanTestHelper::dataMember : 0;
}

#define INTRUSIVE_PTR_OP(op) \
template<typename T> \
inline bool operator op(IntrusivePtr<T> const& lhs, IntrusivePtr<T> const& rhs) \
{ \
	return lhs.get() op rhs.get(); \
}

INTRUSIVE_PTR_OP(==)
INTRUSIVE_PTR_OP(!=)
INTRUSIVE_PTR_OP(<)
INTRUSIVE_PTR_OP(>)
INTRUSIVE_PTR_OP(<=)
INTRUSIVE_PTR_OP(>=)

#endif
