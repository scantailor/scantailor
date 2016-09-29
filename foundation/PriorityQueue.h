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

#ifndef PRIORITY_QUEUE_H_
#define PRIORITY_QUEUE_H_

#include <vector>
#include <algorithm>
#include <stddef.h>
#include <assert.h>

/**
 * \brief A priority queue implemented as a binary heap.
 *
 * \tparam T The type of objects to be stored in the priority queue.
 * \param SubClass A sub class of this class that will be implementing the following:
 *        \li void setIndex(T& obj, size_t heap_idx);
 *        \li bool higherThan(T const& lhs, T const& rhs) const;
 *
 * Also note that this implementation will benefit from a standalone
 * \code
 * void swap(T& o1, T& o2);
 * \endcode
 * function in the same namespace as T.
 */
template<typename T, typename SubClass>
class PriorityQueue
{
	// Member-wise copying is OK.
public:
	PriorityQueue() {}

	void reserve(size_t capacity) { m_index.reserve(capacity); }

	bool empty() const { return m_index.empty(); }

	size_t size() const { return m_index.size(); }

	/**
	 * \brief Provides access to the head of priority queue.
	 *
	 * Modification of an object is allowed, provided your modifications don't
	 * affect the logical order of objects, or you will be calling reposition(),
	 * pop() or erase() on the modified object before any other operation that
	 * involves comparing objects.
	 */
	T& front() { return m_index.front(); }

	T const& front() const { return m_index.front(); }

	void push(T const& obj);

	/**
	 * Like push(), but implemented through swapping \p obj with a default
	 * constructed instance of T. This will make sense if copying a default
	 * constructed instance of T is much cheaper than copying \p obj.
	 */
	void pushDestructive(T& obj);

	void pop();

	/**
	 * Retrieve-and-pop, implemented through swapping \p obj with the instance
	 * at the front of the queue. There are no special requirements to
	 * the state of the object being passed to this function.
	 */
	void retrieveFront(T& obj);

	void swapWith(PriorityQueue& other) { m_index.swap(other.m_index); }
protected:
	void erase(size_t idx);

	void reposition(size_t idx);
private:
	static size_t parent(size_t idx) { return (idx - 1) / 2; }

	static size_t left(size_t idx) { return idx * 2 + 1; }

	static size_t right(size_t idx) { return idx * 2 + 2; }

	SubClass* subClass() { return static_cast<SubClass*>(this); }

	SubClass const* subClass() const { return static_cast<SubClass const*>(this); }

	size_t bubbleUp(size_t idx);

	size_t bubbleDown(size_t idx);

	std::vector<T> m_index;
};


template<typename T, typename SubClass>
inline void swap(PriorityQueue<T, SubClass>& o1, PriorityQueue<T, SubClass>& o2)
{
	o1.swap(o2);
}

template<typename T, typename SubClass>
void
PriorityQueue<T, SubClass>::push(T const& obj)
{
	size_t const idx = m_index.size();
	m_index.push_back(obj);
	subClass()->setIndex(m_index.back(), idx);
	bubbleUp(idx);
}

template<typename T, typename SubClass>
void
PriorityQueue<T, SubClass>::pushDestructive(T& obj)
{
	using namespace std;

	size_t const idx = m_index.size();
	m_index.push_back(T());
	swap(m_index.back(), obj);
	subClass()->setIndex(m_index.back(), idx);
	bubbleUp(idx);
}

template<typename T, typename SubClass>
void
PriorityQueue<T, SubClass>::pop()
{
	using namespace std;

	assert(!empty());

	swap(m_index.front(), m_index.back());
	subClass()->setIndex(m_index.front(), 0);

	m_index.pop_back();
	if (!empty()) {
		bubbleDown(0);
	}
}

template<typename T, typename SubClass>
void
PriorityQueue<T, SubClass>::retrieveFront(T& obj)
{
	using namespace std;

	assert(!empty());

	swap(m_index.front(), obj);
	swap(m_index.front(), m_index.back());
	subClass()->setIndex(m_index.front(), 0);

	m_index.pop_back();
	if (!empty()) {
		bubbleDown(0);
	}
}

template<typename T, typename SubClass>
void
PriorityQueue<T, SubClass>::erase(size_t const idx)
{
	using namespace std;

	swap(m_index[idx], m_index.back());
	subClass()->setIndex(m_index[idx], idx);

	m_index.pop_back();
	reposition(m_index[idx]);
}

template<typename T, typename SubClass>
void
PriorityQueue<T, SubClass>::reposition(size_t const idx)
{
	bubbleUp(bubbleDown(idx));
}

template<typename T, typename SubClass>
size_t
PriorityQueue<T, SubClass>::bubbleUp(size_t idx)
{
	using namespace std;

	// Iteratively swap the element with its parent,
	// if it's greater than the parent.

	assert(idx < m_index.size());

	while (idx > 0) {
		size_t const parent_idx = parent(idx);
		if (!subClass()->higherThan(m_index[idx], m_index[parent_idx])) {
			break;
		}
		swap(m_index[idx], m_index[parent_idx]);
		subClass()->setIndex(m_index[idx], idx);
		subClass()->setIndex(m_index[parent_idx], parent_idx);
		idx = parent_idx;
	}

	return idx;
}

template<typename T, typename SubClass>
size_t
PriorityQueue<T, SubClass>::bubbleDown(size_t idx)
{
	using namespace std;

	size_t const len = m_index.size();
	assert(idx < len);

	// While any child is greater than the element itself,
	// swap it with the greatest child.

	for (;;) {
		size_t const lft = left(idx);
		size_t const rgt = right(idx);
		size_t best_child;

		if(rgt < len) {
			best_child = subClass()->higherThan(m_index[lft], m_index[rgt]) ? lft : rgt;
		} else if (lft < len) {
			best_child = lft;
		} else {
			break;
		}
        
		if (subClass()->higherThan(m_index[best_child], m_index[idx])) {
			swap(m_index[idx], m_index[best_child]);
			subClass()->setIndex(m_index[idx], idx);
			subClass()->setIndex(m_index[best_child], best_child);
            idx = best_child;
		} else {
			break;
		}
    }

	return idx;
}

#endif
