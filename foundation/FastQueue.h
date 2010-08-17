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

#ifndef FAST_QUEUE_H_
#define FAST_QUEUE_H_

#include "NonCopyable.h"
#include <boost/intrusive/list.hpp>
#include <boost/type_traits/alignment_of.hpp>
#include <boost/foreach.hpp>
#include <stddef.h>
#include <stdint.h>
#include <assert.h>

template<typename T>
class FastQueue
{
public:
	FastQueue() : m_chunkCapacity(defaultChunkCapacity()) {}

	FastQueue(FastQueue const& other);

	~FastQueue() { m_chunkList.clear_and_dispose(ChunkDisposer()); }

	FastQueue& operator=(FastQueue const& other);

	bool const empty() const { return m_chunkList.empty(); }

	T& front() { return *m_chunkList.front().pBegin; }

	T const& front() const { return *m_chunkList.front().pBegin; }

	void push(T const& t);

	void pop();

	void swap(FastQueue& other);
private:
	struct Chunk : public boost::intrusive::list_base_hook<>
	{
		DECLARE_NON_COPYABLE(Chunk)
	public:
		Chunk(size_t capacity) {
			uintptr_t const p = (uintptr_t)(this + 1);
			size_t const alignment = boost::alignment_of<T>::value;
			pBegin = (T*)(((p + alignment - 1) / alignment) * alignment);
			pEnd = pBegin;
			pBufferEnd = pBegin + capacity;
			assert(size_t((char*)pBufferEnd - (char*)this) <= storageRequirement(capacity));
		}

		~Chunk() {
			for (; pBegin != pEnd; ++pBegin) {
				pBegin->~T();
			}
		}

		static size_t storageRequirement(size_t capacity) {
			return sizeof(Chunk) + boost::alignment_of<T>::value - 1 + capacity * sizeof(T);
		}
		
		T* pBegin;
		T* pEnd;
		T* pBufferEnd;
		// An implicit array of T follows.
	};

	struct ChunkDisposer
	{
		void operator()(Chunk* chunk) {
			chunk->~Chunk();
			delete[] (char*)chunk;
		}
	};

	typedef boost::intrusive::list<
		Chunk, boost::intrusive::constant_time_size<false>
	> ChunkList;

	static size_t defaultChunkCapacity() {
		return (sizeof(T) >= 4096) ? 1 : 4096 / sizeof(T);
	}

	ChunkList m_chunkList;
	size_t m_chunkCapacity;
};


template<typename T>
FastQueue<T>::FastQueue(FastQueue const& other)
:	m_chunkCapacity(other.m_chunkCapacity)
{
	BOOST_FOREACH(Chunk& chunk, other.m_chunkList) {
		for (T const* obj = chunk->pBegin; obj != chunk->pEnd; ++obj) {
			push(*obj);
		}
	}
}

template<typename T>
FastQueue<T>&
FastQueue<T>::operator=(FastQueue const& other)
{
	FastQueue(other).swap(*this);
	return *this;
}

template<typename T>
void
FastQueue<T>::push(T const& t)
{	
	Chunk* chunk = 0;

	if (!m_chunkList.empty()) {
		chunk = &m_chunkList.back();
		if (chunk->pEnd == chunk->pBufferEnd) {
			chunk = 0;
		}
	}

	if (!chunk) {
		// Create a new chunk.
		char* buf = new char[Chunk::storageRequirement(m_chunkCapacity)];
		chunk = new(buf) Chunk(m_chunkCapacity);
		m_chunkList.push_back(*chunk);
	}

	// Push to chunk.
	new(chunk->pEnd) T(t);
	++chunk->pEnd;
}

template<typename T>
void
FastQueue<T>::pop()
{
	assert(!empty());

	Chunk* chunk = &m_chunkList.front();
	chunk->pBegin->~T();
	++chunk->pBegin;
	if (chunk->pBegin == chunk->pEnd) {
		m_chunkList.pop_front();
		ChunkDisposer()(chunk);
	}
}

template<typename T>
void
FastQueue<T>::swap(FastQueue& other)
{
	m_chunkList.swap(other.m_chunkList);
	size_t const tmp = m_chunkCapacity;
	m_chunkCapacity = other.m_chunkCapacity;
	other.m_chunkCapacity = tmp;
}

template<typename T>
inline void swap(FastQueue<T>& o1, FastQueue<T>& o2)
{
	o1.swap(o2);
}

#endif
