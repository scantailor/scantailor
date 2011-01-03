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

#ifndef TOWARDS_LINE_TRACER_H_
#define TOWARDS_LINE_TRACER_H_

#include "VecNT.h"
#include "imageproc/GrayImage.h"
#include "imageproc/BinaryImage.h"
#include <QPointF>
#include <QPoint>
#include <QLineF>
#include <stdint.h>

/**
 * This class is used for tracing a path towards intersection with a given line.
 */
class TowardsLineTracer
{
public:
	TowardsLineTracer(
		imageproc::BinaryImage const& content, imageproc::GrayImage const& blurred,
		imageproc::BinaryImage const& mask, QLineF const& line, QPoint const& initial_pos);

	QPoint const* trace(qreal max_dist);
private:
	struct Neighbour
	{
		QPoint vec;
		Vec2f vecF;
		int contentLineOffset;
		int blurredPixelOffset;
		int maskLineOffset;
	};

	void setupNeighbours();

	imageproc::BinaryImage m_content;
	uint32_t const* m_pContentData;
	imageproc::GrayImage m_blurred;
	uint8_t const* m_pBlurredData;
	imageproc::BinaryImage m_mask;
	uint32_t const* m_pMaskData;
	QLineF m_line;
	Vec2f m_normalTowardsLine;
	QPoint m_lastOutputPos;
	Neighbour m_neighbours[8];
	bool m_finished;
};

#endif
