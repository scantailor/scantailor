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
#include <QPoint>
#include <vector>
#include <set>
#include <stdint.h>

class QImage;
class TaskStatus;
class DebugImages;

namespace imageproc
{
	class BinaryImage;
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

	static void findVoronoiConnections(
		imageproc::InfluenceMap const& imap, std::set<Connection>& connections);

	static QImage overlay(QImage const& background, imageproc::BinaryImage const& overlay);

	static inline void processPossibleConnection(
		std::set<Connection>& connections,
		imageproc::InfluenceMap::Cell const& cell1,
		imageproc::InfluenceMap::Cell const& cell2);

	static QImage visualizeConnections(
		QImage const& background, std::set<Connection> const& connections,
		std::vector<Component> const& components);

	static void labelGroup(std::vector<Component>& components, Component& comp, uint32_t label);
};

#endif
