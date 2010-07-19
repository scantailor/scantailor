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

#ifndef STATIC_POOL_H_
#define STATIC_POOL_H_

#include "NonCopyable.h"
#include <stdexcept>
#include <stddef.h>

template<typename T>
class StaticPoolBase
{
	DECLARE_NON_COPYABLE(StaticPoolBase)
public:
	StaticPoolBase(T* buf, size_t size) : m_pNext(buf), m_sizeRemaining(size) {}

	/**
	 * \brief Allocates a sequence of objects.
	 *
	 * If the pool has enough free space, returns a sequence of requested
	 * number of elements, otherwise throws an std::runtime_error.
	 * If T is a POD type, the returned objects are uninitialized,
	 * otherwise they are default-constructed.
	 *
	 * This function was moved to the base class in order to have
	 * just one instantiation of it for different sized pool of the same type.
	 */
	T* alloc(size_t num_elements);
private:
	T* m_pNext;
	size_t m_sizeRemaining;
};

/**
 * \brief Allocates objects from a statically sized pool.
 *
 * There is no way of releasing the allocated objects
 * besides destroying the whole pool.
 */
template<typename T, size_t S>
class StaticPool : public StaticPoolBase<T>
{
	DECLARE_NON_COPYABLE(StaticPool)
public:
	StaticPool() : StaticPoolBase<T>(m_buf, S) {}	
private:
	T m_buf[S];
};


template<typename T>
T*
StaticPoolBase<T>::alloc(size_t num_elements)
{
	if (num_elements > m_sizeRemaining) {
		throw std::runtime_error("StaticPool overflow");
	}

	T* sequence = m_pNext;
	m_pNext += num_elements;
	m_sizeRemaining -= num_elements;
	
	return sequence;
}

#endif
