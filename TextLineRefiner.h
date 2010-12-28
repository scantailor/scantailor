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

#ifndef TEXT_LINE_REFINER_H_
#define TEXT_LINE_REFINER_H_

#include "Grid.h"
#include "VecNT.h"
#include <QPointF>
#include <QLineF>
#include <vector>
#include <list>
#include <stdint.h>

class Dpi;
class DebugImages;
class QImage;

namespace imageproc
{
	class GrayImage;
}

class TextLineRefiner
{
public:
	TextLineRefiner(
		imageproc::GrayImage const& image, Dpi const& dpi, DebugImages* dbg);

	void refine(std::list<std::vector<QPointF> >& polylines, int iterations,
		DebugImages* dbg, QImage const* dbg_background);
private:
	struct SnakeNode
	{
		Vec2f center;
		float ribHalfLength;
	};

	typedef std::vector<SnakeNode> Snake;

	struct Step
	{
		SnakeNode node;
		uint32_t prevStepIdx;
		float pathCost;
	};

	static void verticalSobel(imageproc::GrayImage const& src, Grid<float>& dst);

	float externalEnergyAt(Vec2f const& pos, float penalty_if_outside) const;

	static Snake makeSnake(std::vector<QPointF> const& polyline);

	static void calcUpwardPointingNormals(Snake const& snake, std::vector<Vec2f>& normals);

	void evolveSnake(Snake& snake) const;

	static QImage visualizeGradient(Grid<float> const& gradient, QImage const* background = 0);

	QImage visualizeSnakes(std::vector<Snake> const& snakes, QImage const* background = 0) const;
	
	Grid<float> m_gradient;
};

#endif
