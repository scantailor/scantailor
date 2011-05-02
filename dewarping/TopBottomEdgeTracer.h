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

#ifndef DEWARPING_TOP_BOTTOM_EDGE_TRACER_H_
#define DEWARPING_TOP_BOTTOM_EDGE_TRACER_H_

#include "Grid.h"
#include "VecNT.h"
#include <QPointF>
#include <QLineF>
#include <QRectF>
#include <list>
#include <utility>
#include <vector>

class TaskStatus;
class DebugImages;
class QImage;
class QPoint;

namespace imageproc
{
	class GrayImage;
}

namespace dewarping
{

class DistortionModelBuilder;

class TopBottomEdgeTracer
{
public:
	static void trace(
		imageproc::GrayImage const& image, std::pair<QLineF, QLineF> bounds,
		DistortionModelBuilder& output, TaskStatus const& status, DebugImages* dbg = 0);
private:
	struct GridNode;
	class PrioQueue;
	struct Step;

	static bool intersectWithRect(std::pair<QLineF, QLineF>& bounds, QRectF const& rect);

	static void forceSameDirection(std::pair<QLineF, QLineF>& bounds);

	static void calcDirectionalDerivative(
		Grid<GridNode>& gradient, imageproc::GrayImage const& image, Vec2f const& direction);

	static void horizontalSobelInPlace(Grid<GridNode>& grid);

	static void verticalSobelInPlace(Grid<GridNode>& grid);

	static Vec2f calcAvgUnitVector(std::pair<QLineF, QLineF> const& bounds);

	static Vec2f directionFromPointToLine(QPointF const& pt, QLineF const& line);

	static void prepareForShortestPathsFrom(PrioQueue& queue, Grid<GridNode>& grid, QLineF const& from); 

	static void propagateShortestPaths(Vec2f const& direction, PrioQueue& queue, Grid<GridNode>& grid);

	static int initNeighbours(int* next_nbh_offsets, int* prev_nbh_indexes, int stride, Vec2f const& direction);

	static std::vector<QPoint> locateBestPathEndpoints(Grid<GridNode> const& grid, QLineF const& line);

	static std::vector<QPoint> tracePathFromEndpoint(Grid<GridNode> const& grid, QPoint const& endpoint);

	static std::vector<QPointF> pathToSnake(Grid<GridNode> const& grid, QPoint const& endpoint);

	static void gaussBlurGradient(Grid<GridNode>& grid);

	static Vec2f downTheHillDirection(
		QRectF const& page_rect, std::vector<QPointF> const& snake, Vec2f const& bounds_dir);

	static void downTheHillSnake(std::vector<QPointF>& snake, Grid<GridNode> const& grid, Vec2f dir);

	static void upTheHillSnake(std::vector<QPointF>& snake, Grid<GridNode> const& grid, Vec2f dir);

	static int initDisplacementVectors(Vec2f vectors[], Vec2f valid_direction);

	template<typename Extractor>
	static float interpolatedGridValue(
		Grid<GridNode> const& grid, Extractor extractor, Vec2f pos, float default_value);

	static QImage visualizeGradient(Grid<GridNode> const& grid, QImage const* background = 0);

	static QImage visualizeBlurredGradient(Grid<GridNode> const& grid);

	static QImage visualizePaths(QImage const& background, Grid<GridNode> const& grid,
		std::pair<QLineF, QLineF> const& bounds, std::vector<QPoint> const& path_endpoints);

	static QImage visualizePaths(
		QImage const& background, std::vector<std::vector<QPoint> > const& paths,
		std::pair<QLineF, QLineF> const& bounds);

	static QImage visualizeSnakes(
		QImage const& background, std::vector<std::vector<QPointF> > const& snakes,
		std::pair<QLineF, QLineF> const& bounds);

	static QImage visualizePolylines(
		QImage const& background, std::list<std::vector<QPointF> > const& snakes,
		std::pair<QLineF, QLineF> const& bounds);
};

} // namespace dewarping

#endif
