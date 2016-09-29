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

#include "TopBottomEdgeTracer.h"
#include "DistortionModelBuilder.h"
#include "TaskStatus.h"
#include "DebugImages.h"
#include "NumericTraits.h"
#include "PriorityQueue.h"
#include "ToLineProjector.h"
#include "LineBoundedByRect.h"
#include "GridLineTraverser.h"
#include "MatrixCalc.h"
#include "imageproc/GrayImage.h"
#include "imageproc/Scale.h"
#include "imageproc/Constants.h"
#include "imageproc/GaussBlur.h"
#include <QPoint>
#include <QSize>
#include <QRect>
#include <QTransform>
#include <QPainter>
#include <QImage>
#include <QColor>
#include <QtGlobal>
#include <QDebug>
#ifndef Q_MOC_RUN
#include <boost/foreach.hpp>
#include <boost/lambda/lambda.hpp>
#include <boost/lambda/bind.hpp>
#endif
#include <limits>
#include <algorithm>
#include <math.h>
#include <stddef.h>
#include <assert.h>

using namespace imageproc;

namespace dewarping
{

struct TopBottomEdgeTracer::GridNode
{
private:
	static uint32_t const HEAP_IDX_BITS = 28;
	static uint32_t const PREV_NEIGHBOUR_BITS = 3;
	static uint32_t const PATH_CONTINUATION_BITS = 1;

	static uint32_t const HEAP_IDX_SHIFT = 0;
	static uint32_t const PREV_NEIGHBOUR_SHIFT = HEAP_IDX_SHIFT + HEAP_IDX_BITS;
	static uint32_t const PATH_CONTINUATION_SHIFT = PREV_NEIGHBOUR_SHIFT + PREV_NEIGHBOUR_BITS;

	static uint32_t const HEAP_IDX_MASK = ((uint32_t(1) << HEAP_IDX_BITS) - uint32_t(1)) << HEAP_IDX_SHIFT;
	static uint32_t const PREV_NEIGHBOUR_MASK = ((uint32_t(1) << PREV_NEIGHBOUR_BITS) - uint32_t(1)) << PREV_NEIGHBOUR_SHIFT;
	static uint32_t const PATH_CONTINUATION_MASK = ((uint32_t(1) << PATH_CONTINUATION_BITS) - uint32_t(1)) << PATH_CONTINUATION_SHIFT;
public:
	static uint32_t const INVALID_HEAP_IDX = HEAP_IDX_MASK >> HEAP_IDX_SHIFT;

	union {
		float dirDeriv; // Directional derivative.
		float xGrad; // x component of the gradient.
	};
	union {
		float pathCost;
		float blurred;
		float yGrad; // y component of the gradient.
	};
	// Note: xGrad and yGrad are used to calculate the directional
	// derivative, which then gets stored in dirDeriv.  Obviously,
	// pathCost gets overwritten, which is not a problem in our case.

	uint32_t packedData;

	float absDirDeriv() const { return fabs(dirDeriv); }

	void setupForPadding() {
		dirDeriv = 0;
		pathCost = -1;
		packedData = INVALID_HEAP_IDX;
	}

	/**
	 * Note that is one doesn't modify dirDeriv.
	 */
	void setupForInterior() {
		pathCost = NumericTraits<float>::max();
		packedData = INVALID_HEAP_IDX;
	}

	uint32_t heapIdx() const {
		return (packedData & HEAP_IDX_MASK) >> HEAP_IDX_SHIFT;
	}

	void setHeapIdx(uint32_t idx) {
		assert(!(idx & ~(HEAP_IDX_MASK >> HEAP_IDX_SHIFT)));
		packedData = idx | (packedData & ~HEAP_IDX_MASK);
	}

	bool hasPathContinuation() const {
		return packedData & PATH_CONTINUATION_MASK;
	}

	/**
	 * Neibhgours are indexed like this:
	 * 0 1 2
	 * 3   4
	 * 5 6 7
	 */
	uint32_t prevNeighbourIdx() const {
		return (packedData & PREV_NEIGHBOUR_MASK) >> PREV_NEIGHBOUR_SHIFT;
	}

	void setPrevNeighbourIdx(uint32_t idx) {
		assert(!(idx & ~(PREV_NEIGHBOUR_MASK >> PREV_NEIGHBOUR_SHIFT)));
		packedData = PATH_CONTINUATION_MASK | (idx << PREV_NEIGHBOUR_SHIFT) | (packedData & ~PREV_NEIGHBOUR_MASK);
	}

