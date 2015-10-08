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

#include "TextLineRefiner.h"
#include "Dpi.h"
#include "VecNT.h"
#include "NumericTraits.h"
#include "DebugImages.h"
#include "imageproc/GrayImage.h"
#include "imageproc/GaussBlur.h"
#include "imageproc/Sobel.h"
#ifndef Q_MOC_RUN
#include <boost/scoped_array.hpp>
#include <boost/foreach.hpp>
#include <boost/lambda/lambda.hpp>
#include <boost/lambda/bind.hpp>
#endif
#include <QImage>
#include <QPainter>
#include <QPen>
#include <QColor>
#include <Qt>
#include <QtGlobal>
#include <QDebug>
#include <limits>
#include <algorithm>
#include <math.h>
#include <assert.h>

using namespace imageproc;

namespace dewarping
{

class TextLineRefiner::SnakeLength
{
public:
	explicit SnakeLength(Snake const& snake);

	float totalLength() const { return m_totalLength; }

	float avgSegmentLength() const { return m_avgSegmentLength; }

	float arcLengthAt(size_t node_idx) const { return m_integralLength[node_idx]; }

	float arcLengthFractionAt(size_t node_idx) const {
		return m_integralLength[node_idx] * m_rTotalLength;
	}

	float lengthFromTo(size_t from_node_idx, size_t to_node_idx) const {
		return m_integralLength[to_node_idx] - m_integralLength[from_node_idx];
	}
private:
	std::vector<float> m_integralLength;
	float m_totalLength;
	float m_rTotalLength; // Reciprocal of the above.
	float m_avgSegmentLength;
};


struct TextLineRefiner::FrenetFrame
{
	Vec2f unitTangent;
	Vec2f unitDownNormal;
};


class TextLineRefiner::Optimizer
{
public:
	Optimizer(Snake const& snake, Vec2f const& unit_down_vec, float factor);

	bool thicknessAdjustment(Snake& snake, Grid<float> const& gradient);

	bool tangentMovement(Snake& snake, Grid<float> const& gradient);

	bool normalMovement(Snake& snake, Grid<float> const& gradient);
private:
	static float calcExternalEnergy(
		Grid<float> const& gradient, SnakeNode const& node, Vec2f const down_normal);

	static float calcElasticityEnergy(
		SnakeNode const& node1, SnakeNode const& node2, float avg_dist);

	static float calcBendingEnergy(
		SnakeNode const& node, SnakeNode const& prev_node, SnakeNode const& prev_prev_node);

