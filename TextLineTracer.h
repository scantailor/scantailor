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

#ifndef TEXT_LINE_TRACER3_H_
#define TEXT_LINE_TRACER3_H_

#include "Grid.h"
#include <QPoint>
#include <QPointF>
#include <QLineF>
#include <vector>
#include <list>
#include <deque>
#include <memory>
#include <set>
#include <utility>
#include <stdint.h>

class Dpi;
class QImage;
class QColor;
class QRect;
class TaskStatus;
class DebugImages;

namespace imageproc
{
	class BinaryImage;
	class GrayImage;
	class ConnectivityMap;
}

class TextLineTracer
{
public:
	static std::list<std::vector<QPointF> > trace(
		imageproc::GrayImage const& input, Dpi const& dpi,
		QRect const& content_rect,
		TaskStatus const& status, DebugImages* dbg = 0);
private:
	class CentroidCalculator;
	struct Region;
	struct GridNode;
	struct RegionGrowingPosition;
	class RegionGrowingQueue;
	struct Edge;
	struct EdgeConnection;
	struct EdgeNode;
	class ShortestPathQueue;

	typedef uint32_t RegionIdx;
	typedef uint32_t EdgeNodeIdx;

	static imageproc::GrayImage downscale(imageproc::GrayImage const& input, Dpi const& dpi);

	static void segmentBlurredTextLines(
		imageproc::GrayImage const& blurred, imageproc::BinaryImage const& thick_mask,
		std::list<std::vector<QPointF> >& out, DebugImages* dbg);

	static void labelAndGrowRegions(
		imageproc::GrayImage const& blurred, imageproc::BinaryImage region_seeds,
		imageproc::BinaryImage const& thick_mask, std::vector<Region>& regions,
		std::set<Edge>& edges, DebugImages* dbg);

	static void extractEdegeNodePaths(
		std::vector<std::vector<uint32_t> >& edge_node_paths,
		std::vector<EdgeNode> const& edge_nodes,
		std::vector<Region> const& regions);

	static void edgeSequencesToPolylines(
		std::vector<std::vector<EdgeNodeIdx> > const& edge_node_paths,
		std::vector<EdgeNode> const& edge_nodes, std::vector<Region> const& regions,
		std::list<std::vector<QPointF> >& polylines);

	static RegionIdx findConnectingRegion(Edge const& edge1, Edge const& edge2);

	static bool isCurvatureConsistent(std::vector<QPointF> const& polyline);

	static bool isInsideBounds(
		QPointF const& pt, QLineF const& left_bound, QLineF const& right_bound);

	static void filterCurves(std::list<std::vector<QPointF> >& polylines,
		QLineF const& left_bound, QLineF const& right_bound);

	static void pickTopBottomLines(std::list<std::vector<QPointF> >& polylines);

	static void makeLeftToRight(std::vector<QPointF>& polyline);

	static void extendOrTrimPolyline(
		std::vector<QPointF>& polyline, QLineF const& left_bound, QLineF const right_bound,
		imageproc::GrayImage const& blurred, imageproc::BinaryImage const& thick_mask);

	static bool trimFront(std::deque<QPointF>& polyline, QLineF const& bound);

	static bool trimBack(std::deque<QPointF>& polyline, QLineF const& bound);

	static void growFront(
		std::deque<QPointF>& polyline, QLineF const& bound,
		imageproc::GrayImage const& blurred, imageproc::BinaryImage const& thick_mask);

	static void growBack(
		std::deque<QPointF>& polyline, QLineF const& bound,
		imageproc::GrayImage const& blurred, imageproc::BinaryImage const& thick_mask);

	static void intersectFront(
		std::deque<QPointF>& polyline, QLineF const& bound);

	static void intersectBack(
		std::deque<QPointF>& polyline, QLineF const& bound);

	static void intersectWithVerticalBoundaries(
		std::vector<QPointF>& polylines, QLineF const& left_bound, QLineF const& right_bound);

	static void sanitizeBinaryImage(imageproc::BinaryImage& image, QRect const& content_rect);

	static QImage visualizeVerticalBounds(
		QImage const& background, std::pair<QLineF, QLineF> const& bounds);

	static QImage visualizeRegions(Grid<GridNode> const& grid);

	static QImage visualizePolylines(
		QImage const& background, std::list<std::vector<QPointF> > const& polylines,
		std::pair<QLineF, QLineF> const* vert_bounds = 0);

	static QImage visualizeExtendedPolylines(
		QImage const& blurred, imageproc::BinaryImage const&  thick_mask,
		std::list<std::vector<QPointF> > const& polylines,
		QLineF const& left_bound, QLineF const& right_bound);
};

#endif