	void setBothGradients(float grad) {
		xGrad = grad;
		yGrad = grad;
	}
};


class TopBottomEdgeTracer::PrioQueue :
	public PriorityQueue<uint32_t, PrioQueue>
{
public:
	PrioQueue(Grid<GridNode>& grid) : m_pData(grid.data()) {}

	bool higherThan(uint32_t lhs, uint32_t rhs) const {
		return m_pData[lhs].pathCost < m_pData[rhs].pathCost;
	}

	void setIndex(uint32_t grid_idx, size_t heap_idx) {
		m_pData[grid_idx].setHeapIdx(static_cast<uint32_t>(heap_idx));
	}

	void reposition(GridNode* node) {
		PriorityQueue<uint32_t, PrioQueue>::reposition(node->heapIdx());
	}
private:
	GridNode* const m_pData;
};


struct TopBottomEdgeTracer::Step
{
	Vec2f pt;
	uint32_t prevStepIdx;
	float pathCost;
};


template<typename Extractor>
float
TopBottomEdgeTracer::interpolatedGridValue(Grid<GridNode> const& grid, Extractor extractor, Vec2f const pos, float default_value)
{
	float const x_base = floor(pos[0]);
	float const y_base = floor(pos[1]);
	int const x_base_i = (int)x_base;
	int const y_base_i = (int)y_base;

	if (x_base_i < 0 || y_base_i < 0 || x_base_i + 1 >= grid.width() || y_base_i + 1 >= grid.height()) {
		return default_value;
	}

	float const x = pos[0] - x_base;
	float const y = pos[1] - y_base;
	float const x1 = 1.0f - x;
	float const y1 = 1.0f - y;

	int const stride = grid.stride();
	GridNode const* base = grid.data() + y_base_i * stride + x_base_i;

	return extractor(base[0])*x1*y1 + extractor(base[1])*x*y1 +
		extractor(base[stride])*x1*y + extractor(base[stride + 1])*x*y;
}


void
TopBottomEdgeTracer::trace(
	imageproc::GrayImage const& image, std::pair<QLineF, QLineF> bounds,
	DistortionModelBuilder& output, TaskStatus const& status, DebugImages* dbg)
{
	if (bounds.first.p1() == bounds.first.p2() || bounds.second.p1() == bounds.second.p2()) {
		return; // Bad bounds.
	}

	GrayImage downscaled;
	QSize downscaled_size(image.size());
	QTransform downscaling_xform;
	
	if (std::max(image.width(), image.height()) < 1500) {
		// Don't downscale - it's already small.
		downscaled = image;
	} else {
		// Proceed with downscaling.
		downscaled_size.scale(1000, 1000, Qt::KeepAspectRatio);
		downscaling_xform.scale(
			double(downscaled_size.width()) / image.width(),
			double(downscaled_size.height()) / image.height()
		);
		downscaled = scaleToGray(image, downscaled_size);
		if (dbg) {
			dbg->add(downscaled, "downscaled");
		}

		status.throwIfCancelled();

		bounds.first = downscaling_xform.map(bounds.first);
		bounds.second = downscaling_xform.map(bounds.second);
	}

	// Those -1's are to make sure the endpoints, rounded to integers,
	// will be within the image.
	if (!intersectWithRect(bounds, QRectF(downscaled.rect()).adjusted(0, 0, -1, -1))) {
		return;
	}

	forceSameDirection(bounds);
	
	Vec2f const avg_bounds_dir(calcAvgUnitVector(bounds));
	Grid<GridNode> grid(downscaled.width(), downscaled.height(), /*padding=*/1);
	calcDirectionalDerivative(grid, downscaled, avg_bounds_dir);
	if (dbg) {
		dbg->add(visualizeGradient(grid), "gradient");
	}

	status.throwIfCancelled();

	PrioQueue queue(grid);
	
	// Shortest paths from bounds.first towards bounds.second.
	prepareForShortestPathsFrom(queue, grid, bounds.first);
	Vec2f const dir_1st_to_2nd(directionFromPointToLine(bounds.first.pointAt(0.5), bounds.second));
	propagateShortestPaths(dir_1st_to_2nd, queue, grid);
	std::vector<QPoint> const endpoints1(locateBestPathEndpoints(grid, bounds.second));
	if (dbg) {
		dbg->add(visualizePaths(downscaled, grid, bounds, endpoints1), "best_paths_ltr");
	}

	gaussBlurGradient(grid);

	std::vector<std::vector<QPointF> > snakes;
	snakes.reserve(endpoints1.size());
	
	BOOST_FOREACH(QPoint endpoint, endpoints1) {
		snakes.push_back(pathToSnake(grid, endpoint));
		Vec2f const dir(downTheHillDirection(downscaled.rect(), snakes.back(), avg_bounds_dir));
		downTheHillSnake(snakes.back(), grid, dir);
	}
	if (dbg) {
		QImage const background(visualizeBlurredGradient(grid));
		dbg->add(visualizeSnakes(background, snakes, bounds), "down_the_hill_snakes");
	}

	BOOST_FOREACH(std::vector<QPointF>& snake, snakes) {
		Vec2f const dir(-downTheHillDirection(downscaled.rect(), snake, avg_bounds_dir));
		upTheHillSnake(snake, grid, dir);
	}
	if (dbg) {
		QImage const background(visualizeGradient(grid));
		dbg->add(visualizeSnakes(background, snakes, bounds), "up_the_hill_snakes");
	}

	// Convert snakes back to the original coordinate system.
	QTransform const upscaling_xform(downscaling_xform.inverted());
	BOOST_FOREACH(std::vector<QPointF>& snake, snakes) {
		BOOST_FOREACH(QPointF& pt, snake) {
			pt = upscaling_xform.map(pt);
		}
		output.addHorizontalCurve(snake);
	}
}

bool
TopBottomEdgeTracer::intersectWithRect(
	std::pair<QLineF, QLineF>& bounds, QRectF const& rect)
{
	return lineBoundedByRect(bounds.first, rect) && lineBoundedByRect(bounds.second, rect);
}

void
TopBottomEdgeTracer::forceSameDirection(std::pair<QLineF, QLineF>& bounds)
{
	QPointF const v1(bounds.first.p2() - bounds.first.p1());
	QPointF const v2(bounds.second.p2() - bounds.second.p1());
	if (v1.x() * v2.x() + v1.y() * v2.y() < 0) {
		bounds.second.setPoints(bounds.second.p2(), bounds.second.p1());
	}
}

void
TopBottomEdgeTracer::calcDirectionalDerivative(
	Grid<GridNode>& grid, imageproc::GrayImage const& image, Vec2f const& direction)
{
	assert(grid.padding() == 1);

	int const width = grid.width();
	int const height = grid.height();

	int const grid_stride = grid.stride();
	int const image_stride = image.stride();

	uint8_t const* image_line = image.data();
	GridNode* grid_line = grid.data();

	// This ensures that partial derivatives never go beyond the [-1, 1] range.
	float const scale = 1.0f / (255.0f * 8.0f);

	// We are going to use both GridNode::gradient and GridNode::pathCost
	// to calculate the gradient.

	// Copy image to gradient.
	for (int y = 0; y < height; ++y) {
		for (int x = 0; x < width; ++x) {
			grid_line[x].setBothGradients(scale * image_line[x]);
		}
		image_line += image_stride;
		grid_line += grid_stride;
	}

	// Write border corners.
	grid_line = grid.paddedData();
	grid_line[0].setBothGradients(grid_line[grid_stride + 1].xGrad);
	grid_line[grid_stride - 1].setBothGradients(grid_line[grid_stride * 2 - 2].xGrad);
	grid_line += grid_stride * (height + 1);
	grid_line[0].setBothGradients(grid_line[1 - grid_stride].xGrad);
	grid_line[grid_stride - 1].setBothGradients(grid_line[-2].xGrad);

	// Top border line.
	grid_line = grid.paddedData() + 1;
	for (int x = 0; x < width; ++x) {
		grid_line[0].setBothGradients(grid_line[grid_stride].xGrad);
		++grid_line;
	}

	// Bottom border line.
	grid_line = grid.paddedData() + grid_stride * (height + 1) + 1;
	for (int x = 0; x < width; ++x) {
		grid_line[0].setBothGradients(grid_line[-grid_stride].xGrad);
		++grid_line;
	}

	// Left and right border lines.
	grid_line = grid.paddedData() + grid_stride;
	for (int y = 0; y < height; ++y) {
		grid_line[0].setBothGradients(grid_line[1].xGrad);
		grid_line[grid_stride - 1].setBothGradients(grid_line[grid_stride - 2].xGrad);
		grid_line += grid_stride;
	}
	
	horizontalSobelInPlace(grid);
	verticalSobelInPlace(grid);

	// From horizontal and vertical gradients, calculate the directional one.
	grid_line = grid.data();
	for (int y = 0; y < height; ++y) {
		for (int x = 0; x < width; ++x) {
			Vec2f const grad_vec(grid_line[x].xGrad, grid_line[x].yGrad);
			grid_line[x].dirDeriv = grad_vec.dot(direction);
			assert(fabs(grid_line[x].dirDeriv) <= 1.0);
		}

		grid_line += grid_stride;
	}
}

void
TopBottomEdgeTracer::horizontalSobelInPlace(Grid<GridNode>& grid)
{
	assert(grid.padding() == 1);

	int const width = grid.width();
	int const height = grid.height();
	int const grid_stride = grid.stride();

	// Do a vertical pass.
	for (int x = -1; x < width + 1; ++x) {
		GridNode* p_grid = grid.data() + x;
		float prev = p_grid[-grid_stride].xGrad;
		for (int y = 0; y < height; ++y) {
			float const cur = p_grid->xGrad;
			p_grid->xGrad = prev + cur + cur + p_grid[grid_stride].xGrad;
			prev = cur;
			p_grid += grid_stride;
		}
	}

	// Do a horizontal pass and write results.
	GridNode* grid_line = grid.data();
	for (int y = 0; y < height; ++y) {
		float prev = grid_line[-1].xGrad;
		for (int x = 0; x < width; ++x) {
			float cur = grid_line[x].xGrad;
			grid_line[x].xGrad = grid_line[x + 1].xGrad - prev;
			prev = cur;
		}
		grid_line += grid_stride;
	}
}

void
TopBottomEdgeTracer::verticalSobelInPlace(Grid<GridNode>& grid)
{
	assert(grid.padding() == 1);

	int const width = grid.width();
	int const height = grid.height();
	int const grid_stride = grid.stride();

	// Do a horizontal pass.
	GridNode* grid_line = grid.paddedData() + 1;
	for (int y = 0; y < height + 2; ++y) {
		float prev = grid_line[-1].yGrad;
		for (int x = 0; x < width; ++x) {
			float cur = grid_line[x].yGrad;
			grid_line[x].yGrad = prev + cur + cur + grid_line[x + 1].yGrad;
			prev = cur;
		}
		grid_line += grid_stride;
	}

	// Do a vertical pass and write resuts.
	for (int x = 0; x < width; ++x) {
		GridNode* p_grid = grid.data() + x;
		float prev = p_grid[-grid_stride].yGrad;
		for (int y = 0; y < height; ++y) {
			float const cur = p_grid->yGrad;
			p_grid->yGrad = p_grid[grid_stride].yGrad - prev;
			prev = cur;
			p_grid += grid_stride;
		}
	}
}

Vec2f
TopBottomEdgeTracer::calcAvgUnitVector(std::pair<QLineF, QLineF> const& bounds)
{
	Vec2f v1(bounds.first.p2() - bounds.first.p1());
	v1 /= sqrt(v1.squaredNorm());

	Vec2f v2(bounds.second.p2() - bounds.second.p1());
	v2 /= sqrt(v2.squaredNorm());

	Vec2f v3(v1 + v2);
	v3 /= sqrt(v3.squaredNorm());
	
	return v3;
}

Vec2f
TopBottomEdgeTracer::directionFromPointToLine(QPointF const& pt, QLineF const& line)
{
	Vec2f vec(ToLineProjector(line).projectionVector(pt));
	float const sqlen = vec.squaredNorm();
	if (sqlen > 1e-5) {
		vec /= sqrt(sqlen);
	}
	return vec;
}

void
TopBottomEdgeTracer::prepareForShortestPathsFrom(
	PrioQueue& queue, Grid<GridNode>& grid, QLineF const& from)
{
	GridNode padding_node;
	padding_node.setupForPadding();
	grid.initPadding(padding_node);

	int const width = grid.width();
	int const height = grid.height();
	int const stride = grid.stride();
	GridNode* const data = grid.data();

	GridNode* line = grid.data();
	for (int y = 0; y < height; ++y) {
		for (int x = 0; x < width; ++x) {
			GridNode* node = line + x;
			node->setupForInterior();
			// This doesn't modify dirDeriv, which is why
			// we can't use grid.initInterior().
		}
		line += stride;
	}

	GridLineTraverser traverser(from);
	while (traverser.hasNext()) {
		QPoint const pt(traverser.next());

		// intersectWithRect() ensures that.
		assert(pt.x() >= 0 && pt.y() >= 0 && pt.x() < width && pt.y() < height);

		int const offset = pt.y() * stride + pt.x();
		data[offset].pathCost = 0;
		queue.push(offset);
	}
}

void
TopBottomEdgeTracer::propagateShortestPaths(
	Vec2f const& direction, PrioQueue& queue, Grid<GridNode>& grid)
{
	GridNode* const data = grid.data();
	
	int next_nbh_offsets[8];
	int prev_nbh_indexes[8];
	int const num_neighbours = initNeighbours(next_nbh_offsets, prev_nbh_indexes, grid.stride(), direction);

	while (!queue.empty()) {
		int const grid_idx = queue.front();
		GridNode* node = data + grid_idx;
		assert(node->pathCost >= 0);
		queue.pop();
		node->setHeapIdx(GridNode::INVALID_HEAP_IDX);

		for (int i = 0; i < num_neighbours; ++i) {
			int const nbh_grid_idx = grid_idx + next_nbh_offsets[i];
			GridNode* nbh_node = data + nbh_grid_idx;
			
			assert(fabs(node->dirDeriv) <= 1.0);
			float const new_cost = std::max<float>(node->pathCost, 1.0f - fabs(node->dirDeriv));
			if (new_cost < nbh_node->pathCost) {
				nbh_node->pathCost = new_cost;
				nbh_node->setPrevNeighbourIdx(prev_nbh_indexes[i]);
				if (nbh_node->heapIdx() == GridNode::INVALID_HEAP_IDX) {
					queue.push(nbh_grid_idx);
				} else {
					queue.reposition(nbh_node);
				}
			}
		}
	}
}

int
TopBottomEdgeTracer::initNeighbours(
	int* next_nbh_offsets, int* prev_nbh_indexes, int stride, Vec2f const& direction)
{
	int const candidate_offsets[] = {
		-stride - 1, -stride, -stride + 1,
		         -1,                    1,
		 stride - 1,  stride,  stride + 1
	};

	float const candidate_vectors[8][2] = {
		{ -1.0f, -1.0f }, { 0.0f, -1.0f }, { 1.0f, -1.0f },
		{ -1.0f,  0.0f },                  { 1.0f,  0.0f },
		{ -1.0f,  1.0f }, { 0.0f,  1.0f }, { 1.0f,  1.0f }
	};

	static int const opposite_nbh_map[] = {
		7, 6, 5,
		4,    3,
		2, 1, 0
	};

	int out_idx = 0;
	for (int i = 0; i < 8; ++i) {
		Vec2f const vec(candidate_vectors[i][0], candidate_vectors[i][1]);
		if (vec.dot(direction) > 0) {
			next_nbh_offsets[out_idx] = candidate_offsets[i];
			prev_nbh_indexes[out_idx] = opposite_nbh_map[i];
			++out_idx;
		}
	}
	return out_idx;
}

namespace
{

struct Path
{
	QPoint pt;
	float cost;

