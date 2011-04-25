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
#include "ValueConv.h"
#include "imageproc/GrayImage.h"
#include "imageproc/GaussBlur.h"
#include <boost/scoped_array.hpp>
#include <boost/foreach.hpp>
#include <boost/lambda/lambda.hpp>
#include <boost/lambda/bind.hpp>
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

TextLineRefiner::TextLineRefiner(
	GrayImage const& image, Dpi const& dpi, DebugImages* dbg)
:	m_gradient(image.width(), image.height(), /*padding=*/1)
{
	using namespace boost::lambda;

	verticalSobel(image, m_gradient);
	if (dbg) {
		dbg->add(visualizeGradient(m_gradient, &image.toQImage()), "vertical_gradient");
	}

	float const h_sigma = (4.0f / 200.f) * dpi.horizontal();
	float const v_sigma = (4.0f / 200.f) * dpi.vertical();
	gaussBlurGeneric(
		image.size(), h_sigma, v_sigma,
		m_gradient.data(), m_gradient.stride(), _1,
		m_gradient.data(), m_gradient.stride(), _1 = _2
	);
	if (dbg) {
		dbg->add(visualizeGradient(m_gradient, &image.toQImage()), "smoothed_gradient");
	}
}

void
TextLineRefiner::refine(
	std::list<std::vector<QPointF> >& polylines, int const iterations,
	DebugImages* dbg, QImage const* dbg_background)
{
	if (polylines.empty()) {
		return;
	}

	std::vector<Snake> snakes;
	snakes.reserve(polylines.size());
	
	// Convert from polylines to snakes.
	BOOST_FOREACH(std::vector<QPointF> const& polyline, polylines) {
		snakes.push_back(makeSnake(polyline));
	}

	if (dbg) {
		dbg->add(visualizeSnakes(snakes, dbg_background), "initial_snakes");
	}

	for (int i = 0; i < iterations; ++i) {				
		BOOST_FOREACH(Snake& snake, snakes) {
			evolveSnake(snake);
		}
	}

	if (dbg) { 
		QImage gradient_image(visualizeGradient(m_gradient, dbg_background));
		dbg->add(visualizeSnakes(snakes, &gradient_image), "evolved_snakes");
	}

	// Convert from snakes back to polylines.
	int i = -1;
	BOOST_FOREACH(std::vector<QPointF>& polyline, polylines) {
		++i;
		Snake const& snake = snakes[i];
		polyline.clear();
		BOOST_FOREACH(SnakeNode const& node, snake) {
			polyline.push_back(node.center);
		}
	}
}

void
TextLineRefiner::verticalSobel(imageproc::GrayImage const& src, Grid<float>& dst)
{
	assert(dst.padding() == 1);

	int const width = src.width();
	int const height = src.height();

	int const src_stride = src.stride();
	int const dst_stride = dst.stride();

	uint8_t const* src_line = src.data();
	float* dst_line = dst.data();

	float const scale = 1.0f / 255.0f;

	// Copy src to dst.
	for (int y = 0; y < height; ++y) {
		for (int x = 0; x < width; ++x) {
			dst_line[x] = scale * src_line[x];
		}
		src_line += src_stride;
		dst_line += dst_stride;
	}

	// Write border corners.
	dst_line = dst.paddedData();
	dst_line[0] = dst_line[dst_stride + 1];
	dst_line[dst_stride - 1] = dst_line[dst_stride * 2 - 2];
	dst_line += dst_stride * (height + 1);
	dst_line[0] = dst_line[1 - dst_stride];
	dst_line[dst_stride - 1] = dst_line[-2];

	// Top border line.
	dst_line = dst.paddedData() + 1;
	for (int x = 0; x < width; ++x) {
		dst_line[0] = dst_line[dst_stride];
		++dst_line;
	}

	// Bottom border line.
	dst_line = dst.paddedData() + dst_stride * (height + 1) + 1;
	for (int x = 0; x < width; ++x) {
		dst_line[0] = dst_line[-dst_stride];
		++dst_line;
	}

	// Left and right border lines.
	dst_line = dst.paddedData() + dst_stride;
	for (int y = 0; y < height; ++y) {
		dst_line[0] = dst_line[1];
		dst_line[dst_stride - 1] = dst_line[dst_stride - 2];
		dst_line += dst_stride;
	}

	// Do a horizontal pass.
	dst_line = dst.paddedData() + 1;
	for (int y = 0; y < height + 2; ++y) {
		float prev = dst_line[-1];
		for (int x = 0; x < width; ++x) {
			float cur = dst_line[x];
			dst_line[x] = prev + cur + cur + dst_line[x + 1];
			prev = cur;
		}
		dst_line += dst_stride;
	}

	// Do a vertical pass and write resuts.
	for (int x = 0; x < width; ++x) {
		float* p_dst = dst.data() + x;
		float prev = p_dst[-dst_stride];
		for (int y = 0; y < height; ++y) {
			float const cur = *p_dst;
			*p_dst = p_dst[dst_stride] - prev;
			prev = cur;
			p_dst += dst_stride;
		}
	}
}

