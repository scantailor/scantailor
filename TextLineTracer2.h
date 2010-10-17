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

#ifndef TEXT_LINE_TRACER2_H_
#define TEXT_LINE_TRACER2_H_

#include <QPoint>
#include <QPointF>
#include <vector>
#include <stdint.h>

class Dpi;
class QImage;
class QColor;
class TaskStatus;
class DebugImages;

namespace imageproc
{
	class BinaryImage;
	class GrayImage;
	class ConnectivityMap;
}

class TextLineTracer2
{
public:
	static void trace(
		imageproc::GrayImage const& input, Dpi const& dpi,
		TaskStatus const& status, DebugImages* dbg = 0);
private:
	struct Sample;
	struct ApproxLine;
	struct Comp;
	struct Node;

	static imageproc::GrayImage downscale(imageproc::GrayImage const& input, Dpi const& dpi);

	static imageproc::GrayImage verticalSobel(imageproc::GrayImage const& src);

	static imageproc::BinaryImage findVerticalPeaks(imageproc::GrayImage const& input);

	static void seedFillVerticalOnly(imageproc::GrayImage& seed, imageproc::GrayImage const& mask);

	static void killShortComponents(imageproc::ConnectivityMap& cmap, std::vector<Comp>& comps);

	static void approximateRidgeEndings(
		imageproc::ConnectivityMap const& cmap, std::vector<Comp>& comps);

	static void approxLine(ApproxLine& approx_line, std::vector<Sample> const& samples);

	static void markLeftRightEndpoints(
		imageproc::ConnectivityMap const& cmap, std::vector<Comp>& comps);

	static void createNodes(std::vector<Node>& nodes, std::vector<Comp> const& comps);

	static void connectNodes(std::vector<Node>& nodes, std::vector<Comp> const& comps);

	static QImage overlay(QImage const& background, QImage const& overlay);

	static QImage colorizedOverlay(
		QImage const& background, imageproc::BinaryImage const& overlay, QColor const& color);

	static QImage visualizeApproxLines(QImage const& background, std::vector<Comp> const& comps);

	static QImage visualizeEndpoints(QImage const& background, std::vector<Comp> const& comps);

	static QImage visualizeGraph(
		QImage const& background, QImage const& overlay, std::vector<Node> const& nodes);
};

#endif