	static float const m_elasticityWeight;
	static float const m_bendingWeight;
	static float const m_topExternalWeight;
	static float const m_bottomExternalWeight;
	float const m_factor;
	SnakeLength m_snakeLength;
	std::vector<FrenetFrame> m_frenetFrames;
};


TextLineRefiner::TextLineRefiner(
	GrayImage const& image, Dpi const& dpi,
	Vec2f const& unit_down_vector)
: m_image(image)
, m_dpi(dpi)
, m_unitDownVec(unit_down_vector)
{
}

void
TextLineRefiner::refine(
	std::list<std::vector<QPointF> >& polylines,
	int const iterations, DebugImages* dbg) const
{
	if (polylines.empty()) {
		return;
	}

	std::vector<Snake> snakes;
	snakes.reserve(polylines.size());
	
	// Convert from polylines to snakes.
	BOOST_FOREACH(std::vector<QPointF> const& polyline, polylines) {
		snakes.push_back(makeSnake(polyline, iterations));
	}

	if (dbg) {
		dbg->add(visualizeSnakes(snakes), "initial_snakes");
	}

	Grid<float> gradient(m_image.width(), m_image.height(), /*padding=*/0);

	// Start with a rather strong blur.
	float h_sigma = (4.0f / 200.f) * m_dpi.horizontal();
	float v_sigma = (4.0f / 200.f) * m_dpi.vertical();
	calcBlurredGradient(gradient, h_sigma, v_sigma);
	
	BOOST_FOREACH(Snake& snake, snakes) {
		evolveSnake(snake, gradient, ON_CONVERGENCE_STOP);
	}
	if (dbg) { 
		dbg->add(visualizeSnakes(snakes, &gradient), "evolved_snakes1");
	}

	// Less blurring this time.
	h_sigma *= 0.5f;
	v_sigma *= 0.5f;
	calcBlurredGradient(gradient, h_sigma, v_sigma);
	
	BOOST_FOREACH(Snake& snake, snakes) {
		evolveSnake(snake, gradient, ON_CONVERGENCE_GO_FINER);
	}
	if (dbg) { 
		dbg->add(visualizeSnakes(snakes, &gradient), "evolved_snakes2");
	}

	// Convert from snakes back to polylines.
	int i = -1;
	BOOST_FOREACH(std::vector<QPointF>& polyline, polylines) {
		++i;
		Snake const& snake = snakes[i];
		polyline.clear();
		BOOST_FOREACH(SnakeNode const& node, snake.nodes) {
			polyline.push_back(node.center);
		}
	}
}

void
TextLineRefiner::calcBlurredGradient(
	Grid<float>& gradient, float h_sigma, float v_sigma) const
{
	using namespace boost::lambda;

	float const downscale = 1.0f / (255.0f * 8.0f);
	Grid<float> vert_grad(m_image.width(), m_image.height(), /*padding=*/0);
	horizontalSobel<float>(
		m_image.width(), m_image.height(), m_image.data(), m_image.stride(), _1 * downscale,
		gradient.data(), gradient.stride(), _1 = _2, _1,
		gradient.data(), gradient.stride(), _1 = _2
	);
	verticalSobel<float>(
		m_image.width(), m_image.height(), m_image.data(), m_image.stride(), _1 * downscale,
		vert_grad.data(), vert_grad.stride(), _1 = _2, _1,
		gradient.data(), gradient.stride(),
		_1 = _1 * m_unitDownVec[0] + _2 * m_unitDownVec[1]
	);
	Grid<float>().swap(vert_grad); // Save memory.

	gaussBlurGeneric(
		m_image.size(), h_sigma, v_sigma,
		gradient.data(), gradient.stride(), _1,
		gradient.data(), gradient.stride(), _1 = _2
	);
}

float
TextLineRefiner::externalEnergyAt(
	Grid<float> const& gradient, Vec2f const& pos, float penalty_if_outside)
{
	float const x_base = floor(pos[0]);
	float const y_base = floor(pos[1]);
	int const x_base_i = (int)x_base;
	int const y_base_i = (int)y_base;

	if (x_base_i < 0 || y_base_i < 0 || x_base_i + 1 >= gradient.width() || y_base_i + 1 >= gradient.height()) {
		return penalty_if_outside;
	}

	float const x = pos[0] - x_base;
	float const y = pos[1] - y_base;
	float const x1 = 1.0f - x;
	float const y1 = 1.0f - y;

	int const stride = gradient.stride();
	float const* base = gradient.data() + y_base_i * stride + x_base_i;

	return base[0]*x1*y1 + base[1]*x*y1 + base[stride]*x1*y + base[stride + 1]*x*y;
}

TextLineRefiner::Snake
TextLineRefiner::makeSnake(std::vector<QPointF> const& polyline, int const iterations)
{
	float total_length = 0;

	size_t const polyline_size = polyline.size();
	for (size_t i = 1; i < polyline_size; ++i) {
		total_length += sqrt(Vec2f(polyline[i] - polyline[i - 1]).squaredNorm());
	}

	int const points_in_snake = total_length / 20;
	Snake snake;
	snake.iterationsRemaining = iterations;

	int points_inserted = 0;
	float base_t = 0;
	float next_insert_t = 0;
	for (size_t i = 1; i < polyline_size; ++i) {
		Vec2f const base(polyline[i - 1]);
		Vec2f const vec((polyline[i] - base));
		float const next_t = base_t + sqrt(vec.squaredNorm());
		
		while (next_t >= next_insert_t) {
			float const fraction = (next_insert_t - base_t) / (next_t - base_t);
			SnakeNode node;
			node.center = base + fraction * vec;
			node.ribHalfLength = 4;
			snake.nodes.push_back(node);
			++points_inserted;
			next_insert_t = total_length * points_inserted / (points_in_snake - 1);
		}

		base_t = next_t;
	}
	
	return snake;
}

void
TextLineRefiner::calcFrenetFrames(
	std::vector<FrenetFrame>& frenet_frames, Snake const& snake,
	SnakeLength const& snake_length, Vec2f const& unit_down_vec)
{
	size_t const num_nodes = snake.nodes.size();
	frenet_frames.resize(num_nodes);
	
	if (num_nodes == 0) {
		return;
	} else if (num_nodes == 1) {
		frenet_frames[0].unitTangent = Vec2f();
		frenet_frames[0].unitDownNormal = Vec2f();
		return;
	}

	// First segment.
	Vec2f first_segment(snake.nodes[1].center - snake.nodes[0].center);
	float const first_segment_len = snake_length.arcLengthAt(1);
	if (first_segment_len > std::numeric_limits<float>::epsilon()) {
		first_segment /= first_segment_len;
		frenet_frames.front().unitTangent = first_segment;
	}

	// Segments between first and last, exclusive.
	Vec2f prev_segment(first_segment);
	for (size_t i = 1; i < num_nodes - 1; ++i) {
		Vec2f next_segment(snake.nodes[i + 1].center - snake.nodes[i].center);
		float const next_segment_len = snake_length.lengthFromTo(i, i + 1);
		if (next_segment_len > std::numeric_limits<float>::epsilon()) {
			next_segment /= next_segment_len;
		}

		Vec2f tangent_vec(0.5 * (prev_segment + next_segment));
		float const len = sqrt(tangent_vec.squaredNorm());
		if (len > std::numeric_limits<float>::epsilon()) {
			tangent_vec /= len;
		}
		frenet_frames[i].unitTangent = tangent_vec;

		prev_segment = next_segment;
	}

	// Last segments.
	Vec2f last_segment(snake.nodes[num_nodes - 1].center - snake.nodes[num_nodes - 2].center);
	float const last_segment_len = snake_length.lengthFromTo(num_nodes - 2, num_nodes - 1);
	if (last_segment_len > std::numeric_limits<float>::epsilon()) {
		last_segment /= last_segment_len;
		frenet_frames.back().unitTangent = last_segment;
	}

	// Calculate normals and make sure they point down.
	BOOST_FOREACH(FrenetFrame& frame, frenet_frames) {
		frame.unitDownNormal = Vec2f(frame.unitTangent[1], -frame.unitTangent[0]);
		if (frame.unitDownNormal.dot(unit_down_vec) < 0) {
			frame.unitDownNormal = -frame.unitDownNormal;
		}
	}
}

void
TextLineRefiner::evolveSnake(Snake& snake, Grid<float> const& gradient,
							 OnConvergence const on_convergence) const
{
	float factor = 1.0f;

	while (snake.iterationsRemaining > 0) {
		--snake.iterationsRemaining;

		Optimizer optimizer(snake, m_unitDownVec, factor);
		bool changed = false;
		changed |= optimizer.thicknessAdjustment(snake, gradient);
		changed |= optimizer.tangentMovement(snake, gradient);
		changed |= optimizer.normalMovement(snake, gradient);

		if (!changed) {
			//qDebug() << "Converged.  Iterations remaining = " << snake.iterationsRemaining;
			if (on_convergence == ON_CONVERGENCE_STOP) {
				break;
			} else {
				factor *= 0.5f;
			}
		}
	}
}

QImage
TextLineRefiner::visualizeGradient(Grid<float> const& gradient) const
{
	int const width = gradient.width();
	int const height = gradient.height();
	int const gradient_stride = gradient.stride();

	// First let's find the maximum and minimum values.
	float min_value = NumericTraits<float>::max();
	float max_value = NumericTraits<float>::min();

	float const* gradient_line = gradient.data();
	for (int y = 0; y < height; ++y) {
		for (int x = 0; x < width; ++x) {
			float const value = gradient_line[x];
			if (value < min_value) {
				min_value = value;
			} else if (value > max_value) {
				max_value = value;
			}
		}
		gradient_line += gradient_stride;
	}

	float scale = std::max(max_value, -min_value);
	if (scale > std::numeric_limits<float>::epsilon()) {
		scale = 255.0f / scale;
	} 

	QImage overlay(width, height, QImage::Format_ARGB32_Premultiplied);
	uint32_t* overlay_line = (uint32_t*)overlay.bits();
	int const overlay_stride = overlay.bytesPerLine() / 4;

	gradient_line = gradient.data();
	for (int y = 0; y < height; ++y) {
		for (int x = 0; x < width; ++x) {
			float const value = gradient_line[x] * scale;
			int const magnitude = qBound(0, (int)(fabs(value) + 0.5), 255);
			if (value > 0) {
				// Red for positive gradients which indicate bottom edges.
				overlay_line[x] = qRgba(magnitude, 0, 0, magnitude);
			} else {
				overlay_line[x] = qRgba(0, 0, magnitude, magnitude);
			}
		}
		gradient_line += gradient_stride;
		overlay_line += overlay_stride;
	}

	QImage canvas(m_image.toQImage().convertToFormat(QImage::Format_ARGB32_Premultiplied));
	QPainter painter(&canvas);
	painter.drawImage(0, 0, overlay);

	return canvas;
}

QImage
TextLineRefiner::visualizeSnakes(std::vector<Snake> const& snakes, Grid<float> const* gradient) const
{
	QImage canvas;
	if (gradient) {
		canvas = visualizeGradient(*gradient);	
	} else {
		canvas = m_image.toQImage().convertToFormat(QImage::Format_ARGB32_Premultiplied);
	}

	QPainter painter(&canvas);
	painter.setRenderHint(QPainter::Antialiasing);
	
	QPen top_pen(QColor(0, 0, 255));
	top_pen.setWidthF(1.5);

	QPen bottom_pen(QColor(255, 0, 0));
	bottom_pen.setWidthF(1.5);
	
	QPen middle_pen(QColor(255, 0, 255));
	middle_pen.setWidth(1.5);

	QBrush knot_brush(QColor(255, 255, 0, 180));
	painter.setBrush(knot_brush);

	QRectF knot_rect(0, 0, 7, 7);
	std::vector<FrenetFrame> frenet_frames;

	BOOST_FOREACH(Snake const& snake, snakes) {
		SnakeLength const snake_length(snake);
		calcFrenetFrames(frenet_frames, snake, snake_length, m_unitDownVec);
		QVector<QPointF> top_polyline;
		QVector<QPointF> middle_polyline;
		QVector<QPointF> bottom_polyline;

		size_t const num_nodes = snake.nodes.size();
		for (size_t i = 0; i < num_nodes; ++i) {
			QPointF const mid(snake.nodes[i].center + QPointF(0.5, 0.5));
			QPointF const top(mid - snake.nodes[i].ribHalfLength * frenet_frames[i].unitDownNormal);
			QPointF const bottom(mid + snake.nodes[i].ribHalfLength * frenet_frames[i].unitDownNormal);
			top_polyline << top;
			middle_polyline << mid;
			bottom_polyline << bottom;
		}
		
		// Draw polylines.
		painter.setPen(top_pen);
		painter.drawPolyline(top_polyline);

		painter.setPen(bottom_pen);
		painter.drawPolyline(bottom_polyline);

		painter.setPen(middle_pen);
		painter.drawPolyline(middle_polyline);

		// Draw knots.
		painter.setPen(Qt::NoPen);
		BOOST_FOREACH(QPointF const& pt, middle_polyline) {
			knot_rect.moveCenter(pt);
			painter.drawEllipse(knot_rect);
		}
	}

	return canvas;
}


/*============================ SnakeLength =============================*/

TextLineRefiner::SnakeLength::SnakeLength(Snake const& snake)
: m_integralLength(snake.nodes.size())
, m_totalLength()
, m_rTotalLength()
, m_avgSegmentLength()
{
	size_t const num_nodes = snake.nodes.size();
	float arc_length_accum = 0;
	for (size_t i = 1; i < num_nodes; ++i) { 
		Vec2f const vec(snake.nodes[i].center - snake.nodes[i - 1].center);
		arc_length_accum += sqrt(vec.squaredNorm());
		m_integralLength[i] = arc_length_accum;
	}
	m_totalLength = arc_length_accum;
	if (m_totalLength > std::numeric_limits<float>::epsilon()) {
		m_rTotalLength = 1.0f / m_totalLength;
	}
	if (num_nodes > 1) {
		m_avgSegmentLength = m_totalLength / (num_nodes - 1);
	}
}


/*=========================== Optimizer =============================*/

float const TextLineRefiner::Optimizer::m_elasticityWeight = 0.2f;
float const TextLineRefiner::Optimizer::m_bendingWeight = 1.8f;
float const TextLineRefiner::Optimizer::m_topExternalWeight = 1.0f;
float const TextLineRefiner::Optimizer::m_bottomExternalWeight = 1.0f;

TextLineRefiner::Optimizer::Optimizer(
	Snake const& snake, Vec2f const& unit_down_vec, float factor)
: m_factor(factor)
, m_snakeLength(snake)
{
	calcFrenetFrames(m_frenetFrames, snake, m_snakeLength, unit_down_vec);
}

bool
TextLineRefiner::Optimizer::thicknessAdjustment(Snake& snake, Grid<float> const& gradient)
{
	size_t const num_nodes = snake.nodes.size();

	float const rib_adjustments[] = { 0.0f * m_factor, 0.5f * m_factor, -0.5f * m_factor };
	enum { NUM_RIB_ADJUSTMENTS = sizeof(rib_adjustments)/sizeof(rib_adjustments[0]) };

	int best_i = 0;
	int best_j = 0;
	float best_cost = NumericTraits<float>::max();
	for (int i = 0; i < NUM_RIB_ADJUSTMENTS; ++i) {
		float const head_rib = snake.nodes.front().ribHalfLength + rib_adjustments[i];
		if (head_rib <= std::numeric_limits<float>::epsilon()) {
			continue;
		}

		for (int j = 0; j < NUM_RIB_ADJUSTMENTS; ++j) {
			float const tail_rib = snake.nodes.back().ribHalfLength + rib_adjustments[j];
			if (tail_rib <= std::numeric_limits<float>::epsilon()) {
				continue;
			}
			
			float cost = 0;
			for (size_t node_idx = 0; node_idx < num_nodes; ++node_idx) {
				float const t = m_snakeLength.arcLengthFractionAt(node_idx);
				float const rib = head_rib + t * (tail_rib - head_rib);
				Vec2f const down_normal(m_frenetFrames[node_idx].unitDownNormal);

				SnakeNode node(snake.nodes[node_idx]);
				node.ribHalfLength = rib;
				cost += calcExternalEnergy(gradient, node, down_normal);
			}
			if (cost < best_cost) {
				best_cost = cost;
				best_i = i;
				best_j = j;
			}
		}
	}
	float const head_rib = snake.nodes.front().ribHalfLength + rib_adjustments[best_i];
	float const tail_rib = snake.nodes.back().ribHalfLength + rib_adjustments[best_j];
	for (size_t node_idx = 0; node_idx < num_nodes; ++node_idx) {
		float const t = m_snakeLength.arcLengthFractionAt(node_idx);
		snake.nodes[node_idx].ribHalfLength = head_rib + t * (tail_rib - head_rib);
		// Note that we need to recalculate inner ribs even if outer ribs
		// didn't change, as movement of ribs in tangent direction affects
		// interpolation.
	}
	
	return rib_adjustments[best_i] != 0 || rib_adjustments[best_j] != 0;
}

bool
TextLineRefiner::Optimizer::tangentMovement(Snake& snake, Grid<float> const& gradient)
{
	size_t const num_nodes = snake.nodes.size();
	if (num_nodes < 3) {
		return false;
	}

	float const tangent_movements[] = { 0.0f * m_factor, 1.0f * m_factor, -1.0f * m_factor };
	enum { NUM_TANGENT_MOVEMENTS = sizeof(tangent_movements)/sizeof(tangent_movements[0]) };

	std::vector<uint32_t> paths;
	std::vector<uint32_t> new_paths;
	std::vector<Step> step_storage;

	// Note that we don't move the first and the last node in tangent direction.
	paths.push_back(step_storage.size());
	step_storage.push_back(Step());
	step_storage.back().prevStepIdx = ~uint32_t(0);
	step_storage.back().node = snake.nodes.front();
	step_storage.back().pathCost = 0;

	for (size_t node_idx = 1; node_idx < num_nodes - 1; ++node_idx) {
		Vec2f const initial_pos(snake.nodes[node_idx].center);
		float const rib = snake.nodes[node_idx].ribHalfLength;
		Vec2f const unit_tangent(m_frenetFrames[node_idx].unitTangent);
		Vec2f const down_normal(m_frenetFrames[node_idx].unitDownNormal);

		for (int i = 0; i < NUM_TANGENT_MOVEMENTS; ++i) {
			Step step;
			step.prevStepIdx = ~uint32_t(0);
			step.node.center = initial_pos + tangent_movements[i] * unit_tangent;
			step.node.ribHalfLength = rib;
			step.pathCost = NumericTraits<float>::max();

			float base_cost = calcExternalEnergy(gradient, step.node, down_normal);

			if (node_idx == num_nodes - 2) {
				// Take into account the distance to the last node as well.
				base_cost += calcElasticityEnergy(
					step.node, snake.nodes.back(), m_snakeLength.avgSegmentLength()
				);
			}

			// Now find the best step for the previous node to combine with.
			BOOST_FOREACH(uint32_t prev_step_idx, paths) {
				Step const& prev_step = step_storage[prev_step_idx];
				float const cost = base_cost + prev_step.pathCost +
					calcElasticityEnergy(step.node, prev_step.node, m_snakeLength.avgSegmentLength());

				if (cost < step.pathCost) {
					step.pathCost = cost;
					step.prevStepIdx = prev_step_idx;
				}
			}
			assert(step.prevStepIdx != ~uint32_t(0));
			new_paths.push_back(step_storage.size());
			step_storage.push_back(step);
		}
		assert(!new_paths.empty());
		paths.swap(new_paths);
		new_paths.clear();
	}
	
	// Find the best overall path.
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
	float max_sqdist = 0;
	uint32_t step_idx = best_path_idx;
	for (int node_idx = num_nodes - 2; node_idx > 0; --node_idx) {
		assert(step_idx != ~uint32_t(0));
		Step const& step = step_storage[step_idx];
		SnakeNode& node = snake.nodes[node_idx];
		
		float const sqdist = (node.center - step.node.center).squaredNorm();
		max_sqdist = std::max<float>(max_sqdist, sqdist);

		node = step.node;
		step_idx = step.prevStepIdx;
	}

	return max_sqdist > std::numeric_limits<float>::epsilon();
}

bool
TextLineRefiner::Optimizer::normalMovement(Snake& snake, Grid<float> const& gradient)
{
	size_t const num_nodes = snake.nodes.size();
	if (num_nodes < 3) {
		return false;
	}

	float const normal_movements[] = { 0.0f * m_factor, 1.0f * m_factor, -1.0f * m_factor };
	enum { NUM_NORMAL_MOVEMENTS = sizeof(normal_movements)/sizeof(normal_movements[0]) };

	std::vector<uint32_t> paths;
	std::vector<uint32_t> new_paths;
	std::vector<Step> step_storage;

	// The first two nodes pose a problem for us.  These nodes don't have two predecessors,
	// and therefore we can't take bending into the account.  We could take the followers
	// instead of the ancestors, but then this follower is going to move itself, making
	// our calculations less accurate.  The proper solution is to provide not N but N*N
	// paths to the 3rd node, each path corresponding to a combination of movement of
	// the first and the second node.  That's the approach we are taking here.
	for (int i = 0; i < NUM_NORMAL_MOVEMENTS; ++i) {
		uint32_t const prev_step_idx = step_storage.size();
		{
			// Movements of the first node.
			Vec2f const down_normal(m_frenetFrames[0].unitDownNormal);
			Step step;
			step.node.center = snake.nodes[0].center + normal_movements[i] * down_normal;
			step.node.ribHalfLength = snake.nodes[0].ribHalfLength;
			step.prevStepIdx = ~uint32_t(0);
			step.pathCost = calcExternalEnergy(gradient, step.node, down_normal);

			step_storage.push_back(step);
		}
		
		for (int j = 0; j < NUM_NORMAL_MOVEMENTS; ++j) {
			// Movements of the second node.
			Vec2f const down_normal(m_frenetFrames[1].unitDownNormal);
			
			Step step;
			step.node.center = snake.nodes[1].center + normal_movements[j] * down_normal;
			step.node.ribHalfLength = snake.nodes[1].ribHalfLength;
			step.prevStepIdx = prev_step_idx;
			step.pathCost = step_storage[prev_step_idx].pathCost +
				calcExternalEnergy(gradient, step.node, down_normal);

			paths.push_back(step_storage.size());
			step_storage.push_back(step);
		}
	}

	for (size_t node_idx = 2; node_idx < num_nodes; ++node_idx) {
		SnakeNode const& node = snake.nodes[node_idx];
		Vec2f const down_normal(m_frenetFrames[node_idx].unitDownNormal);

		for (int i = 0; i < NUM_NORMAL_MOVEMENTS; ++i) {
			Step step;
			step.prevStepIdx = ~uint32_t(0);
			step.node.center = node.center + normal_movements[i] * down_normal;
			step.node.ribHalfLength = node.ribHalfLength;
			step.pathCost = NumericTraits<float>::max();

			float const base_cost = calcExternalEnergy(gradient, step.node, down_normal);

			// Now find the best step for the previous node to combine with.
			BOOST_FOREACH(uint32_t prev_step_idx, paths) {
				Step const& prev_step = step_storage[prev_step_idx];
				Step const& prev_prev_step = step_storage[prev_step.prevStepIdx];

				float const cost = base_cost + prev_step.pathCost +
					calcBendingEnergy(step.node, prev_step.node, prev_prev_step.node);

				if (cost < step.pathCost) {
					step.pathCost = cost;
					step.prevStepIdx = prev_step_idx;
				}
			}
			assert(step.prevStepIdx != ~uint32_t(0));
			new_paths.push_back(step_storage.size());
			step_storage.push_back(step);
		}
		assert(!new_paths.empty());
		paths.swap(new_paths);
		new_paths.clear();
	}
	
	// Find the best overall path.
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
	float max_sqdist = 0;
	uint32_t step_idx = best_path_idx;
	for (int node_idx = num_nodes - 1; node_idx >= 0; --node_idx) {
		assert(step_idx != ~uint32_t(0));
		Step const& step = step_storage[step_idx];
		SnakeNode& node = snake.nodes[node_idx];
		
		float const sqdist = (node.center - step.node.center).squaredNorm();
		max_sqdist = std::max<float>(max_sqdist, sqdist);

		node = step.node;
		step_idx = step.prevStepIdx;
	}

	return max_sqdist > std::numeric_limits<float>::epsilon();
}

float
TextLineRefiner::Optimizer::calcExternalEnergy(
	Grid<float> const& gradient, SnakeNode const& node, Vec2f const down_normal)
{
	Vec2f const top(node.center - node.ribHalfLength * down_normal);
	Vec2f const bottom(node.center + node.ribHalfLength * down_normal);

	float const top_grad = externalEnergyAt(gradient, top, 0.0f);
	float const bottom_grad = externalEnergyAt(gradient, bottom, 0.0f);

	// Surprisingly, it turns out it's a bad idea to penalize for the opposite
	// sign in the gradient.  Sometimes a snake's edge has to move over the
	// "wrong" gradient ridge before it gets into a good position.
	// Those std::min and std::max prevent such penalties.
	float const top_energy = m_topExternalWeight * std::min<float>(top_grad, 0.0f);
	float const bottom_energy = m_bottomExternalWeight * std::max<float>(bottom_grad, 0.0f);

	// Positive gradient indicates the bottom edge and vice versa.
	// Note that negative energies are fine with us - the less the better.
	return top_energy - bottom_energy;
}

float
TextLineRefiner::Optimizer::calcElasticityEnergy(
	SnakeNode const& node1, SnakeNode const& node2, float avg_dist)
{
	Vec2f const vec(node1.center - node2.center);
	float const vec_len = sqrt(vec.squaredNorm());

	if (vec_len < 1.0f) {
		return 1000.0f; // Penalty for moving too close to another node.
	}

	float const dist_diff = fabs(avg_dist - vec_len);
	return m_elasticityWeight * (dist_diff / avg_dist);
}

float
TextLineRefiner::Optimizer::calcBendingEnergy(
	SnakeNode const& node, SnakeNode const& prev_node, SnakeNode const& prev_prev_node)
{
	Vec2f const vec(node.center - prev_node.center);
	float const vec_len = sqrt(vec.squaredNorm());

	if (vec_len < 1.0f) {
		return 1000.0f; // Penalty for moving too close to another node.
	}

	Vec2f const prev_vec(prev_node.center - prev_prev_node.center);
	float const prev_vec_len = sqrt(prev_vec.squaredNorm());
	if (prev_vec_len < 1.0f) {
		return 1000.0f; // Penalty for moving too close to another node.
	}

	Vec2f const bend_vec(vec / vec_len - prev_vec / prev_vec_len);
	return m_bendingWeight * bend_vec.squaredNorm();
}

} // namespace dewarping