float
TextLineRefiner::externalEnergyAt(Vec2f const& pos, float penalty_if_outside) const
{
	float const x_base = floor(pos[0]);
	float const y_base = floor(pos[1]);
	int const x_base_i = (int)x_base;
	int const y_base_i = (int)y_base;

	if (x_base_i < 0 || y_base_i < 0 || x_base_i + 1 >= m_gradient.width() || y_base_i + 1 >= m_gradient.height()) {
		return penalty_if_outside;
	}

	float const x = pos[0] - x_base;
	float const y = pos[1] - y_base;
	float const x1 = 1.0f - x;
	float const y1 = 1.0f - y;

	int const stride = m_gradient.stride();
	float const* base = m_gradient.data() + y_base_i * stride + x_base_i;

	return base[0]*x1*y1 + base[1]*x*y1 + base[stride]*x1*y + base[stride + 1]*x*y;
}

TextLineRefiner::Snake
TextLineRefiner::makeSnake(std::vector<QPointF> const& polyline)
{
	// TODO: first make sure polyline doesn't contain zero length segments.

	int const points_in_snake = 30;

	float total_length = 0;

	size_t const polyline_size = polyline.size();
	for (size_t i = 1; i < polyline_size; ++i) {
		total_length += sqrt(Vec2f(polyline[i] - polyline[i - 1]).squaredNorm());
	}

	Snake snake;

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
			snake.push_back(node);
			++points_inserted;
			next_insert_t = total_length * points_inserted / (points_in_snake - 1);
		}

		base_t = next_t;
	}
	
	return snake;
}

void
TextLineRefiner::calcUpwardPointingNormals(Snake const& snake, std::vector<Vec2f>& normals)
{
	size_t const num_nodes = snake.size();
	normals.resize(num_nodes);
	
	if (num_nodes == 0) {
		return;
	} else if (num_nodes == 1) {
		normals.front() = Vec2f(0, 0);
		return;
	}


	Vec2f const first_segment(snake[1].center - snake[0].center);
	Vec2f const last_segment(snake[num_nodes - 1].center - snake[num_nodes - 2].center);

	normals.front() = Vec2f(-first_segment[1], first_segment[0]);
	normals.back() = Vec2f(-last_segment[1], last_segment[0]);
	
	Vec2f prev_segment(first_segment);
	float const prev_segment_len = sqrt(first_segment.squaredNorm());
	if (prev_segment_len > std::numeric_limits<float>::epsilon()) {
		prev_segment /= prev_segment_len;
	}

	for (size_t i = 1; i < num_nodes - 1; ++i) {
		Vec2f next_segment(snake[i + 1].center - snake[i].center);
		float const next_segment_len = sqrt(next_segment.squaredNorm());
		if (next_segment_len > std::numeric_limits<float>::epsilon()) {
			next_segment /= next_segment_len;
		}

		Vec2f const avg_direction(0.5 * (prev_segment + next_segment));
		normals[i] = Vec2f(-avg_direction[1], avg_direction[0]);

		prev_segment = next_segment;
	}

	// Normalize the lengths of normals and make sure they point upward.
	BOOST_FOREACH(Vec2f& normal, normals) {
		float const len = sqrt(normal.squaredNorm());
		if (len > std::numeric_limits<float>::epsilon()) {
			normal /= len;
		}
		if (normal[1] > 0) {
			normal = -normal;
		}
	}
}

namespace
{

struct Offset
{
	float dx;
	float dy;

	operator Vec2f() const { return Vec2f(dx, dy); }
};

}

