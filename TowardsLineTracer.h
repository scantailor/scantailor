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

#include "imageproc/GrayImage.h"
#include "imageproc/BinaryImage.h"
#include <QPointF>
#include <QLineF>
#include <stdint.h>

/**
 * This class is used for tracing a path towards intersection with a given line.
 */
class TowardsLineTracer
{
public:
	TowardsLineTracer(
		imageproc::GrayImage const& blurred, imageproc::BinaryImage const& mask,
		QLineF const& line, QPointF const& initial_pos);

	QPointF const* trace(qreal max_dist);
private:
	struct Neighbour
	{
		qreal fdx;
		qreal fdy;
		int dx;
		int dy;
		int blurredPixelOffset;
		int maskLineOffset;
	};

	void setupNeighbours();

	imageproc::GrayImage m_blurred;
	uint8_t const* m_pBlurredData;
	int const m_blurredStride;
	imageproc::BinaryImage m_mask;
	uint32_t const* m_pMaskData;
	int const m_maskStride;
	QLineF m_line;
	QPointF m_normalTowardsLine;
	QPointF m_lastPos;
	Neighbour m_neighbours[8];
	bool m_finished;
};

#endif