	Path(QPoint pt, float cost) : pt(pt), cost(cost) {}
};

}

std::vector<QPoint>
TopBottomEdgeTracer::locateBestPathEndpoints(Grid<GridNode> const& grid, QLineF const& line)
{
	int const width = grid.width();
	int const height = grid.height();
	int const stride = grid.stride();
	GridNode const* const data = grid.data();

	size_t const num_best_paths = 2; // Take N best paths.
	int const min_sqdist = 100*100;
	std::vector<Path> best_paths;

	GridLineTraverser traverser(line);
	while (traverser.hasNext()) {
		QPoint const pt(traverser.next());

		// intersectWithRect() ensures that.
		assert(pt.x() >= 0 && pt.y() >= 0 && pt.x() < width && pt.y() < height);

		uint32_t const offset = pt.y() * stride + pt.x();
		GridNode const* node = data + offset;
		
		// Find the closest path.
		Path* closest_path = 0;
		int closest_sqdist = std::numeric_limits<int>::max();
		BOOST_FOREACH(Path& path, best_paths) {
			QPoint const delta(path.pt - pt);
			int const sqdist = delta.x() * delta.x() + delta.y() * delta.y();
			if (sqdist < closest_sqdist) {
				closest_path = &path;
				closest_sqdist = sqdist;
			}
		}

		if (closest_sqdist < min_sqdist) {
			// That's too close.
			if (node->pathCost < closest_path->cost) {
				closest_path->pt = pt;
				closest_path->cost = node->pathCost;
			}
			continue;
		}

		if (best_paths.size() < num_best_paths) {
			best_paths.push_back(Path(pt, node->pathCost));
		} else {
			// Find the one to kick out (if any).
			BOOST_FOREACH(Path& path, best_paths) {
				if (node->pathCost < path.cost) {
					path = Path(pt, node->pathCost);
					break;
				}
			}
		}
	}

	std::vector<QPoint> best_endpoints;

	BOOST_FOREACH(Path const& path, best_paths) {
		if (path.cost < 0.95f) {
			best_endpoints.push_back(path.pt);
		}
	}

	return best_endpoints;
}

std::vector<QPoint>
TopBottomEdgeTracer::tracePathFromEndpoint(Grid<GridNode> const& grid, QPoint const& endpoint)
{
	static int const dx[8] = {
		-1, 0, 1,
		-1,    1,
		-1, 0, 1
	};
	static int const dy[8] = {
		-1, -1, -1,
		 0,      0,
		 1,  1,  1
	};

	int const stride = grid.stride();
	int const grid_offsets[8] = {
		-stride - 1, -stride, -stride + 1,
		        - 1,                  + 1,
		+stride - 1, +stride, +stride + 1
	};

	GridNode const* const data = grid.data();
	std::vector<QPoint> path;

	QPoint pt(endpoint);
	int grid_offset = pt.x() + pt.y() * stride;
	for (;;) {
		path.push_back(pt);

		GridNode const* node = data + grid_offset;
		if (!node->hasPathContinuation()) {
			break;
		}

		int const nbh_idx = node->prevNeighbourIdx();
		grid_offset += grid_offsets[nbh_idx];
		pt += QPoint(dx[nbh_idx], dy[nbh_idx]);
	}

	return path;
}

std::vector<QPointF>
TopBottomEdgeTracer::pathToSnake(Grid<GridNode> const& grid, QPoint const& endpoint)
{
	int const max_dist = 15; // Maximum distance between two snake knots.
	int const max_dist_sq = max_dist * max_dist;
	int const half_max_dist = max_dist / 2;
	int const half_max_dist_sq = half_max_dist * half_max_dist;

	static int const dx[8] = {
		-1, 0, 1,
		-1,    1,
		-1, 0, 1
	};
	static int const dy[8] = {
		-1, -1, -1,
		 0,      0,
		 1,  1,  1
	};

	int const stride = grid.stride();
	int const grid_offsets[8] = {
		-stride - 1, -stride, -stride + 1,
		        - 1,                  + 1,
		+stride - 1, +stride, +stride + 1
	};

	GridNode const* const data = grid.data();
	std::vector<QPointF> snake;
	snake.push_back(endpoint);
	QPoint snake_tail(endpoint);

	QPoint pt(endpoint);
	int grid_offset = pt.x() + pt.y() * stride;
	for (;;) {
		QPoint const delta(pt - snake_tail);
		int const sqdist = delta.x() * delta.x() + delta.y() * delta.y();

		GridNode const* node = data + grid_offset;
		if (!node->hasPathContinuation()) {
			if (sqdist >= half_max_dist_sq) {
				snake.push_back(pt);
				snake_tail = pt;
			}
			break;
		}

		if (sqdist >= max_dist_sq) {
			snake.push_back(pt);
			snake_tail = pt;
		}

		int const nbh_idx = node->prevNeighbourIdx();
		grid_offset += grid_offsets[nbh_idx];
		pt += QPoint(dx[nbh_idx], dy[nbh_idx]);
	}

	return snake;
}

void
TopBottomEdgeTracer::gaussBlurGradient(Grid<GridNode>& grid)
{
	using namespace boost::lambda;

	gaussBlurGeneric(
		QSize(grid.width(), grid.height()), 2.0f, 2.0f,
		grid.data(), grid.stride(), bind(&GridNode::absDirDeriv, _1),
		grid.data(), grid.stride(), bind(&GridNode::blurred, _1) = _2
	);
}

Vec2f
TopBottomEdgeTracer::downTheHillDirection(
	QRectF const& page_rect, std::vector<QPointF> const& snake, Vec2f const& bounds_dir)
{
	assert(!snake.empty());

	// Take the centroid of a snake.
	QPointF centroid;
	BOOST_FOREACH(QPointF const& pt, snake) {
		centroid += pt;
	}
	centroid /= snake.size();
	
	QLineF line(centroid, centroid + bounds_dir);
	lineBoundedByRect(line, page_rect);

	// The downhill direction is the direction *inside* the page.
	Vec2d const v1(line.p1() - centroid);
	Vec2d const v2(line.p2() - centroid);
	if (v1.squaredNorm() > v2.squaredNorm()) {
		return v1;
	} else {
		return v2;
	}
}

void
TopBottomEdgeTracer::downTheHillSnake(
	std::vector<QPointF>& snake, Grid<GridNode> const& grid, Vec2f const dir)
{
	using namespace boost::lambda;

	size_t const num_nodes = snake.size();
	if (num_nodes <= 1) {
		return;
	}

	float avg_dist = 0;
	for (size_t i = 1; i < num_nodes; ++i) {
		Vec2f const vec(snake[i] - snake[i - 1]);
		avg_dist += sqrt(vec.squaredNorm());
	}
	avg_dist /= num_nodes - 1;

	std::vector<Step> step_storage;

	Vec2f displacements[9];
	int const num_displacements = initDisplacementVectors(displacements, dir);

	float const elasticity_weight = 0.6f;
	float const bending_weight = 8.0f;
	float const external_weight = 0.4f;

	float const segment_dist_threshold = 1;

	for (int iteration = 0; iteration < 40; ++iteration) {
		step_storage.clear();

		std::vector<uint32_t> paths;
		std::vector<uint32_t> new_paths;
		
		for (size_t node_idx = 0; node_idx < num_nodes; ++node_idx) {
			Vec2f const pt(snake[node_idx]);
			float const cur_external_energy = interpolatedGridValue(
				grid, bind<float>(&GridNode::blurred, _1), pt, 1000
			);

			for (int displacement_idx = 0; displacement_idx < num_displacements; ++displacement_idx) {
				Step step;
				step.prevStepIdx = ~uint32_t(0);
				step.pt = pt + displacements[displacement_idx];
				step.pathCost = 0;

				float const adjusted_external_energy = interpolatedGridValue(
					grid, bind<float>(&GridNode::blurred, _1), step.pt, 1000
				);
				if (displacement_idx == 0) {
					step.pathCost += 100;
				} else if (cur_external_energy < 0.01) {
					if (cur_external_energy - adjusted_external_energy < 0.01f) {
						continue;
					}
				}

				step.pathCost += external_weight * adjusted_external_energy;

				float best_cost = NumericTraits<float>::max();
				uint32_t best_prev_step_idx = step.prevStepIdx;

				BOOST_FOREACH(uint32_t prev_step_idx, paths) {
					Step const& prev_step = step_storage[prev_step_idx];
					float cost = prev_step.pathCost + step.pathCost;

					Vec2f const vec(step.pt - prev_step.pt);
					float const vec_len = sqrt(vec.squaredNorm());
					if (vec_len < segment_dist_threshold) {
						cost += 1000;
					}

					// Elasticity.
					float const dist_diff = fabs(avg_dist - vec_len);
					cost += elasticity_weight * (dist_diff / avg_dist);

					// Bending energy.
					if (prev_step.prevStepIdx != ~uint32_t(0) && vec_len >= segment_dist_threshold) {
						Step const& prev_prev_step = step_storage[prev_step.prevStepIdx];
						Vec2f prev_normal(prev_step.pt - prev_prev_step.pt);
						std::swap(prev_normal[0], prev_normal[1]);
						prev_normal[0] = -prev_normal[0];
						float const prev_normal_len = sqrt(prev_normal.squaredNorm());
						if (prev_normal_len < segment_dist_threshold) {
							cost += 1000;
						} else {
							float const cos = vec.dot(prev_normal) / (vec_len * prev_normal_len);
							//cost += 0.7 * fabs(cos);
							cost += bending_weight * cos * cos;
						}
					}

					assert(cost < NumericTraits<float>::max());

					if (cost < best_cost) {
						best_cost = cost;
						best_prev_step_idx = prev_step_idx;
					}
				}

				step.prevStepIdx = best_prev_step_idx;
				if (best_prev_step_idx != ~uint32_t(0)) {
					step.pathCost = best_cost;
				}
					
				new_paths.push_back(step_storage.size());
				step_storage.push_back(step);
			}
			assert(!new_paths.empty());
			paths.swap(new_paths);
			new_paths.clear();
		}
		
		uint32_t best_path_idx = ~uint32_t(0);
		float best_cost = NumericTraits<float>::max();
		BOOST_FOREACH(uint32_t last_step_idx, paths) {
			Step const& step = step_storage[last_step_idx];
			if (step.pathCost < best_cost) {
				best_cost = step.pathCost;
				best_path_idx = last_step_idx;
			}
		}

		// Having found the best path, convert it back to a snake.
		snake.clear();
		uint32_t step_idx = best_path_idx;
		while (step_idx != ~uint32_t(0)) {
			Step const& step = step_storage[step_idx];
			snake.push_back(step.pt);
			step_idx = step.prevStepIdx;
		}
		assert(num_nodes == snake.size());	
	}
}

void
TopBottomEdgeTracer::upTheHillSnake(
	std::vector<QPointF>& snake, Grid<GridNode> const& grid, Vec2f const dir)
{
	using namespace boost::lambda;

	size_t const num_nodes = snake.size();
	if (num_nodes <= 1) {
		return;
	}

	float avg_dist = 0;
	for (size_t i = 1; i < num_nodes; ++i) {
		Vec2f const vec(snake[i] - snake[i - 1]);
		avg_dist += sqrt(vec.squaredNorm());
	}
	avg_dist /= num_nodes - 1;

	std::vector<Step> step_storage;

	Vec2f displacements[9];
	int const num_displacements = initDisplacementVectors(displacements, dir);
	for (int i = 0; i < num_displacements; ++i) {
		// We need more accuracy here.
		displacements[i] *= 0.5f;
	}

	float const elasticity_weight = 0.6f;
	float const bending_weight = 3.0f;
	float const external_weight = 2.0f;

	float const segment_dist_threshold = 1;

	for (int iteration = 0; iteration < 40; ++iteration) {
		step_storage.clear();

		std::vector<uint32_t> paths;
		std::vector<uint32_t> new_paths;
		
		for (size_t node_idx = 0; node_idx < num_nodes; ++node_idx) {
			Vec2f const pt(snake[node_idx]);
			float const cur_external_energy = -interpolatedGridValue(
				grid, bind<float>(&GridNode::absDirDeriv, _1), pt, 1000
			);

			for (int displacement_idx = 0; displacement_idx < num_displacements; ++displacement_idx) {
				Step step;
				step.prevStepIdx = ~uint32_t(0);
				step.pt = pt + displacements[displacement_idx];
				step.pathCost = 0;

				float const adjusted_external_energy = -interpolatedGridValue(
					grid, bind<float>(&GridNode::absDirDeriv, _1), step.pt, 1000
				);
				if (displacement_idx == 0 && adjusted_external_energy > -0.02) {
					// Discorage staying on the spot if the gradient magnitude is too
					// small at that point.
					step.pathCost += 100;
				}

				step.pathCost += external_weight * adjusted_external_energy;

				float best_cost = NumericTraits<float>::max();
				uint32_t best_prev_step_idx = step.prevStepIdx;

				BOOST_FOREACH(uint32_t prev_step_idx, paths) {
					Step const& prev_step = step_storage[prev_step_idx];
					float cost = prev_step.pathCost + step.pathCost;

					Vec2f const vec(step.pt - prev_step.pt);
					float const vec_len = sqrt(vec.squaredNorm());
					if (vec_len < segment_dist_threshold) {
						cost += 1000;
					}

					// Elasticity.
					float const dist_diff = fabs(avg_dist - vec_len);
					cost += elasticity_weight * (dist_diff / avg_dist);

					// Bending energy.
					if (prev_step.prevStepIdx != ~uint32_t(0) && vec_len >= segment_dist_threshold) {
						Step const& prev_prev_step = step_storage[prev_step.prevStepIdx];
						Vec2f prev_normal(prev_step.pt - prev_prev_step.pt);
						std::swap(prev_normal[0], prev_normal[1]);
						prev_normal[0] = -prev_normal[0];
						float const prev_normal_len = sqrt(prev_normal.squaredNorm());
						if (prev_normal_len < segment_dist_threshold) {
							cost += 1000;
						} else {
							float const cos = vec.dot(prev_normal) / (vec_len * prev_normal_len);
							//cost += 0.7 * fabs(cos);
							cost += bending_weight * cos * cos;
						}
					}

					assert(cost < NumericTraits<float>::max());

					if (cost < best_cost) {
						best_cost = cost;
						best_prev_step_idx = prev_step_idx;
					}
				}

				step.prevStepIdx = best_prev_step_idx;
				if (best_prev_step_idx != ~uint32_t(0)) {
					step.pathCost = best_cost;
				}
					
				new_paths.push_back(step_storage.size());
				step_storage.push_back(step);
			}
			assert(!new_paths.empty());
			paths.swap(new_paths);
			new_paths.clear();
		}
		
		uint32_t best_path_idx = ~uint32_t(0);
		float best_cost = NumericTraits<float>::max();
		BOOST_FOREACH(uint32_t last_step_idx, paths) {
			Step const& step = step_storage[last_step_idx];
			if (step.pathCost < best_cost) {
				best_cost = step.pathCost;
				best_path_idx = last_step_idx;
			}
		}

		// Having found the best path, convert it back to a snake.
		snake.clear();
		uint32_t step_idx = best_path_idx;
		while (step_idx != ~uint32_t(0)) {
			Step const& step = step_storage[step_idx];
			snake.push_back(step.pt);
			step_idx = step.prevStepIdx;
		}
		assert(num_nodes == snake.size());	
	}
}

int
TopBottomEdgeTracer::initDisplacementVectors(Vec2f vectors[], Vec2f valid_direction)
{
	int out_idx = 0;

	// This one must always be present, and must be first, as we want to prefer it
	// over another one with exactly the same score.
	vectors[out_idx++] = Vec2f(0, 0); 

	static float const dx[] = {
		-1, 0, 1,
		-1,    1,
		-1, 0, 1
	};

	static float const dy[] = {
		-1, -1, -1,
		 0,      0,
		 1,  1,  1
	};

	for (int i = 0; i < 8; ++i) {
		Vec2f const vec(dx[i], dy[i]);
		if (vec.dot(valid_direction) > 0) {
			vectors[out_idx++] = vec;
		}
	}

	return out_idx;
}

QImage
TopBottomEdgeTracer::visualizeGradient(Grid<GridNode> const& grid, QImage const* background)
{
	int const width = grid.width();
	int const height = grid.height();
	int const grid_stride = grid.stride();

	// First let's find the maximum and minimum values.
	float min_value = NumericTraits<float>::max();
	float max_value = NumericTraits<float>::min();

	GridNode const* grid_line = grid.data();
	for (int y = 0; y < height; ++y) {
		for (int x = 0; x < width; ++x) {
			float const value = grid_line[x].dirDeriv;
			if (value < min_value) {
				min_value = value;
			} else if (value > max_value) {
				max_value = value;
			}
		}
		grid_line += grid_stride;
	}

	float scale = std::max(max_value, -min_value);
	if (scale > std::numeric_limits<float>::epsilon()) {
		scale = 255.0f / scale;
	} 

	QImage overlay(width, height, QImage::Format_ARGB32_Premultiplied);
	uint32_t* overlay_line = (uint32_t*)overlay.bits();
	int const overlay_stride = overlay.bytesPerLine() / 4;

	grid_line = grid.data();
	for (int y = 0; y < height; ++y) {
		for (int x = 0; x < width; ++x) {
			float const value = grid_line[x].dirDeriv * scale;
			int const magnitude = qBound(0, (int)(fabs(value) + 0.5), 255);
			if (value > 0) {
				// Red for positive gradients which indicate bottom edges.
				overlay_line[x] = qRgba(magnitude, 0, 0, magnitude);
			} else {
				overlay_line[x] = qRgba(0, 0, magnitude, magnitude);
			}
		}
		grid_line += grid_stride;
		overlay_line += overlay_stride;
	}

	QImage canvas;
	if (background) {
		canvas = background->convertToFormat(QImage::Format_ARGB32_Premultiplied);
	} else {
		canvas = QImage(width, height, QImage::Format_ARGB32_Premultiplied);
		canvas.fill(0xffffffff); // Opaque white.
	}
	
	QPainter painter(&canvas);
	painter.drawImage(0, 0, overlay);

	return canvas;
}

QImage
TopBottomEdgeTracer::visualizeBlurredGradient(Grid<GridNode> const& grid)
{
	int const width = grid.width();
	int const height = grid.height();
	int const grid_stride = grid.stride();

	// First let's find the maximum and minimum values.
	float min_value = NumericTraits<float>::max();
	float max_value = NumericTraits<float>::min();

	GridNode const* grid_line = grid.data();
	for (int y = 0; y < height; ++y) {
		for (int x = 0; x < width; ++x) {
			float const value = grid_line[x].blurred;
			if (value < min_value) {
				min_value = value;
			} else if (value > max_value) {
				max_value = value;
			}
		}
		grid_line += grid_stride;
	}

	float scale = std::max(max_value, -min_value);
	if (scale > std::numeric_limits<float>::epsilon()) {
		scale = 255.0f / scale;
	} 

	QImage overlay(width, height, QImage::Format_ARGB32_Premultiplied);
	uint32_t* overlay_line = (uint32_t*)overlay.bits();
	int const overlay_stride = overlay.bytesPerLine() / 4;

	grid_line = grid.data();
	for (int y = 0; y < height; ++y) {
		for (int x = 0; x < width; ++x) {
			float const value = grid_line[x].blurred * scale;
			int const magnitude = qBound(0, (int)(fabs(value) + 0.5), 255);
			overlay_line[x] = qRgba(magnitude, 0, 0, magnitude);
		}
		grid_line += grid_stride;
		overlay_line += overlay_stride;
	}

	QImage canvas(grid.width(), grid.height(), QImage::Format_ARGB32_Premultiplied);
	canvas.fill(0xffffffff); // Opaque white.
	QPainter painter(&canvas);
	painter.drawImage(0, 0, overlay);

	return canvas;
}

QImage
TopBottomEdgeTracer::visualizePaths(
	QImage const& background, Grid<GridNode> const& grid,
	std::pair<QLineF, QLineF> const& bounds, std::vector<QPoint> const& path_endpoints)
{
	QImage canvas(background.convertToFormat(QImage::Format_RGB32));
	uint32_t* const canvas_data = (uint32_t*)canvas.bits();
	int const canvas_stride = canvas.bytesPerLine() / 4;

	int const width = grid.width();
	int const height = grid.height();
	int const grid_stride = grid.stride();
	GridNode const* const grid_data = grid.data();
	
	int const nbh_canvas_offsets[8] = {
		-canvas_stride - 1, -canvas_stride, -canvas_stride + 1,
		               - 1,                                + 1,
		+canvas_stride - 1, +canvas_stride, +canvas_stride + 1
	};
	int const nbh_grid_offsets[8] = {
		-grid_stride - 1, -grid_stride, -grid_stride + 1,
		             - 1,                            + 1,
		+grid_stride - 1, +grid_stride, +grid_stride + 1
	};

	BOOST_FOREACH(QPoint const path_endpoint, path_endpoints) {
		int grid_offset = path_endpoint.x() + path_endpoint.y() * grid_stride;
		int canvas_offset = path_endpoint.x() + path_endpoint.y() * canvas_stride;
		for (;;) {
			GridNode const* node = grid_data + grid_offset;
			canvas_data[canvas_offset] = 0x00ff0000;
			if (!node->hasPathContinuation()) {
				break;
			}

			int const nbh_idx = node->prevNeighbourIdx();
			grid_offset += nbh_grid_offsets[nbh_idx];
			canvas_offset += nbh_canvas_offsets[nbh_idx];
		}
	}

	QPainter painter(&canvas);
	painter.setRenderHint(QPainter::Antialiasing);
	QPen pen(Qt::blue);
	pen.setWidthF(1.0);
	painter.setPen(pen);
	painter.drawLine(bounds.first);
	painter.drawLine(bounds.second);

	return canvas;
}

QImage
TopBottomEdgeTracer::visualizePaths(
	QImage const& background, std::vector<std::vector<QPoint> > const& paths,
	std::pair<QLineF, QLineF> const& bounds)
{
	QImage canvas(background.convertToFormat(QImage::Format_RGB32));
	uint32_t* const canvas_data = (uint32_t*)canvas.bits();
	int const canvas_stride = canvas.bytesPerLine() / 4;
	
	BOOST_FOREACH(std::vector<QPoint> const& path, paths) {
		BOOST_FOREACH(QPoint pt, path) {
			canvas_data[pt.x() + pt.y() * canvas_stride] = 0x00ff0000;
		}
	}

	QPainter painter(&canvas);
	painter.setRenderHint(QPainter::Antialiasing);
	QPen pen(Qt::blue);
	pen.setWidthF(1.0);
	painter.setPen(pen);
	painter.drawLine(bounds.first);
	painter.drawLine(bounds.second);

	return canvas;
}

QImage
TopBottomEdgeTracer::visualizeSnakes(
	QImage const& background, std::vector<std::vector<QPointF> > const& snakes,
	std::pair<QLineF, QLineF> const& bounds)
{
	QImage canvas(background.convertToFormat(QImage::Format_ARGB32_Premultiplied));
	QPainter painter(&canvas);
	painter.setRenderHint(QPainter::Antialiasing);
	
	QPen snake_pen(QColor(0, 255, 0));
	snake_pen.setWidthF(1.5);

	QBrush knot_brush(QColor(255, 255, 0, 180));
	painter.setBrush(knot_brush);

	QRectF knot_rect(0, 0, 7, 7);

	BOOST_FOREACH(std::vector<QPointF> const& snake, snakes) {
		if (snake.empty()) {
			continue;
		}

		painter.setPen(snake_pen);
		painter.drawPolyline(&snake[0], snake.size());
		painter.setPen(Qt::NoPen);
		BOOST_FOREACH(QPointF const& knot, snake) {
			knot_rect.moveCenter(knot);
			painter.drawEllipse(knot_rect);
		}
	}

	QPen bounds_pen(Qt::blue);
	bounds_pen.setWidthF(1.5);
	painter.setPen(bounds_pen);
	painter.drawLine(bounds.first);
	painter.drawLine(bounds.second);

	return canvas;
}

QImage
TopBottomEdgeTracer::visualizePolylines(
	QImage const& background, std::list<std::vector<QPointF> > const& polylines,
	std::pair<QLineF, QLineF> const& bounds)
{
	QImage canvas(background.convertToFormat(QImage::Format_ARGB32_Premultiplied));
	QPainter painter(&canvas);
	painter.setRenderHint(QPainter::Antialiasing);
	
	QPen polyline_pen(QColor(255, 0, 0, 100));
	polyline_pen.setWidthF(4.0);
	painter.setPen(polyline_pen);

	BOOST_FOREACH(std::vector<QPointF> const& polyline, polylines) {
		if (polyline.empty()) {
			continue;
		}

		painter.drawPolyline(&polyline[0], polyline.size());
	}

	QPen bounds_pen(Qt::blue);
	bounds_pen.setWidthF(1.5);
	painter.setPen(bounds_pen);
	painter.drawLine(bounds.first);
	painter.drawLine(bounds.second);

	return canvas;
}

} // namespace dewarping
