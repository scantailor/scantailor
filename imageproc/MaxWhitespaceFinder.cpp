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

#include "MaxWhitespaceFinder.h"
#include <QPoint>
#include <QDebug>
#ifndef Q_MOC_RUN
#include <boost/foreach.hpp>
#endif
#include <algorithm>
#include <limits>
#include <stddef.h>
#include <stdint.h>
#include <assert.h>

namespace imageproc
{

using namespace max_whitespace_finder;

namespace
{

class AreaCompare
{
public:
	bool operator()(QRect const lhs, QRect const rhs) const {
		int const lhs_area = lhs.width() * lhs.height();
		int const rhs_area = rhs.width() * rhs.height();
		return lhs_area < rhs_area;
	}
};

} // anonymous namespace


MaxWhitespaceFinder::MaxWhitespaceFinder(BinaryImage const& img, QSize min_size)
:	m_integralImg(img.size()),
	m_ptrQueuedRegions(new PriorityStorageImpl<AreaCompare>(AreaCompare())),
	m_minSize(min_size)
{
	init(img);
}

void
MaxWhitespaceFinder::init(BinaryImage const& img)
{
	int const width = img.width();
	int const height = img.height();
	uint32_t const* line = img.data();
	int const wpl = img.wordsPerLine();
	
	for (int y = 0; y < height; ++y, line += wpl) {
		m_integralImg.beginRow();
		for (int x = 0; x < width; ++x) {
			int const shift = 31 - (x & 31);
			m_integralImg.push((line[x >> 5] >> shift) & 1);
		}
	}
	
	Region region(0, img.rect());
	m_ptrQueuedRegions->push(region);
}

void
MaxWhitespaceFinder::addObstacle(QRect const& obstacle)
{
	if (m_ptrQueuedRegions->size() == 1) {
		m_ptrQueuedRegions->top().addObstacle(obstacle);
	} else {
		m_newObstacles.push_back(obstacle);
	}
}

QRect
MaxWhitespaceFinder::next(ObstacleMode const obstacle_mode, int max_iterations)
{
	while (max_iterations-- > 0 && !m_ptrQueuedRegions->empty()) {
		Region& top_region = m_ptrQueuedRegions->top();
		Region region(top_region);
		region.swapObstacles(top_region);
		m_ptrQueuedRegions->pop();
		
		region.addNewObstacles(m_newObstacles);
		
		if (!region.obstacles().empty()) {
			subdivideUsingObstacles(region);
			continue;
		}
		
		if (m_integralImg.sum(region.bounds()) != 0) {
			subdivideUsingRaster(region);
			continue;
		}
		
		if (obstacle_mode == AUTO_OBSTACLES) {
			m_newObstacles.push_back(region.bounds());
		}
		
		return region.bounds();
	}
	
	return QRect();
}

void
MaxWhitespaceFinder::subdivideUsingObstacles(Region const& region)
{
	QRect const bounds(region.bounds());
	QRect const pivot_rect(findPivotObstacle(region));
	
	subdivide(region, bounds, pivot_rect);
}

void
MaxWhitespaceFinder::subdivideUsingRaster(Region const& region)
{
	QRect const bounds(region.bounds());
	QPoint const pivot_pixel(findBlackPixelCloseToCenter(bounds));
	QRect const pivot_rect(extendBlackPixelToBlackBox(pivot_pixel, bounds));
	
	subdivide(region, bounds, pivot_rect);
}

void
MaxWhitespaceFinder::subdivide(
	Region const& region, QRect const bounds, QRect const pivot)
{
	// Area above the pivot obstacle.
	if (pivot.top() - bounds.top() >= m_minSize.height()) {
		QRect new_bounds(bounds);
		new_bounds.setBottom(pivot.top() - 1); // Bottom is inclusive.
		Region new_region(m_newObstacles.size(), new_bounds);
		new_region.addObstacles(region);
		m_ptrQueuedRegions->push(new_region);
	}
	
	// Area below the pivot obstacle.
	if (bounds.bottom() - pivot.bottom() >= m_minSize.height()) {
		QRect new_bounds(bounds);
		new_bounds.setTop(pivot.bottom() + 1);
		Region new_region(m_newObstacles.size(), new_bounds);
		new_region.addObstacles(region);
		m_ptrQueuedRegions->push(new_region);
	}
	
	// Area to the left of the pivot obstacle.
	if (pivot.left() - bounds.left() >= m_minSize.width()) {
		QRect new_bounds(bounds);
		new_bounds.setRight(pivot.left() - 1); // Right is inclusive.
		Region new_region(m_newObstacles.size(), new_bounds);
		new_region.addObstacles(region);
		m_ptrQueuedRegions->push(new_region);
	}
	
	// Area to the right of the pivot obstacle.
	if (bounds.right() - pivot.right() >= m_minSize.width()) {
		QRect new_bounds(bounds);
		new_bounds.setLeft(pivot.right() + 1);
		Region new_region(m_newObstacles.size(), new_bounds);
		new_region.addObstacles(region);
		m_ptrQueuedRegions->push(new_region);
	}
}

QRect
MaxWhitespaceFinder::findPivotObstacle(Region const& region) const
{
	assert(!region.obstacles().empty());
	
	QPoint const center(region.bounds().center());
	
	QRect best_obstacle;
	int best_distance = std::numeric_limits<int>::max();
	BOOST_FOREACH (QRect const& obstacle, region.obstacles()) {
		QPoint const vec(center - obstacle.center());
		int const distance = vec.x() * vec.x() + vec.y() * vec.y();
		if (distance <= best_distance) {
			best_obstacle = obstacle;
			best_distance = distance;
		}
	}
	
	return best_obstacle;
}

QPoint
MaxWhitespaceFinder::findBlackPixelCloseToCenter(
	QRect const non_white_rect) const
{
	assert(m_integralImg.sum(non_white_rect) != 0);
	
	QPoint const center(non_white_rect.center());
	QRect outer_rect(non_white_rect);
	QRect inner_rect(center.x(), center.y(), 1, 1);
	
	if (m_integralImg.sum(inner_rect) != 0) {
		return center;
	}
	
	// We have two rectangles: the outer one, that always contains at least
	// one black pixel, and the inner one (contained within the outer one),
	// that doesn't contain any black pixels.
	
	// The first thing we do is bringing those two rectangles as close
	// as possible to each other, so that no more than 1 pixel separates
	// their corresponding edges.
	
	for (;;) {
		int const outer_inner_dw = outer_rect.width() - inner_rect.width();
		int const outer_inner_dh = outer_rect.height() - inner_rect.height();
		
		if (outer_inner_dw <= 1 && outer_inner_dh <= 1) {
			break;
		}
		
		int const delta_left = inner_rect.left() - outer_rect.left();
		int const delta_right = outer_rect.right() - inner_rect.right();
		int const delta_top = inner_rect.top() - outer_rect.top();
		int const delta_bottom = outer_rect.bottom() - inner_rect.bottom();
		
		QRect middle_rect(
			outer_rect.left() + ((delta_left + 1) >> 1),
			outer_rect.top() + ((delta_top + 1) >> 1),
			0, 0
		);
		middle_rect.setRight(outer_rect.right() - (delta_right >> 1));
		middle_rect.setBottom(outer_rect.bottom() - (delta_bottom >> 1));
		assert(outer_rect.contains(middle_rect));
		assert(middle_rect.contains(inner_rect));
		
		if (m_integralImg.sum(middle_rect) == 0) {
			inner_rect = middle_rect;
		} else {
			outer_rect = middle_rect;
		}
	}
	
	// Process the left edge.
	if (outer_rect.left() != inner_rect.left()) {
		QRect rect(outer_rect);
		rect.setRight(rect.left()); // Right is inclusive.
		unsigned const sum = m_integralImg.sum(rect);
		if (outer_rect.height() == 1) {
			// This means we are dealing with a horizontal line
			// and that we only have to check at most two pixels
			// (the endpoints) and that at least one of them
			// is definitely black and that rect is a 1x1 pixels
			// block pointing to the left endpoint.
			if (sum != 0) {
				return outer_rect.topLeft();
			} else {
				return outer_rect.topRight();
			}
		} else if (sum != 0) {
			return findBlackPixelCloseToCenter(rect);
		}
	}
	
	// Process the right edge.
	if (outer_rect.right() != inner_rect.right()) {
		QRect rect(outer_rect);
		rect.setLeft(rect.right()); // Right is inclusive.
		unsigned const sum = m_integralImg.sum(rect);
		if (outer_rect.height() == 1) {
			// Same as above, except rect now points to the
			// right endpoint.
			if (sum != 0) {
				return outer_rect.topRight();
			} else {
				return outer_rect.topLeft();
			}
		} else if (sum != 0) {
			return findBlackPixelCloseToCenter(rect);
		}
	}
	
	// Process the top edge.
	if (outer_rect.top() != inner_rect.top()) {
		QRect rect(outer_rect);
		rect.setBottom(rect.top()); // Bottom is inclusive.
		unsigned const sum = m_integralImg.sum(rect);
		if (outer_rect.width() == 1) {
			// Same as above, except rect now points to the
			// top endpoint.
			if (sum != 0) {
				return outer_rect.topLeft();
			} else {
				return outer_rect.bottomLeft();
			}
		} else if (sum != 0) {
			return findBlackPixelCloseToCenter(rect);
		}
	}
	
	// Process the bottom edge.
	assert(outer_rect.bottom() != inner_rect.bottom());
	QRect rect(outer_rect);
	rect.setTop(rect.bottom()); // Bottom is inclusive.
	assert(m_integralImg.sum(rect) != 0);
	if (outer_rect.width() == 1) {
		return outer_rect.bottomLeft();
	} else {
		return findBlackPixelCloseToCenter(rect);
	}
}

QRect
MaxWhitespaceFinder::extendBlackPixelToBlackBox(
	QPoint const pixel, QRect const bounds) const
{
	assert(bounds.contains(pixel));
	
	QRect outer_rect(bounds);
	QRect inner_rect(pixel.x(), pixel.y(), 1, 1);
	
	if (m_integralImg.sum(outer_rect) ==
			unsigned(outer_rect.width() * outer_rect.height())) {
		return outer_rect;
	}
	
	// We have two rectangles: the outer one, that always contains at least
	// one white pixel, and the inner one (contained within the outer one),
	// that doesn't.
	
	// We will be bringing those two rectangles as close as possible to
	// each other, so that no more than 1 pixel separates their
	// corresponding edges.
	
	for (;;) {
		int const outer_inner_dw = outer_rect.width() - inner_rect.width();
		int const outer_inner_dh = outer_rect.height() - inner_rect.height();
		
		if (outer_inner_dw <= 1 && outer_inner_dh <= 1) {
			break;
		}
		
		int const delta_left = inner_rect.left() - outer_rect.left();
		int const delta_right = outer_rect.right() - inner_rect.right();
		int const delta_top = inner_rect.top() - outer_rect.top();
		int const delta_bottom = outer_rect.bottom() - inner_rect.bottom();
		
		QRect middle_rect(
			outer_rect.left() + ((delta_left + 1) >> 1),
			outer_rect.top() + ((delta_top + 1) >> 1),
			0, 0
		);
		middle_rect.setRight(outer_rect.right() - (delta_right >> 1));
		middle_rect.setBottom(outer_rect.bottom() - (delta_bottom >> 1));
		assert(outer_rect.contains(middle_rect));
		assert(middle_rect.contains(inner_rect));
		
		unsigned const area = middle_rect.width() * middle_rect.height();
		if (m_integralImg.sum(middle_rect) == area) {
			inner_rect = middle_rect;
		} else {
			outer_rect = middle_rect;
		}
	}
	
	return inner_rect;
}


/*======================= MaxWhitespaceFinder::Region =====================*/

MaxWhitespaceFinder::Region::Region(
	unsigned known_new_obstacles, QRect const& bounds)
:	m_knownNewObstacles(known_new_obstacles),
	m_bounds(bounds)
{
}

MaxWhitespaceFinder::Region::Region(Region const& other)
:	m_knownNewObstacles(other.m_knownNewObstacles),
	m_bounds(other.m_bounds)
{
	// Note that we don't copy m_obstacles.  This is a shallow copy.
}

/**
 * Adds obstacles from another region that intersect this region's area.
 */
void
MaxWhitespaceFinder::Region::addObstacles(Region const& other_region)
{
	BOOST_FOREACH (QRect const& obstacle, other_region.obstacles()) {
		QRect const intersected(obstacle.intersected(m_bounds));
		if (!intersected.isEmpty()) {
			m_obstacles.push_back(intersected);
		}
	}
}

/**
 * Adds global obstacles that were not there when this region was constructed.
 */
void
MaxWhitespaceFinder::Region::addNewObstacles(
	std::vector<QRect> const& new_obstacles)
{
	for (size_t i = m_knownNewObstacles; i < new_obstacles.size(); ++i) {
		QRect const intersected(new_obstacles[i].intersected(m_bounds));
		if (!intersected.isEmpty()) {
			m_obstacles.push_back(intersected);
		}
	}
}

/**
 * A fast and non-throwing swap operation.
 */
void
MaxWhitespaceFinder::Region::swap(Region& other)
{
	std::swap(m_bounds, other.m_bounds);
	std::swap(m_knownNewObstacles, other.m_knownNewObstacles);
	m_obstacles.swap(other.m_obstacles);
}

} // namespace imageproc
