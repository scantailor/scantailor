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

#include "DetectVertContentBounds.h"
#include "DebugImages.h"
#include "VecNT.h"
#include "SidesOfLine.h"
#include "imageproc/BinaryImage.h"
#include "imageproc/Constants.h"
#include <QImage>
#include <QPoint>
#include <QPointF>
#include <QRectF>
#include <QLine>
#include <QPainter>
#include <QPen>
#include <QColor>
#include <Qt>
#include <QtGlobal>
#include <boost/foreach.hpp>
#include <boost/lambda/lambda.hpp>
#include <boost/lambda/bind.hpp>
#include <vector>
#include <deque>
#include <algorithm>
#include <math.h>
#include <assert.h>

using namespace imageproc;

namespace
{

struct VertRange
{
	int top;
	int bottom;

	VertRange() : top(-1), bottom(-1) {}

	VertRange(int t, int b) : top(t), bottom(b) {}

	bool isValid() const { return top != -1; }
};


struct Segment
{
	QLine line;
	Vec2d unitVec;
	int vertDist;

	bool distToVertLine(int vert_line_x) const {
		return std::min<int>(abs(line.p1().x() - vert_line_x), abs(line.p2().x() - vert_line_x));
	}

	Segment(QLine const& line, Vec2d const& vec, int dist)
		: line(line), unitVec(vec), vertDist(dist) {}
};


struct RansacModel
{
	std::vector<Segment> segments;
	int totalVertDist; // Sum of individual Segment::vertDist

	RansacModel() : totalVertDist(0) {}

	void add(Segment const& seg) {
		segments.push_back(seg);
		totalVertDist += seg.vertDist;
	}

	bool betterThan(RansacModel const& other) const {
		return totalVertDist > other.totalVertDist;
	}

	void swap(RansacModel& other) {
		segments.swap(other.segments);
		std::swap(totalVertDist, other.totalVertDist);
	}
};


class RansacAlgo
{
public:
	RansacAlgo(std::vector<Segment> const& segments);

	void buildAndAssessModel(Segment const& seed_segment);

	RansacModel& bestModel() { return m_bestModel; }

	RansacModel const& bestModel() const { return m_bestModel; }
private:
	std::vector<Segment> const& m_rSegments;
	RansacModel m_bestModel;
	double m_cosThreshold;
};


class SequentialColumnProcessor
{
public:
	enum LeftOrRight { LEFT, RIGHT };

	SequentialColumnProcessor(LeftOrRight left_or_right);

	void process(int x, VertRange const& range);

	QLineF approximateWithLine(std::vector<Segment>* dbg_segments = 0) const;

	QImage visualizeEnvelope(QImage const& background);
private:
	bool midPointShouldGo(QPoint top, QPoint mid, QPoint bottom, QPoint leader) const;

	QLineF interpolateSegments(std::vector<Segment> const& segments) const;

