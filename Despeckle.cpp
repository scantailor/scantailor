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

#include "Despeckle.h"
#include "TaskStatus.h"
#include "DebugImages.h"
#include "Dpi.h"
#include "FastQueue.h"
#include "imageproc/BinaryImage.h"
#include "imageproc/ConnectivityMap.h"
#include "imageproc/Connectivity.h"
#ifndef Q_MOC_RUN
#include <boost/foreach.hpp>
#endif
#include <QtGlobal>
#include <QImage>
#include <QDebug>
#include <vector>
#include <map>
#include <limits>
#include <algorithm>
#include <stddef.h>
#include <stdint.h>
#include <assert.h>

/**
 * \file
 * The idea of this despeckling algorithm is as follows:
 * \li The connected components that are larger than the specified threshold
 * are marked as non-garbage.
 * \li If a connected component is close enough to a non-garbage component
 * and their sizes are comparable or the non-garbage one is larger, then
 * the other one is also marked as non-garbage.
 *
 * The last step may be repeated until no new components are marked.
 * as non-garbage.
 */

using namespace imageproc;

namespace
{

/**
 * We treat vertical distances differently from the horizontal ones.
 * We want horizontal proximity to have greater weight, so we
 * multiply the vertical component distances by VERTICAL_SCALE,
 * so that the distance is not:\n
 * sqrt(dx^2 + dy^2)\n
 * but:\n
 * sqrt(dx^2 + (VERTICAL_SCALE*dy)^2)\n
 * Keep in mind that we actually operate on squared distances,
 * so we don't need to take that square root.
 */
static int const VERTICAL_SCALE = 2;
static int const VERTICAL_SCALE_SQ = VERTICAL_SCALE * VERTICAL_SCALE;

struct Settings
{
	/**
	 * When multiplied by the number of pixels in a connected component,
	 * gives the minimum size (in terms of the number of pixels) of a connected
	 * component we may attach it to.
	 */
	double minRelativeParentWeight;

	/**
	 * When multiplied by the number of pixels in a connected component,
	 * gives the maximum squared distance to another connected component
	 * we may attach it to.
	 */
	uint32_t pixelsToSqDist;

	/**
	 * Defines the minimum width or height in pixels that will guarantee
	 * the object won't be removed.
	 */
	int bigObjectThreshold;

	static Settings get(Despeckle::Level level, Dpi const& dpi);
};

Settings
Settings::get(Despeckle::Level const level, Dpi const& dpi)
{
	Settings settings;

	int const min_dpi = std::min(dpi.horizontal(), dpi.vertical());
	double const dpi_factor = min_dpi / 300.0;

	// To silence compiler's warnings.
	settings.minRelativeParentWeight = 0;
	settings.pixelsToSqDist = 0;
	settings.bigObjectThreshold = 0;

	switch (level) {
		case Despeckle::CAUTIOUS:
			settings.minRelativeParentWeight = 0.125 * dpi_factor;
			settings.pixelsToSqDist = 10.0*10.0;
			settings.bigObjectThreshold = qRound(7 * dpi_factor);
			break;
		case Despeckle::NORMAL:
			settings.minRelativeParentWeight = 0.175 * dpi_factor;
			settings.pixelsToSqDist = 6.5*6.5;
			settings.bigObjectThreshold = qRound(12 * dpi_factor);
			break;
		case Despeckle::AGGRESSIVE:
			settings.minRelativeParentWeight = 0.225 * dpi_factor;
			settings.pixelsToSqDist = 3.5*3.5;
			settings.bigObjectThreshold = qRound(17 * dpi_factor);
			break;
	}

	return settings;
}


struct Component
{
	static uint32_t const ANCHORED_TO_BIG = uint32_t(1) << 31;
	static uint32_t const ANCHORED_TO_SMALL = uint32_t(1) << 30;
	static uint32_t const TAG_MASK = ANCHORED_TO_BIG|ANCHORED_TO_SMALL;
	
