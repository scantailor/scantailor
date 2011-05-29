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

#ifndef DEWARPING_TEXT_LINE_REFINER_H_
#define DEWARPING_TEXT_LINE_REFINER_H_

#include "Grid.h"
#include "VecNT.h"
#include "Dpi.h"
#include "imageproc/GrayImage.h"
#include <QPointF>
#include <QLineF>
#include <vector>
#include <list>
#include <stdint.h>

class Dpi;
class DebugImages;
class QImage;

namespace dewarping
{

class TextLineRefiner
{
public:
	TextLineRefiner(
		imageproc::GrayImage const& image,
		Dpi const& dpi, Vec2f const& unit_down_vector);

	void refine(std::list<std::vector<QPointF> >& polylines,
		int iterations, DebugImages* dbg) const;
private:
	enum OnConvergence { ON_CONVERGENCE_STOP, ON_CONVERGENCE_GO_FINER };

	class SnakeLength;
	struct FrenetFrame;
	class Optimizer;

	struct SnakeNode
	{
		Vec2f center;
		float ribHalfLength;
	};

	struct Snake
	{
		std::vector<SnakeNode> nodes;
		int iterationsRemaining;

		Snake() : iterationsRemaining(0) {}
	};

	struct Step
	{
		SnakeNode node;
		uint32_t prevStepIdx;
		float pathCost;
	};

	void calcBlurredGradient(Grid<float>& gradient, float h_sigma, float v_sigma) const;

	static float externalEnergyAt(
		Grid<float> const& gradient, Vec2f const& pos, float penalty_if_outside);

	static Snake makeSnake(std::vector<QPointF> const& polyline, int iterations);

	static void calcFrenetFrames(
		std::vector<FrenetFrame>& frenet_frames, Snake const& snake,
		SnakeLength const& snake_length, Vec2f const& unit_down_vec);

	void evolveSnake(Snake& snake, Grid<float> const& gradient, OnConvergence on_convergence) const;

	QImage visualizeGradient(Grid<float> const& gradient) const;

	QImage visualizeSnakes(std::vector<Snake> const& snakes, Grid<float> const* gradient = 0) const;
	
	imageproc::GrayImage m_image;
	Dpi m_dpi;
	Vec2f m_unitDownVec;
};

} // namespace dewarping

#endif
