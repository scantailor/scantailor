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

#ifndef DEWARPING_TEXT_LINE_TRACER_H_
#define DEWARPING_TEXT_LINE_TRACER_H_

#include "Grid.h"
#include "VecNT.h"
#include <QPoint>
#include <QPointF>
#include <QLineF>
#include <vector>
#include <list>
#include <utility>
#include <stdint.h>

class Dpi;
class QImage;
class QSize;
class QRect;
class TaskStatus;
class DebugImages;

namespace imageproc
{
	class BinaryImage;
	class GrayImage;
	class SEDM;
}

namespace dewarping
{

class DistortionModelBuilder;

class TextLineTracer
{
public:
	static void trace(
		imageproc::GrayImage const& input, Dpi const& dpi,
		QRect const& content_rect, DistortionModelBuilder& output,
		TaskStatus const& status, DebugImages* dbg = 0);
private:
	static imageproc::GrayImage downscale(imageproc::GrayImage const& input, Dpi const& dpi);

	static void sanitizeBinaryImage(imageproc::BinaryImage& image, QRect const& content_rect);

	static void extractTextLines(
		std::list<std::vector<QPointF> >& out, imageproc::GrayImage const& image,
		std::pair<QLineF, QLineF> const& bounds, DebugImages* dbg);

	static Vec2f calcAvgUnitVector(std::pair<QLineF, QLineF> const& bounds);

	static imageproc::BinaryImage closeWithObstacles(
		imageproc::BinaryImage const& image,
		imageproc::BinaryImage const& obstacles, QSize const& brick);

	static QLineF calcMidLine(QLineF const& line1, QLineF const& line2);

	static void findMidLineSeeds(
		imageproc::SEDM const& sedm, QLineF mid_line, std::vector<QPoint>& seeds);

	static bool isCurvatureConsistent(std::vector<QPointF> const& polyline);

	static bool isInsideBounds(
		QPointF const& pt, QLineF const& left_bound, QLineF const& right_bound);

	static void filterShortCurves(std::list<std::vector<QPointF> >& polylines,
		QLineF const& left_bound, QLineF const& right_bound);

	static void filterOutOfBoundsCurves(std::list<std::vector<QPointF> >& polylines,
		QLineF const& left_bound, QLineF const& right_bound);

	static void filterEdgyCurves(std::list<std::vector<QPointF> >& polylines);

	static QImage visualizeVerticalBounds(
		QImage const& background, std::pair<QLineF, QLineF> const& bounds);

	static QImage visualizeGradient(QImage const& background, Grid<float> const& grad);

	static QImage visualizeMidLineSeeds(QImage const& background, imageproc::BinaryImage const& overlay,
		std::pair<QLineF, QLineF> bounds, QLineF mid_line, std::vector<QPoint> const& seeds);

	static QImage visualizePolylines(
		QImage const& background, std::list<std::vector<QPointF> > const& polylines,
		std::pair<QLineF, QLineF> const* vert_bounds = 0);
};

} // namespace dewarping

#endif
