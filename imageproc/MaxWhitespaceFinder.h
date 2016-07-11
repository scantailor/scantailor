/*
    Scan Tailor - Interactive post-processing tool for scanned pages.
    Copyright (C) 2007-2009  Joseph Artsimovich <joseph_a@mail.ru>

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

#ifndef IMAGEPROC_MAX_WHITESPACE_FINDER_H_
#define IMAGEPROC_MAX_WHITESPACE_FINDER_H_

#include "NonCopyable.h"
#include "BinaryImage.h"
#include "IntegralImage.h"
#include <QRect>
#include <QSize>
#include <vector>
#include <deque>
#include <memory>
#include <stddef.h>

namespace imageproc
{

class BinaryImage;

namespace max_whitespace_finder
{
	class PriorityStorage;
}

/**
 * \brief Finds white rectangles in a binary image starting from the largest ones.
 */
class MaxWhitespaceFinder
{
	DECLARE_NON_COPYABLE(MaxWhitespaceFinder)
	friend class max_whitespace_finder::PriorityStorage;
public:
	/** \see next() */
	enum ObstacleMode { AUTO_OBSTACLES, MANUAL_OBSTACLES };
	
	/**
	 * \brief Constructor.
	 *
	 * \param img The image to find white regions in.
	 * \param min_size The minimum dimensions of regions to find.
	 */
	MaxWhitespaceFinder(
		BinaryImage const& img, QSize min_size = QSize(1, 1));
	
	/**
	 * \brief Constructor with customized rectangle ordering.
	 *
	 * \param comp A functor used to compare the "quality" of
	 *        rectangles.  It will be called like this:\n
	 * \code
	 * QRect lhs, rhs;
	 * if (comp(lhs, rhs)) {
	 *     // lhs is of less quality than rhs.
	 * }
	 * \endcode
	 * The comparison functor must comply with the following
	 * restriction:
	 * \code
	 * QRect rect, subrect;
	 * if (rect.contains(subrect)) {
	 *     assert(comp(rect, subrect) == false);
	 * }
	 * \endcode
	 * That is, if one rectangle contains or is equal to another,
	 * it can't have lesser quality.
	 *
	 * \param img The image to find white rectangles in.
	 * \param min_size The minimum dimensions of regions to find.
	 */
	template<typename QualityCompare>
	MaxWhitespaceFinder(
		QualityCompare comp,
		BinaryImage const& img, QSize min_size = QSize(1, 1));
	
	/**
	 * \brief Mark a region as black.
	 *
	 * This will prevent further rectangles from covering this region.
	 *
	 * \param rect The rectangle to mark as black.  It may exceed
	 *        the image area.
	 */
	void addObstacle(QRect const& rect);
	
	/**
	 * \brief Find the next white rectangle.
	 *
	 * \param obstacle_mode If set to AUTO_OBSTACLES, addObstacle()
	 *        will be called automatically to prevent further rectangles
	 *        from covering this region.  If set to MANUAL_OBSTACLES,
	 *        the caller is expected to call addObstacle() himself,
	 *        not necessarily with the same rectangle returned by next().
	 *        This mode allows finding partially overlapping rectangles
	 *        (by adding reduced obstacles).  There is no strict
	 *        requirement to manually add an obstacle after calling
	 *        this function with MANUAL_OBSTACLES.
	 * \param max_iterations The maximum number of iterations to spend
	 *        searching for the next maximal white rectangle.
	 *        Reaching this limit without finding one will cause
	 *        a null rectangle to be returned.  You generally don't
	 *        want to set this limit MAX_INT or similar, because
	 *        some patterns (a pixel by pixel checkboard pattern for example)
	 *        will take prohibitively long time to process.
	 * \return A white rectangle, or a null rectangle, if no white
	 *         rectangles confirming to the minimum size were found.
	 */
	QRect next(ObstacleMode obstacle_mode = AUTO_OBSTACLES, int max_iterations = 1000);
private:
	class Region
	{
	public:
		Region(unsigned known_new_obstacles, QRect const& bounds);
		
		/**
		 * A shallow copy.  Copies everything except the obstacle list.
		 */
		Region(Region const& other);
		
		QRect const& bounds() const { return m_bounds; }
		
		std::vector<QRect> const& obstacles() const {
			return m_obstacles;
		}
		
		void addObstacle(QRect const& obstacle) {
			m_obstacles.push_back(obstacle);
		}
		
		void addObstacles(Region const& other_region);
		
		void addNewObstacles(std::vector<QRect> const& new_obstacles);
		
		void swap(Region& other);
		
		void swapObstacles(Region& other) {
			m_obstacles.swap(other.m_obstacles);
		}
	private:
		Region& operator=(Region const&);
		
		unsigned m_knownNewObstacles;
		QRect m_bounds;
		std::vector<QRect> m_obstacles;
	};
	
	void init(BinaryImage const& img);
	
	void subdivideUsingObstacles(Region const& region);
	
	void subdivideUsingRaster(Region const& region);
	
	void subdivide(Region const& region, QRect bounds, QRect pivot);
	
	QRect findPivotObstacle(Region const& region) const;
	
	QPoint findBlackPixelCloseToCenter(QRect non_white_rect) const;
	
	QRect extendBlackPixelToBlackBox(QPoint pixel, QRect bounds) const;
	
