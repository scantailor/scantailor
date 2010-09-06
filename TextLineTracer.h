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

#ifndef TEXT_LINE_TRACER_H_
#define TEXT_LINE_TRACER_H_

#include "imageproc/InfluenceMap.h"
#include "FastQueue.h"
#include <QPoint>
#include <QLineF>
#include <vector>
#include <list>
#include <set>
#include <stdint.h>

class QImage;
class TaskStatus;
class DebugImages;

namespace imageproc
{
	class BinaryImage;
	class ConnectivityMap;
}

class TextLineTracer
{
public:
	static void trace(
		imageproc::BinaryImage const& input,
		TaskStatus const& status, DebugImages* dbg = 0);
private:
	struct BoundingBox;
	struct Component;
	struct Connection;
	class CompPriorityQueue;

	static void calcBoundingBoxes(
		imageproc::ConnectivityMap const& cmap, std::vector<Component>& components);

	static void subdivideLongComponents(
		imageproc::ConnectivityMap& cmap, std::vector<Component>& components);

	static int calcMedianCompHeight(std::vector<Component> const& components);

	static void findVoronoiConnections(
		imageproc::InfluenceMap const& imap, std::set<Connection>& connections);

	static inline void processPossibleConnection(
		std::set<Connection>& connections,
		imageproc::InfluenceMap::Cell const& cell1,
		imageproc::InfluenceMap::Cell const& cell2);

	static void labelEdgeComponents(
		std::vector<Component>& components,
		imageproc::InfluenceMap const& imap, int x, int flag);

	template<typename BetterX>
	static QLineF estimateVerticalCutter(
		imageproc::InfluenceMap const& imap, std::vector<Component>& components,
		int flag, int median_comp_height, BetterX better_x);

	static void connectEndpoints(
		std::vector<Component>& components, std::list<std::vector<uint32_t> >& lines);

	static void propagateLengthFrom(
		Component& comp, std::vector<Component>& components, CompPriorityQueue& queue);

	static void propagateLengthFromTo(
		QPoint vec_from_prev, double vec_from_prev_len, Component const& from_comp,
		QPoint comp_center, Component& to_comp, CompPriorityQueue& queue);

	static void propagateMaxCurvatureFrom(
		Component& comp, std::vector<Component>& components, CompPriorityQueue& queue);

	static void propagateMaxCurvatureFromTo(
		QPointF normal_from_prev, Component const& from_comp,
		QPoint comp_center, Component& to_comp, CompPriorityQueue& queue);

	static QImage overlay(QImage const& background, imageproc::BinaryImage const& overlay);

	static QImage visualizeLeftRightComponents(
		std::vector<Component> const& components,
		imageproc::InfluenceMap const& imap);

	static QImage visualizeCutters(
		QImage const& background, QLineF const& cutter1, QLineF const& cutter2);

	static QImage visualizeTracedLinesAndCutters(
		QImage const& background, std::list<std::vector<uint32_t> > const& lines,
		std::vector<Component> const& components, QLineF const& cutter1, QLineF const& cutter2);
};

#endif
