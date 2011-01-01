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
#include "imageproc/BinaryImage.h"
#include <QPoint>
#include <QPointF>
#include <vector>
#include <deque>
#include <math.h>
#include <assert.h>

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


class SequentialColumnProcessor
{
public:
	enum LeftOrRight { LEFT, RIGHT };

	void process(int x, VertRange const& range);

	QLineF approximateWithLine(LeftOrRight left_or_right) const;
private:
	// top and bottom on the leftmost or the rightmost line.
	QPoint m_leadingTop;
	QPoint m_leadingBottom;
	std::deque<QPoint> m_path; // Top to bottom.
};

void
SequentialColumnProcessor::process(int x, VertRange const& range)
{
	if (!range.isValid()) {
		return;
	}

	if (m_path.empty()) {
		m_leadingTop = QPoint(x, range.top);
		m_leadingBottom = QPoint(x, range.bottom);
		m_path.push_front(m_leadingTop);

		if (range.top != range.bottom) { // We don't want zero length segments in m_path.
			m_path.push_back(m_leadingBottom);
		}
	} else {
		if (range.top < m_path.front().y()) {
			m_path.push_front(QPoint(x, range.top));
		}
		if (range.bottom > m_path.back().y()) {
			m_path.push_back(QPoint(x, range.bottom));
		}
	}
}

QLineF
SequentialColumnProcessor::approximateWithLine(LeftOrRight left_or_right) const
{
	double sin_sum = 0;
	double cos_sum = 0;
	double weight_sum = 0;

	size_t const num_points = m_path.size();
	for (size_t i = 1; i < num_points; ++i) {
		QPoint const pt1(m_path[i - 1]);
		QPoint const pt2(m_path[i]);
		assert(pt1 != pt2);
		QPoint const vec(pt2 - pt1);

		double const weight = vec.y() * vec.y();
		double const len = sqrt(double(vec.x() * vec.x() + vec.y() * vec.y()));
		double const sin = vec.y() / len;
		double const cos = vec.x() / len;
		
		sin_sum += weight * sin;
		cos_sum += weight * cos;
		weight_sum += weight;
	}

	QPointF const reference_pt((cos_sum > 0) == (left_or_right == RIGHT) ? m_leadingTop : m_leadingBottom);

	if (weight_sum == 0) {
		return QLineF(reference_pt, reference_pt + QPointF(0, 1));
	} else {
		QPointF const vec(cos_sum / weight_sum, sin_sum / weight_sum);
		return QLineF(reference_pt, reference_pt + vec);
	}
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


std::pair<QLineF, QLineF> detectVertContentBounds(imageproc::BinaryImage const& image)
{
	int const width = image.width();
	int const height = image.height();

	std::vector<VertRange> cols;
	calculateVertRanges(image, cols);

	SequentialColumnProcessor left_processor;
	for (int x = 0; x < width; ++x) {
		left_processor.process(x, cols[x]);
	}

	SequentialColumnProcessor right_processor;
	for (int x = width - 1; x >= 0; --x) {
		right_processor.process(x, cols[x]);
	}

	std::pair<QLineF, QLineF> bounds;

	QLineF left_line(left_processor.approximateWithLine(left_processor.LEFT));
	left_line.translate(-1, 0);
	bounds.first = extendLine(left_line, height);

	QLineF right_line(right_processor.approximateWithLine(right_processor.RIGHT));
	right_line.translate(1, 0);
	bounds.second = extendLine(right_line, height);

	return bounds;
}