	IntegralImage<unsigned> m_integralImg;
	std::auto_ptr<max_whitespace_finder::PriorityStorage> m_ptrQueuedRegions;
	std::vector<QRect> m_newObstacles;
	QSize m_minSize;
};


namespace max_whitespace_finder
{

class PriorityStorage
{
protected:
	typedef MaxWhitespaceFinder::Region Region;
public:
	virtual ~PriorityStorage() {}
	
	virtual bool empty() const = 0;
	
	virtual size_t size() const = 0;
	
	virtual Region& top() = 0;
	
	virtual void push(Region& region) = 0;
	
	virtual void pop() = 0;
};


template<typename QualityCompare>
class PriorityStorageImpl : public PriorityStorage
{
public:
	PriorityStorageImpl(QualityCompare comp) : m_qualityLess(comp) {}
	
	virtual bool empty() const { return m_priorityQueue.empty(); }
	
	virtual size_t size() const { return m_priorityQueue.size(); }
	
	virtual Region& top() { return m_priorityQueue.front(); }
	
	virtual void push(Region& region);
	
	virtual void pop();
private:
	class ProxyComparator
	{
	public:
		ProxyComparator(QualityCompare delegate) : m_delegate(delegate) {}
		
		bool operator()(Region const& lhs, Region const& rhs) const {
			return m_delegate(lhs.bounds(), rhs.bounds());
		}
	private:
		QualityCompare m_delegate;
	};
	
	void pushHeap(
		std::deque<Region>::iterator begin,
		std::deque<Region>::iterator end);
	
	void popHeap(
		std::deque<Region>::iterator begin,
		std::deque<Region>::iterator end);
	
	std::deque<Region> m_priorityQueue;
	ProxyComparator m_qualityLess;
};

template<typename QualityCompare>
void
PriorityStorageImpl<QualityCompare>::push(Region& region)
{
	m_priorityQueue.push_back(region);
	m_priorityQueue.back().swapObstacles(region);
	pushHeap(m_priorityQueue.begin(), m_priorityQueue.end());
}

template<typename QualityCompare>
void
PriorityStorageImpl<QualityCompare>::pop()
{
	popHeap(m_priorityQueue.begin(), m_priorityQueue.end());
	m_priorityQueue.pop_back();
}

/**
 * Same as std::push_heap(), except this one never copies objects, but swap()'s
 * them instead.  We need this to avoid copying the obstacle list over and over.
 */
template<typename QualityCompare>
void
PriorityStorageImpl<QualityCompare>::pushHeap(
	std::deque<Region>::iterator const begin,
	std::deque<Region>::iterator const end)
{
	typedef std::vector<Region>::iterator::difference_type Distance;
	
	Distance valueIdx = end - begin - 1;
	Distance parentIdx = (valueIdx - 1) / 2;
	
	// While the node is bigger than its parent, swap them.
	while (valueIdx > 0 &&
			m_qualityLess(*(begin + parentIdx), *(begin + valueIdx))) {
		(begin + valueIdx)->swap(*(begin + parentIdx));
		valueIdx = parentIdx;
		parentIdx = (valueIdx - 1) / 2;
	}
}

/**
 * Same as std::pop_heap(), except this one never copies objects, but swap()'s
 * them instead.  We need this to avoid copying the obstacle list over and over.
 */
template<typename QualityCompare>
void
PriorityStorageImpl<QualityCompare>::popHeap(
	std::deque<Region>::iterator const begin,
	std::deque<Region>::iterator const end)
{
	// Swap the first (top) and the last elements.
	begin->swap(*(end - 1));
	
	typedef std::vector<Region>::iterator::difference_type Distance;
	Distance const new_length = end - begin - 1;
	Distance nodeIdx = 0;
	Distance secondChildIdx = 2 * (nodeIdx + 1);
	
	// Lower the new top node all the way down the tree
	// by continuously swapping it with the biggest of its children.
	while (secondChildIdx < new_length) {
		Distance const firstChildIdx = secondChildIdx - 1;
		Distance biggestChildIdx = firstChildIdx;
		
		if (m_qualityLess(*(begin + firstChildIdx),
				*(begin + secondChildIdx))) {
			biggestChildIdx = secondChildIdx;
		}
		
		(begin + nodeIdx)->swap(*(begin + biggestChildIdx));
		
		nodeIdx = biggestChildIdx;
		secondChildIdx = 2 * (nodeIdx + 1);
	}
	if (secondChildIdx == new_length) {
		// Swap it with its only child.
		Distance const firstChildIdx = secondChildIdx - 1;
		(begin + nodeIdx)->swap(*(begin + firstChildIdx));
		nodeIdx = firstChildIdx;
	}
	
	// Now raise the node until it's at correct position.  Very little
	// raising should be necessary, that's why it's faster than adding
	// an additional comparison to the loop where we lower the node.
	pushHeap(begin, begin + nodeIdx + 1);
}

} // namespace max_whitespace_finder


template<typename QualityCompare>
MaxWhitespaceFinder::MaxWhitespaceFinder(
	QualityCompare const comp, BinaryImage const& img, QSize const min_size)
:	m_integralImg(img.size()),
	m_ptrQueuedRegions(
		new max_whitespace_finder::PriorityStorageImpl<QualityCompare>(comp)),
	m_minSize(min_size)
{
	init(img);
}

} // namespace imageproc

#endif