	/**
	 * Lower 30 bits: the number of pixels in the connected component.
	 * Higher 2 bits: tags.
	 */
	uint32_t num_pixels;
	
	Component() : num_pixels(0) {}
	
	uint32_t const anchoredToBig() const {
		return num_pixels & ANCHORED_TO_BIG;
	}
	
	void setAnchoredToBig() {
		num_pixels |= ANCHORED_TO_BIG;
	}
	
	uint32_t const anchoredToSmall() const {
		return num_pixels & ANCHORED_TO_SMALL;
	}
	
	void setAnchoredToSmall() {
		num_pixels |= ANCHORED_TO_SMALL;
	}
	
	bool const anchoredToSmallButNotBig() const {
		return (num_pixels & TAG_MASK) == ANCHORED_TO_SMALL;
	}
	
	void clearTags() { num_pixels &= ~TAG_MASK; }
};

uint32_t const Component::ANCHORED_TO_BIG;
uint32_t const Component::ANCHORED_TO_SMALL;
uint32_t const Component::TAG_MASK;

struct BoundingBox
{
	int top;
	int left;
	int bottom;
	int right;

	BoundingBox() {
		top = left = std::numeric_limits<int>::max();
		bottom = right = std::numeric_limits<int>::min();
	}

	int width() const { return right - left + 1; }

	int height() const { return bottom - top + 1; }

	void extend(int x, int y) {
		top = std::min(top, y);
		left = std::min(left, x);
		bottom = std::max(bottom, y);
		right = std::max(right, x);
	}
};

struct Vector
{
	int16_t x;
	int16_t y;
};

union Distance
{
	Vector vec;
	uint32_t raw;
	
	static Distance zero() {
		Distance dist;
		dist.raw = 0;
		return dist;
	}
	
	static Distance special() {
		Distance dist;
		dist.vec.x = dist.vec.y = std::numeric_limits<int16_t>::max();
		return dist;
	}
	
	bool operator==(Distance const& other) const {
		return raw == other.raw;
	}
	
	bool operator!=(Distance const& other) const {
		return raw != other.raw;
	}
	
	void reset(int x) {
		vec.x = std::numeric_limits<int16_t>::max() - x;
		vec.y = 0;
	}
	
	uint32_t sqdist() const {
		int const x = vec.x;
		int const y = vec.y;
		return static_cast<uint32_t>(x * x + VERTICAL_SCALE_SQ * y * y);
	}
};

/**
 * \brief A bidirectional association between two connected components.
 */
struct Connection
{
	uint32_t lesser_label;
	uint32_t greater_label;
	
	Connection(uint32_t lbl1, uint32_t lbl2) {
		if (lbl1 < lbl2) {
			lesser_label = lbl1;
			greater_label = lbl2;
		} else {
			lesser_label = lbl2;
			greater_label = lbl1;
		}
	}
	
	bool operator<(Connection const& rhs) const {
		if (lesser_label < rhs.lesser_label) {
			return true;
		} else if (lesser_label > rhs.lesser_label) {
			return false;
		} else {
			return greater_label < rhs.greater_label;
		}
	}
};

/**
 * \brief A directional assiciation between two connected components.
 */
struct TargetSourceConn
{
	uint32_t target; /**< The label of the target connected component. */
	uint32_t source; /**< The label of the source connected component. */
	
	TargetSourceConn(uint32_t tgt, uint32_t src)
	: target(tgt), source(src) {}
	