	// top and bottom on the leftmost or the rightmost line.
	QPoint m_leadingTop;
	QPoint m_leadingBottom;
	std::deque<QPoint> m_path; // Top to bottom.
	LeftOrRight m_leftOrRight;
};


RansacAlgo::RansacAlgo(std::vector<Segment> const& segments)
:	m_rSegments(segments),
	m_cosThreshold(cos(4.0 * constants::DEG2RAD))
{
}

void
RansacAlgo::buildAndAssessModel(Segment const& seed_segment)
{
	RansacModel cur_model;
	cur_model.add(seed_segment);
	
	BOOST_FOREACH(Segment const& seg, m_rSegments) {
		double const cos = seg.unitVec.dot(seed_segment.unitVec);
		if (cos > m_cosThreshold) {
			cur_model.add(seg);
		}
	}

	if (cur_model.betterThan(m_bestModel)) {
		cur_model.swap(m_bestModel);
	}
}


SequentialColumnProcessor::SequentialColumnProcessor(LeftOrRight left_or_right)
:	m_leftOrRight(left_or_right)
{
}

void
SequentialColumnProcessor::process(int x, VertRange const& range)
{
	if (!range.isValid()) {
		return;
	}

	// If defined, we build something like a convex hull, but just from one side.
	// Our current choice is not to, as it's bad for cases when the title or some
	// other decorative element is the widest element on the page and doesn't have
	// a counterpart on the other side (top / bottom).
#define ENFORCE_CONVEXITY 0

	if (m_path.empty()) {
		m_leadingTop = QPoint(x, range.top);
		m_leadingBottom = QPoint(x, range.bottom);
		m_path.push_front(m_leadingTop);

		if (range.top != range.bottom) { // We don't want zero length segments in m_path.
			m_path.push_back(m_leadingBottom);
		}
	} else {
		if (range.top < m_path.front().y()) {
			QPoint const new_pt(x, range.top);
#if ENFORCE_CONVEXITY
			while (m_path.size() > 1 && midPointShouldGo(new_pt, m_path.front(), m_path[1], m_leadingTop)) {
				m_path.pop_front();
			}
#endif
			m_path.push_front(new_pt);
		}

		if (range.bottom > m_path.back().y()) {
			QPoint const new_pt(x, range.bottom);
#if ENFORCE_CONVEXITY
			while (m_path.size() > 1 && midPointShouldGo(m_path[m_path.size() - 2], m_path.back(), new_pt, m_leadingBottom)) {
				m_path.pop_back();
			}
#endif
			m_path.push_back(new_pt);
		}
	}
}

bool
SequentialColumnProcessor::midPointShouldGo(
	QPoint top, QPoint mid, QPoint bottom, QPoint leader) const
{
	if (mid == leader) {
		// The leader can't go.
		return false;
	}

	QPoint upper_normal(top - mid);
	std::swap(upper_normal.rx(), upper_normal.ry());
	if (m_leftOrRight == LEFT) {
		upper_normal.setX(-upper_normal.x());
	} else {
		upper_normal.setY(-upper_normal.y());
	}
	// upper_normal now points *inside* the image, towards the other bound.

	QPoint down_vec(bottom - mid);
	int const dot = upper_normal.x() * down_vec.x() + upper_normal.y() * down_vec.y();

	return dot <= 0;
}

QLineF
SequentialColumnProcessor::approximateWithLine(std::vector<Segment>* dbg_segments) const
{
	using namespace boost::lambda;

	size_t const num_points = m_path.size();

	std::vector<Segment> segments;
	segments.reserve(num_points);
	
	// Collect line segments from m_path and convert them to unit vectors.
	for (size_t i = 1; i < num_points; ++i) {
		QPoint const pt1(m_path[i - 1]);
		QPoint const pt2(m_path[i]);
		assert(pt2.y() > pt1.y());
		
		Vec2d vec(pt2 - pt1);
		if (fabs(vec[0]) > fabs(vec[1])) {
			// We don't want segments that are more horizontal than vertical.
			continue;
		}

		vec /= sqrt(vec.squaredNorm());
		segments.push_back(Segment(QLine(pt1, pt2), vec, pt2.y() - pt1.y()));
	}
	

	// Run RANSAC on the segments.
	
	RansacAlgo ransac(segments);
	qsrand(0); // Repeatablity is important.

	// We want to make sure we do pick a few segments closest
	// to the edge, so let's sort segments appropriately
	// and manually feed the best ones to RANSAC.
	size_t const num_best_segments = std::min<size_t>(6, segments.size());
	std::partial_sort(
		segments.begin(), segments.begin() + num_best_segments, segments.end(),
		bind(&Segment::distToVertLine, _1, m_leadingTop.x()) <
		bind(&Segment::distToVertLine, _2, m_leadingTop.x())
	);
	for (size_t i = 0; i < num_best_segments; ++i) {
		ransac.buildAndAssessModel(segments[i]);
	}

	// Continue with random samples.
	int const ransac_iterations = segments.empty() ? 0 : 200;
	for (int i = 0; i < ransac_iterations; ++i) {
		ransac.buildAndAssessModel(segments[qrand() % segments.size()]);
	}

	if (ransac.bestModel().segments.empty()) {
		return QLineF(m_leadingTop, m_leadingTop + QPointF(0, 1));
	}
	
	QLineF const line(interpolateSegments(ransac.bestModel().segments));
	
	if (dbg_segments) {
		// Has to be the last thing we do with best model.
		dbg_segments->swap(ransac.bestModel().segments);
	}

	return line;
}

QLineF
SequentialColumnProcessor::interpolateSegments(std::vector<Segment> const& segments) const
{
	assert(!segments.empty());

	// First, interpolate the angle of segments.
	Vec2d accum_vec;
	double accum_weight = 0;

	BOOST_FOREACH(Segment const& seg, segments) {
		double const weight = sqrt(double(seg.vertDist));
		accum_vec += weight * seg.unitVec;
		accum_weight += weight;
	}

	assert(accum_weight != 0);
	accum_vec /= accum_weight;

	QPoint reference_pt;
	int const best_x = m_leadingTop.x();
	
	// Now find the vertex in m_path through which our line should pass.
	if (m_path.size() < 3) {
		reference_pt = (accum_vec[0] > 0) == (m_leftOrRight == RIGHT) ? m_leadingTop : m_leadingBottom;
	} else {
		// Initialize reference_pt with either front or back of m_path, whichever is farther from best_x.
		if (abs(m_path.front().x() - best_x) > abs(m_path.back().x() - best_x)) {
			reference_pt = m_path.front();
		} else {
			reference_pt = m_path.back();
		}
		std::deque<QPoint>::const_iterator prev_it(m_path.begin());
		std::deque<QPoint>::const_iterator cur_it(prev_it);
		++cur_it;
		std::deque<QPoint>::const_iterator next_it(cur_it);
		++next_it;
		std::deque<QPoint>::const_iterator const end(m_path.end());
		for (; next_it != end; prev_it = cur_it, cur_it = next_it, ++next_it) {
			QLineF const line(*cur_it, QPointF(*cur_it) + accum_vec);
			if (sidesOfLine(line, *prev_it, *next_it) >= 0) {
				if (abs(cur_it->x() - best_x) < abs(reference_pt.x() - best_x)) {
					reference_pt = *cur_it;
				}
			}
		}
	}

	return QLineF(reference_pt, QPointF(reference_pt) + accum_vec);
}

QImage
SequentialColumnProcessor::visualizeEnvelope(QImage const& background)
{
	QImage canvas(background.convertToFormat(QImage::Format_RGB32));
	QPainter painter(&canvas);
	painter.setRenderHint(QPainter::Antialiasing);
	
	QPen pen(QColor(0xff, 0, 0, 180));
	pen.setWidthF(3.0);
	painter.setPen(pen);

	if (!m_path.empty()) {
		std::vector<QPointF> const polyline(m_path.begin(), m_path.end());
		painter.drawPolyline(&polyline[0], polyline.size());
	}

	painter.setPen(Qt::NoPen);
	painter.setBrush(QColor(Qt::blue));
	painter.setOpacity(0.7);
	QRectF rect(0, 0, 9, 9);

	BOOST_FOREACH(QPoint pt, m_path) {
		rect.moveCenter(pt + QPointF(0.5, 0.5));
		painter.drawEllipse(rect);
	}

	return canvas;
}

QImage visualizeSegments(QImage const& background, std::vector<Segment> const& segments)
{
	QImage canvas(background.convertToFormat(QImage::Format_RGB32));
	QPainter painter(&canvas);
	painter.setRenderHint(QPainter::Antialiasing);

	QPen pen(Qt::red);
	pen.setWidthF(3.0);
	painter.setPen(pen);
	painter.setOpacity(0.7);

	BOOST_FOREACH(Segment const& seg, segments) {
		painter.drawLine(seg.line);
	}

	return canvas;
}

// For every column in the image, store the top-most and bottom-most black pixel.
void calculateVertRanges(imageproc::BinaryImage const& image, std::vector<VertRange>& ranges)
{
	int const width = image.width();
	int const height = image.height();
	uint32_t const* image_data = image.data();
	int const image_stride = image.wordsPerLine();
	uint32_t const msb = uint32_t(1) << 31;

	ranges.reserve(width);

	for (int x = 0; x < width; ++x) {
		ranges.push_back(VertRange());
		VertRange& range = ranges.back();
		
		uint32_t const mask = msb >> (x & 31);
		uint32_t const* p_word = image_data + (x >> 5);

		int top_y = 0;
		for (; top_y < height; ++top_y, p_word += image_stride) {
			if (*p_word & mask) {
				range.top = top_y;
				break;
			}
		}

		int bottom_y = height - 1;
		p_word = image_data + bottom_y * image_stride + (x >> 5);
		for (; bottom_y >= top_y; --bottom_y, p_word -= image_stride) {
			if (*p_word & mask) {
				range.bottom = bottom_y;
				break;
			}
		}
	}
}

QLineF extendLine(QLineF const& line, int height)
{
	QPointF top_intersection;
	QPointF bottom_intersection;
	
	QLineF const top_line(QPointF(0, 0), QPointF(1, 0));
	QLineF const bottom_line(QPointF(0, height), QPointF(1, height));
	
	line.intersect(top_line, &top_intersection);
	line.intersect(bottom_line, &bottom_intersection);

	return QLineF(top_intersection, bottom_intersection);
}

} // anonymous namespace