void
TextLineRefiner::evolveSnake(Snake& snake) const
{
	size_t const num_nodes = snake.size();
	if (num_nodes <= 1) {
		return;
	}

	std::vector<Step> step_storage;

	static Offset const offsets[] = {
		{  0,  0 },
		{ -1, -1 },
		{  0, -1 },
		{  1, -1 },
		{ -1,  0 },
		{  1,  0 },
		{ -1,  1 },
		{  0,  1 },
		{  1,  1 }
	};

	static float const rib_length_adjusters[] = { 0, 0.5, -0.5 };

	float const elasticity_weight = 0.3f;
	float const bending_weight = 6.0f;
	float const thickness_weight = 0.8f;
	float const top_external_weight = 0.3f;
	float const bottom_external_weight = 0.8f;
	// By setting bottom weight higher than top weight, we are trying
	// to handle words in capitals and long numbers.

	float const segment_dist_threshold = 1;

	float avg_dist = 0;
	for (size_t i = 1; i < num_nodes; ++i) {
		Vec2f const vec(snake[i].center - snake[i - 1].center);
		avg_dist += sqrt(vec.squaredNorm());
	}
	avg_dist /= num_nodes - 1;

	std::vector<Vec2f> normals;
	calcUpwardPointingNormals(snake, normals);

	std::vector<uint32_t> paths;
	std::vector<uint32_t> new_paths;
	
	for (size_t node_idx = 0; node_idx < num_nodes; ++node_idx) {
		SnakeNode const& node = snake[node_idx];
		BOOST_FOREACH(float const rib_length_adjuster, rib_length_adjusters) {
			BOOST_FOREACH(Vec2f offset, offsets) {
				Step step;
				step.prevStepIdx = ~uint32_t(0);
				step.node.center = node.center + offset;
				step.node.ribHalfLength = node.ribHalfLength + rib_length_adjuster;
				step.pathCost = 0;

				if (step.node.ribHalfLength < 0) {
					step.pathCost -= 100 * step.node.ribHalfLength;
				}

				Vec2f const top_point(step.node.center + step.node.ribHalfLength * normals[node_idx]);
				Vec2f const bottom_point(step.node.center - step.node.ribHalfLength * normals[node_idx]);

				// Positive gradient indicates bottom edge and vice versa.
				// Note that negative costs are fine with us - the less the better.
				step.pathCost += top_external_weight * externalEnergyAt(top_point, 1000);
				step.pathCost -= bottom_external_weight * externalEnergyAt(bottom_point, -1000);

				float best_cost = NumericTraits<float>::max();
				uint32_t best_prev_step_idx = step.prevStepIdx;

				BOOST_FOREACH(uint32_t prev_step_idx, paths) {
					Step const& prev_step = step_storage[prev_step_idx];
					float cost = prev_step.pathCost + step.pathCost;

					Vec2f const vec(step.node.center - prev_step.node.center);
					float const vec_len = sqrt(vec.squaredNorm());
					if (vec_len < segment_dist_threshold) {
						cost += 1000;
					}

					// Elasticity.
					float const dist_diff = fabs(avg_dist - vec_len);
					cost += elasticity_weight * (dist_diff / avg_dist);

					// Thickness.
					float const thickness_diff = fabs(step.node.ribHalfLength - prev_step.node.ribHalfLength);
					cost += thickness_weight * (thickness_diff / prev_step.node.ribHalfLength);

					if (prev_step.prevStepIdx != ~uint32_t(0) && vec_len >= segment_dist_threshold) {
						Step const& prev_prev_step = step_storage[prev_step.prevStepIdx];
						Vec2f prev_normal(prev_step.node.center - prev_prev_step.node.center);
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
		snake.push_back(step.node);	
		step_idx = step.prevStepIdx;
	}
	assert(num_nodes == snake.size());
}

QImage
TextLineRefiner::visualizeGradient(Grid<float> const& gradient, QImage const* background)
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
TextLineRefiner::visualizeSnakes(std::vector<Snake> const& snakes, QImage const* background) const
{
	QImage canvas;
	if (background) {
		canvas = background->convertToFormat(QImage::Format_ARGB32_Premultiplied);
	} else {
		canvas = QImage(m_gradient.width(), m_gradient.height(), QImage::Format_ARGB32_Premultiplied);
		canvas.fill(0xffffffff); // Opaque white.
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

	BOOST_FOREACH(Snake const& snake, snakes) {
		std::vector<Vec2f> normals;
		calcUpwardPointingNormals(snake, normals);
		QVector<QPointF> top_polyline;
		QVector<QPointF> middle_polyline;
		QVector<QPointF> bottom_polyline;

		size_t const num_nodes = snake.size();
		for (size_t i = 0; i < num_nodes; ++i) {
			QPointF const mid(snake[i].center + QPointF(0.5, 0.5));
			QPointF const top(mid + snake[i].ribHalfLength * normals[i]);
			QPointF const bottom(mid - snake[i].ribHalfLength * normals[i]);
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

} // namespace dewarping