	/**
	 * The ordering is by target then source.  It's designed to be able
	 * to quickly locate all associations involving a specific target.
	 */
	bool operator<(TargetSourceConn const& rhs) const {
		if (target < rhs.target) {
			return true;
		} else if (target > rhs.target) {
			return false;
		} else {
			return source < rhs.source;
		}
	}
};

/**
 * \brief If the association didn't exist, create it,
 *        otherwise the minimum distance.
 */
void updateDistance(
	std::map<Connection, uint32_t>& conns,
	uint32_t label1, uint32_t label2, uint32_t sqdist)
{
	typedef std::map<Connection, uint32_t> Connections;
	
	Connection const conn(label1, label2);
	Connections::iterator it(conns.lower_bound(conn));
	if (it == conns.end() || conn < it->first) {
		conns.insert(Connections::value_type(conn, sqdist));
	} else if (sqdist < it->second) {
		it->second = sqdist;
	}
}

/**
 * \brief Tag the source component with ANCHORED_TO_SMALL, ANCHORED_TO_BIG
 *        or none of the above.
 */
void tagSourceComponent(
	Component& source, Component const& target, uint32_t sqdist, Settings const& settings)
{
	if (source.anchoredToBig()) {
		// No point in setting ANCHORED_TO_SMALL.
		return;
	}
	
	if (sqdist > source.num_pixels * settings.pixelsToSqDist) {
		// Too far.
		return;
	}
	
	if (target.num_pixels >= settings.minRelativeParentWeight * source.num_pixels) {
		source.setAnchoredToBig();
	} else {
		source.setAnchoredToSmall();
	}
}

/**
 * Check if the component may be attached to another one.
 * Attaching a component to another one will preserve the component
 * being attached, provided that the one it's attached to is also preserved.
 */
bool canBeAttachedTo(
	Component const& comp, Component const& target, uint32_t sqdist, Settings const& settings)
{
	if (sqdist <= comp.num_pixels * settings.pixelsToSqDist) {
		if (target.num_pixels >= comp.num_pixels * settings.minRelativeParentWeight) {
			return true;
		}
	}
	return false;
}

void voronoi(ConnectivityMap& cmap, std::vector<Distance>& dist)
{
	int const width = cmap.size().width() + 2;
	int const height = cmap.size().height() + 2;
	
	assert(dist.empty());
	dist.resize(width * height, Distance::zero());
	
	std::vector<uint32_t> sqdists(width * 2, 0);
	uint32_t* prev_sqdist_line = &sqdists[0];
	uint32_t* this_sqdist_line = &sqdists[width];
	
	Distance* dist_line = &dist[0];
	uint32_t* cmap_line = cmap.paddedData();
	
	dist_line[0].reset(0);
	prev_sqdist_line[0] = dist_line[0].sqdist();
	for (int x = 1; x < width; ++x) {
		dist_line[x].vec.x = dist_line[x - 1].vec.x - 1;
		prev_sqdist_line[x] = prev_sqdist_line[x - 1]
				- (int(dist_line[x - 1].vec.x) << 1) + 1;
	}
	
	// Top to bottom scan.
	for (int y = 1; y < height; ++y) {
		dist_line += width;
		cmap_line += width;
		dist_line[0].reset(0);
		dist_line[width - 1].reset(width - 1);
		this_sqdist_line[0] = dist_line[0].sqdist();
		this_sqdist_line[width - 1] = dist_line[width - 1].sqdist();
		// Left to right scan.
		for (int x = 1; x < width - 1; ++x) {
			if (cmap_line[x]) {
				this_sqdist_line[x] = 0;
				assert(dist_line[x] == Distance::zero());
				continue;
			}
			
			// Propagate from left.
			Distance left_dist = dist_line[x - 1];
			uint32_t sqdist_left = this_sqdist_line[x - 1];
			sqdist_left += 1 - (int(left_dist.vec.x) << 1);
			
			// Propagate from top.
			Distance top_dist = dist_line[x - width];
			uint32_t sqdist_top = prev_sqdist_line[x];
			sqdist_top += VERTICAL_SCALE_SQ - 2 * VERTICAL_SCALE_SQ * int(top_dist.vec.y);
			
			if (sqdist_left < sqdist_top) {
				this_sqdist_line[x] = sqdist_left;
				--left_dist.vec.x;
				dist_line[x] = left_dist;
				cmap_line[x] = cmap_line[x - 1];
			} else {
				this_sqdist_line[x] = sqdist_top;
				--top_dist.vec.y;
				dist_line[x] = top_dist;
				cmap_line[x] = cmap_line[x - width];
			}
		}
		
		// Right to left scan.
		for (int x = width - 2; x >= 1; --x) {
			// Propagate from right.
			Distance right_dist = dist_line[x + 1];
			uint32_t sqdist_right = this_sqdist_line[x + 1];
			sqdist_right += 1 + (int(right_dist.vec.x) << 1);

			if (sqdist_right < this_sqdist_line[x]) {
				this_sqdist_line[x] = sqdist_right;
				++right_dist.vec.x;
				dist_line[x] = right_dist;
				cmap_line[x] = cmap_line[x + 1];
			}
		}
		
		std::swap(this_sqdist_line, prev_sqdist_line);
	}
	
	// Bottom to top scan.
	for (int y = height - 2; y >= 1; --y) {
		dist_line -= width;
		cmap_line -= width;
		dist_line[0].reset(0);
		dist_line[width - 1].reset(width - 1);
		this_sqdist_line[0] = dist_line[0].sqdist();
		this_sqdist_line[width - 1] = dist_line[width - 1].sqdist();
		// Right to left scan.
		for (int x = width - 2; x >= 1; --x) {
			// Propagate from right.
			Distance right_dist = dist_line[x + 1];
			uint32_t sqdist_right = this_sqdist_line[x + 1];
			sqdist_right += 1 + (int(right_dist.vec.x) << 1);
			
			// Propagate from bottom.
			Distance bottom_dist = dist_line[x + width];
			uint32_t sqdist_bottom = prev_sqdist_line[x];
			sqdist_bottom += VERTICAL_SCALE_SQ + 2 * VERTICAL_SCALE_SQ * int(bottom_dist.vec.y);

			this_sqdist_line[x] = dist_line[x].sqdist();
			
			if (sqdist_right < this_sqdist_line[x]) {
				this_sqdist_line[x] = sqdist_right;
				++right_dist.vec.x;
				dist_line[x] = right_dist;
				assert(cmap_line[x] == 0 || cmap_line[x + 1] != 0);
				cmap_line[x] = cmap_line[x + 1];
			}
			if (sqdist_bottom < this_sqdist_line[x]) {
				this_sqdist_line[x] = sqdist_bottom;
				++bottom_dist.vec.y;
				dist_line[x] = bottom_dist;
				assert(cmap_line[x] == 0 || cmap_line[x + width] != 0);
				cmap_line[x] = cmap_line[x + width];
			}
		}
		
		// Left to right scan.
		for (int x = 1; x < width - 1; ++x) {
			// Propagate from left.
			Distance left_dist = dist_line[x - 1];
			uint32_t sqdist_left = this_sqdist_line[x - 1];
			sqdist_left += 1 - (int(left_dist.vec.x) << 1);
			
			if (sqdist_left < this_sqdist_line[x]) {
				this_sqdist_line[x] = sqdist_left;
				--left_dist.vec.x;
				dist_line[x] = left_dist;
				assert(cmap_line[x] == 0 || cmap_line[x - 1] != 0);
				cmap_line[x] = cmap_line[x - 1];
			}
		}
		
		std::swap(this_sqdist_line, prev_sqdist_line);
	}
}

void voronoiSpecial(ConnectivityMap& cmap, std::vector<Distance>& dist, Distance const special_distance)
{
	int const width = cmap.size().width() + 2;
	int const height = cmap.size().height() + 2;
	
	std::vector<uint32_t> sqdists(width * 2, 0);
	uint32_t* prev_sqdist_line = &sqdists[0];
	uint32_t* this_sqdist_line = &sqdists[width];
	
	Distance* dist_line = &dist[0];
	uint32_t* cmap_line = cmap.paddedData();

	dist_line[0].reset(0);
	prev_sqdist_line[0] = dist_line[0].sqdist();
	for (int x = 1; x < width; ++x) {
		dist_line[x].vec.x = dist_line[x - 1].vec.x - 1;
		prev_sqdist_line[x] = prev_sqdist_line[x - 1]
				- (int(dist_line[x - 1].vec.x) << 1) + 1;
	}
	
	// Top to bottom scan.
	for (int y = 1; y < height - 1; ++y) {
		dist_line += width;
		cmap_line += width;
		dist_line[0].reset(0);
		dist_line[width - 1].reset(width - 1);
		this_sqdist_line[0] = dist_line[0].sqdist();
		this_sqdist_line[width - 1] = dist_line[width - 1].sqdist();
		// Left to right scan.
		for (int x = 1; x < width - 1; ++x) {
			if (dist_line[x] == special_distance) {
				continue;
			}
			
			this_sqdist_line[x] = dist_line[x].sqdist();
			
			// Propagate from left.
			Distance left_dist = dist_line[x - 1];
			if (left_dist != special_distance) {
				uint32_t sqdist_left = this_sqdist_line[x - 1];
				sqdist_left += 1 - (int(left_dist.vec.x) << 1);
				if (sqdist_left < this_sqdist_line[x]) {
					this_sqdist_line[x] = sqdist_left;
					--left_dist.vec.x;
					dist_line[x] = left_dist;
					assert(cmap_line[x] == 0 || cmap_line[x - 1] != 0);
					cmap_line[x] = cmap_line[x - 1];
				}
			}
			
			// Propagate from top.
			Distance top_dist = dist_line[x - width];
			if (top_dist != special_distance) {
				uint32_t sqdist_top = prev_sqdist_line[x];
				sqdist_top += VERTICAL_SCALE_SQ - 2 * VERTICAL_SCALE_SQ * int(top_dist.vec.y);
				if (sqdist_top < this_sqdist_line[x]) {
					this_sqdist_line[x] = sqdist_top;
					--top_dist.vec.y;
					dist_line[x] = top_dist;
					assert(cmap_line[x] == 0 || cmap_line[x - width] != 0);
					cmap_line[x] = cmap_line[x - width];
				}
			}
		}
		
		// Right to left scan.
		for (int x = width - 2; x >= 1; --x) {
			if (dist_line[x] == special_distance) {
				continue;
			}
			
			// Propagate from right.
			Distance right_dist = dist_line[x + 1];
			if (right_dist != special_distance) {
				uint32_t sqdist_right = this_sqdist_line[x + 1];
				sqdist_right += 1 + (int(right_dist.vec.x) << 1);
				if (sqdist_right < this_sqdist_line[x]) {
					this_sqdist_line[x] = sqdist_right;
					++right_dist.vec.x;
					dist_line[x] = right_dist;
					assert(cmap_line[x] == 0 || cmap_line[x + 1] != 0);
					cmap_line[x] = cmap_line[x + 1];
				}
			}
		}
		
		std::swap(this_sqdist_line, prev_sqdist_line);
	}
	
	// Bottom to top scan.
	for (int y = height - 2; y >= 1; --y) {
		dist_line -= width;
		cmap_line -= width;
		dist_line[0].reset(0);
		dist_line[width - 1].reset(width - 1);
		this_sqdist_line[0] = dist_line[0].sqdist();
		this_sqdist_line[width - 1] = dist_line[width - 1].sqdist();
		// Right to left scan.
		for (int x = width - 2; x >= 1; --x) {
			if (dist_line[x] == special_distance) {
				continue;
			}
			
			this_sqdist_line[x] = dist_line[x].sqdist();
			
			// Propagate from right.
			Distance right_dist = dist_line[x + 1];
			if (right_dist != special_distance) {
				uint32_t sqdist_right = this_sqdist_line[x + 1];
				sqdist_right += 1 + (int(right_dist.vec.x) << 1);
				if (sqdist_right < this_sqdist_line[x]) {
					this_sqdist_line[x] = sqdist_right;
					++right_dist.vec.x;
					dist_line[x] = right_dist;
					assert(cmap_line[x] == 0 || cmap_line[x + 1] != 0);
					cmap_line[x] = cmap_line[x + 1];
				}
			}
			
			// Propagate from bottom.
			Distance bottom_dist = dist_line[x + width];
			if (bottom_dist != special_distance) {
				uint32_t sqdist_bottom = prev_sqdist_line[x];
				sqdist_bottom += VERTICAL_SCALE_SQ + 2 * VERTICAL_SCALE_SQ * int(bottom_dist.vec.y);
				if (sqdist_bottom < this_sqdist_line[x]) {
					this_sqdist_line[x] = sqdist_bottom;
					++bottom_dist.vec.y;
					dist_line[x] = bottom_dist;
					assert(cmap_line[x] == 0 || cmap_line[x + width] != 0);
					cmap_line[x] = cmap_line[x + width];
				}
			}
		}
		
		// Left to right scan.
		for (int x = 1; x < width - 1; ++x) {
			if (dist_line[x] == special_distance) {
				continue;
			}
			
			// Propagate from left.
			Distance left_dist = dist_line[x - 1];
			if (left_dist != special_distance) {
				uint32_t sqdist_left = this_sqdist_line[x - 1];
				sqdist_left += 1 - (int(left_dist.vec.x) << 1);
				if (sqdist_left < this_sqdist_line[x]) {
					this_sqdist_line[x] = sqdist_left;
					--left_dist.vec.x;
					dist_line[x] = left_dist;
					assert(cmap_line[x] == 0 || cmap_line[x - 1] != 0);
					cmap_line[x] = cmap_line[x - 1];
				}
			}
		}
		
		std::swap(this_sqdist_line, prev_sqdist_line);
	}
}

/**
 * Calculate the minimum distance between components from neighboring
 * Voronoi segments.
 */
void voronoiDistances(
	ConnectivityMap const& cmap,
	std::vector<Distance> const& distance_matrix,
	std::map<Connection, uint32_t>& conns)
{
	int const width = cmap.size().width();
	int const height = cmap.size().height();
	
	int const offsets[] = { -cmap.stride(), -1, 1, cmap.stride() };
	
	uint32_t const* const cmap_data = cmap.data();
	Distance const* const distance_data = &distance_matrix[0] + width + 3;
	for (int y = 0, offset = 0; y < height; ++y, offset += 2) {
		for (int x = 0; x < width; ++x, ++offset) {
			uint32_t const label = cmap_data[offset];
			assert(label != 0);
			
			int const x1 = x + distance_data[offset].vec.x;
			int const y1 = y + distance_data[offset].vec.y;
			
			for (int i = 0; i < 4; ++i) {
				int const nbh_offset = offset + offsets[i];
				uint32_t const nbh_label = cmap_data[nbh_offset];
				if (nbh_label == 0 || nbh_label == label) {
					// label 0 can be encountered in
					// padding lines.
					continue;
				}
				
				int const x2 = x + distance_data[nbh_offset].vec.x;
				int const y2 = y + distance_data[nbh_offset].vec.y;
				int const dx = x1 - x2;
				int const dy = y1 - y2;
				uint32_t const sqdist = dx * dx + dy * dy;
				
				updateDistance(conns, label, nbh_label, sqdist);
			}
		}
	}
}

} // anonymous namespace