std::pair<QLineF, QLineF> detectVertContentBounds(
	imageproc::BinaryImage const& image, DebugImages* dbg)
{
	int const width = image.width();
	int const height = image.height();

	std::vector<VertRange> cols;
	calculateVertRanges(image, cols);

	SequentialColumnProcessor left_processor(SequentialColumnProcessor::LEFT);
	for (int x = 0; x < width; ++x) {
		left_processor.process(x, cols[x]);
	}
	
	SequentialColumnProcessor right_processor(SequentialColumnProcessor::RIGHT);
	for (int x = width - 1; x >= 0; --x) {
		right_processor.process(x, cols[x]);
	}

	if (dbg) {
		QImage const background(image.toQImage().convertToFormat(QImage::Format_RGB32));
		dbg->add(left_processor.visualizeEnvelope(background), "left_envelope");
		dbg->add(right_processor.visualizeEnvelope(background), "right_envelope");
	}

	std::pair<QLineF, QLineF> bounds;

	std::vector<Segment> segments;
	std::vector<Segment>* dbg_segments = dbg ? &segments : 0;

	QLineF left_line(left_processor.approximateWithLine(dbg_segments));
	left_line.translate(-1, 0);
	bounds.first = extendLine(left_line, height);
	if (dbg) {
		dbg->add(visualizeSegments(image.toQImage(), *dbg_segments), "left_ransac_model");
	}

	QLineF right_line(right_processor.approximateWithLine(dbg_segments));
	right_line.translate(1, 0);
	bounds.second = extendLine(right_line, height);
	if (dbg) {
		dbg->add(visualizeSegments(image.toQImage(), *dbg_segments), "right_ransac_model");
	}

	return bounds;
}