BinaryImage
Despeckle::despeckle(
	BinaryImage const& src, Dpi const& dpi, Level const level,
	TaskStatus const& status, DebugImages* const dbg)
{
	BinaryImage dst(src);
	despeckleInPlace(dst, dpi, level, status, dbg);
	return dst;
}

void
Despeckle::despeckleInPlace(
	BinaryImage& image, Dpi const& dpi, Level const level,
	TaskStatus const& status, DebugImages* const dbg)
{
	Settings const settings(Settings::get(level, dpi));

	ConnectivityMap cmap(image, CONN8);
	if (cmap.maxLabel() == 0) {
		// Completely white image?
		return;
	}

	status.throwIfCancelled();
	
	std::vector<Component> components(cmap.maxLabel() + 1);
	std::vector<BoundingBox> bounding_boxes(cmap.maxLabel() + 1);

	int const width = image.width();
	int const height = image.height();

	uint32_t* const cmap_data = cmap.data();
	
	// Count the number of pixels and a bounding rect of each component.
	uint32_t* cmap_line = cmap_data;
	int const cmap_stride = cmap.stride();
	for (int y = 0; y < height; ++y) {
		for (int x = 0; x < width; ++x) {
			uint32_t const label = cmap_line[x];
			++components[label].num_pixels;
			bounding_boxes[label].extend(x, y);
		}
		cmap_line += cmap_stride;
	}
	
	status.throwIfCancelled();

	// Unify big components into one.
	std::vector<uint32_t> remapping_table(components.size());
	uint32_t unified_big_component = 0;
	uint32_t next_avail_component = 1;
	for (uint32_t label = 1; label <= cmap.maxLabel(); ++label) {
		if (bounding_boxes[label].width() < settings.bigObjectThreshold &&
				bounding_boxes[label].height() < settings.bigObjectThreshold) {
			components[next_avail_component] = components[label];
			remapping_table[label] = next_avail_component;
			++next_avail_component;
		} else {
			if (unified_big_component == 0) {
				unified_big_component = next_avail_component;
				++next_avail_component;
				components[unified_big_component] = components[label];
				// Set num_pixels to a large value so that canBeAttachedTo()
				// always allows attaching to any such component.
				components[unified_big_component].num_pixels = width * height;
			}
			remapping_table[label] = unified_big_component;
		}
	}
	components.resize(next_avail_component);
	std::vector<BoundingBox>().swap(bounding_boxes); // We don't need them any more.
	
	status.throwIfCancelled();

	uint32_t const max_label = next_avail_component - 1;
	
	// Remapping individual pixels.
	cmap_line = cmap_data;
	for (int y = 0; y < height; ++y) {
		for (int x = 0; x < width; ++x) {
			cmap_line[x] = remapping_table[cmap_line[x]];
		}
		cmap_line += cmap_stride;
	}
	if (dbg) {
		dbg->add(cmap.visualized(), "big_components_unified");
	}

	status.throwIfCancelled();
	
	// Build a Voronoi diagram.
	std::vector<Distance> distance_matrix;
	voronoi(cmap, distance_matrix);
	if (dbg) {
		dbg->add(cmap.visualized(), "voronoi");
	}
	
	status.throwIfCancelled();

	Distance* const distance_data = &distance_matrix[0] + width + 3;
	
	// Now build a bidirectional map of distances between neighboring
	// connected components.
	
	typedef std::map<Connection, uint32_t> Connections; // conn -> sqdist
	Connections conns;
	
	voronoiDistances(cmap, distance_matrix, conns);
	
	status.throwIfCancelled();

	// Tag connected components with ANCHORED_TO_BIG or ANCHORED_TO_SMALL.
	BOOST_FOREACH(Connections::value_type const& pair, conns) {
		Connection const conn(pair.first);
		uint32_t const sqdist = pair.second;
		Component& comp1 = components[conn.lesser_label];
		Component& comp2 = components[conn.greater_label];
		tagSourceComponent(comp1, comp2, sqdist, settings);
		tagSourceComponent(comp2, comp1, sqdist, settings);
	}
	
	// Prevent it from growing when we compute the Voronoi diagram
	// the second time.
	components[unified_big_component].setAnchoredToBig();
	
	bool have_anchored_to_small_but_not_big = false;
	BOOST_FOREACH(Component const& comp, components) {
		have_anchored_to_small_but_not_big = comp.anchoredToSmallButNotBig();
	}
	
	if (have_anchored_to_small_but_not_big) {
		
		status.throwIfCancelled();
		
		// Give such components a second chance.  Maybe they do have
		// big neighbors, but Voronoi regions from a smaller ones
		// block the path to the bigger ones.
		
		Distance const zero_distance(Distance::zero());
		Distance const special_distance(Distance::special());
		for (int y = 0, offset = 0; y < height; ++y, offset += 2) {
			for (int x = 0; x < width; ++x, ++offset) {
				uint32_t const label = cmap_data[offset];
				assert(label != 0);
				
				Component const& comp = components[label];
				if (!comp.anchoredToSmallButNotBig()) {
					if (distance_data[offset] == zero_distance) {
						// Prevent this region from growing
						// and from being taken over by another
						// by another region.
						distance_data[offset] = special_distance;
					} else {
						// Allow this region to be taken over by others.
						// Note: x + 1 here is equivalent to x
						// in voronoi() or voronoiSpecial().
						distance_data[offset].reset(x + 1);
					}
				}
			}
		}
		
		status.throwIfCancelled();

		// Calculate the Voronoi diagram again, but this time
		// treat pixels with a special distance in such a way
		// to prevent them from spreading but also preventing
		// them from being overwritten.
		voronoiSpecial(cmap, distance_matrix, special_distance);
		if (dbg) {
			dbg->add(cmap.visualized(), "voronoi_special");
		}
		
		status.throwIfCancelled();

		// We've got new connections.  Add them to the map.
		voronoiDistances(cmap, distance_matrix, conns);
	}
	
	status.throwIfCancelled();

	// Clear the distance matrix.
	std::vector<Distance>().swap(distance_matrix);
	
	// Remove tags from components.
	BOOST_FOREACH(Component& comp, components) {
		comp.clearTags();
	}
	
	// Build a directional connection map and only include
	// good connections, that is those with a small enough
	// distance.
	// While at it, clear the bidirectional connection map.
	std::vector<TargetSourceConn> target_source;
	while (!conns.empty()) {
		Connections::iterator const it(conns.begin());
		uint32_t const label1 = it->first.lesser_label;
		uint32_t const label2 = it->first.greater_label;
		uint32_t const sqdist = it->second;
		Component const& comp1 = components[label1];
		Component const& comp2 = components[label2];
		if (canBeAttachedTo(comp1, comp2, sqdist, settings)) {
			target_source.push_back(TargetSourceConn(label2, label1));
		}
		if (canBeAttachedTo(comp2, comp1, sqdist, settings)) {
			target_source.push_back(TargetSourceConn(label1, label2));
		}
		conns.erase(it);
	}

	std::sort(target_source.begin(), target_source.end());
	
	status.throwIfCancelled();

	// Create an index for quick access to a group of connections
	// with a specified target.
	std::vector<size_t> target_source_idx;
	size_t const num_target_sources = target_source.size();
	uint32_t prev_label = uint32_t(0) - 1;
	for (size_t i = 0; i < num_target_sources; ++i) {
		TargetSourceConn const& conn = target_source[i];
		assert(conn.target != 0);
		for (; prev_label != conn.target; ++prev_label) {
			target_source_idx.push_back(i);
		}
		assert(target_source_idx.size() - 1 == conn.target);
	}
	for (uint32_t label = target_source_idx.size();
			label <= max_label; ++label) {
		target_source_idx.push_back(num_target_sources);
	}
	
	// Labels of components that are to be retained.
	FastQueue<uint32_t> ok_labels;
	ok_labels.push(unified_big_component);
	
	while (!ok_labels.empty()) {
		uint32_t const label = ok_labels.front();
		ok_labels.pop();
		
		Component& comp = components[label];
		if (comp.anchoredToBig()) {
			continue;
		}
		
		comp.setAnchoredToBig();
		
		size_t idx = target_source_idx[label];
		while (idx < num_target_sources &&
				target_source[idx].target == label) {
			ok_labels.push(target_source[idx].source);
			++idx;
		}
	}
	
	status.throwIfCancelled();

	// Remove unmarked components from the binary image.
	uint32_t const msb = uint32_t(1) << 31;
	uint32_t* image_line = image.data();
	int const image_stride = image.wordsPerLine();
	cmap_line = cmap_data;
	for (int y = 0; y < height; ++y) {
		for (int x = 0; x < width; ++x) {
			if (!components[cmap_line[x]].anchoredToBig()) {
				image_line[x >> 5] &= ~(msb >> (x & 31));
			}
		}
		image_line += image_stride;
		cmap_line += cmap_stride;
	}
}
