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

#include "TowardsLineTracer.h"
#include "SidesOfLine.h"
#include "ToLineProjector.h"
#include "NumericTraits.h"
#include <QtGlobal>
#include <boost/foreach.hpp>

using namespace imageproc;

TowardsLineTracer::TowardsLineTracer(
	imageproc::GrayImage const& blurred, imageproc::BinaryImage const& mask,
	QLineF const& line, QPointF const& initial_pos)
:	m_blurred(blurred),
	m_pBlurredData(blurred.data()),
	m_blurredStride(m_blurred.stride()),
	m_mask(mask),
	m_pMaskData(m_mask.data()),
	m_maskStride(m_mask.wordsPerLine()),
	m_line(line),
	m_normalTowardsLine(m_line.normalVector().p2() - m_line.p1()),
	m_lastPos(initial_pos),
	m_finished(false)
{
	if (sidesOfLine(m_line, initial_pos, line.p1() + m_normalTowardsLine) > 0) {
		// It points the wrong way -> fix that.
		m_normalTowardsLine = -m_normalTowardsLine;
	}

	setupNeighbours();
}

QPointF const*
TowardsLineTracer::trace(qreal const max_dist)
{
	if (m_finished) {
		return 0;
	}

	qreal const max_sqdist = max_dist * max_dist;

	int const width = m_blurred.width();
	int const height = m_blurred.height();

	uint32_t const msb = uint32_t(1) << 31;

	int x = qRound(m_lastPos.x());
	int y = qRound(m_lastPos.y());

	uint8_t const* p_blurred = m_pBlurredData + y * m_blurredStride + x;
	uint32_t const* mask_line = m_pMaskData + y * m_maskStride;

	for (;;) {
		int best_neighbour = -1;
		qreal best_cost = NumericTraits<qreal>::max();
	
		for (int nbh_idx = 0; nbh_idx < 8; ++nbh_idx) {
			Neighbour const& nbh = m_neighbours[nbh_idx];
			
			int const new_x = x + nbh.dx;
			int const new_y = y + nbh.dy;
			if (new_x < 0 || new_y < 0 || new_x >= width || new_y >= height) {
				continue;
			}

			uint32_t const* new_mask_line = mask_line + nbh.maskLineOffset;
			if (!(new_mask_line[new_x >> 5] & (msb >> (new_x & 31)))) {
				continue;
			}

			qreal const dot = nbh.fdx * m_normalTowardsLine.x() + nbh.fdy * m_normalTowardsLine.y();
			if (dot <= 0) {
				// We only allow movement that gets us closer to the line.
				continue;
			}

			// We add the term with "dot" to chose the preferred direction when all
			// colors in the neighbourhood are the same.
			qreal const cost = p_blurred[nbh.blurredPixelOffset] + 0.000001 * dot;
			if (cost < best_cost) {
				best_cost = cost;
				best_neighbour = nbh_idx;
			}
		}

		if (best_neighbour == -1) {
			// We can't proceed any further without going outside of the mask
			// or the entire image.  Now we could have jumped straight to the
			// line from here, but that's a bad idea, as that point would become
			// a part of a snake, and there wouldn't be any gradient for that
			// point to be attracted to.
			m_lastPos = QPointF(x, y);
			m_finished = true;
			break;
		}

		Neighbour const& nbh = m_neighbours[best_neighbour];
		x += nbh.dx;
		y += nbh.dy;
		p_blurred += nbh.blurredPixelOffset;
		mask_line += nbh.maskLineOffset;

		QPointF const old_pos(m_lastPos);
		m_lastPos += QPointF(nbh.fdx, nbh.fdy);

		if (sidesOfLine(m_line, m_lastPos, old_pos) <= 0) {
			// Line reached.
			m_finished = true;
			break;
		}

		QPointF const vec(m_lastPos - old_pos);
		if (vec.x() * vec.x() + vec.y() * vec.y() > max_sqdist) {
			// max_dist exceeded.
			break;
		}
	}

	return &m_lastPos;
}

void
TowardsLineTracer::setupNeighbours()
{
	// m_neighbours[0] is top-left neighbour, and then clockwise from there.

	static int const m0p[] = { -1, 0, 1 };
	static int const p0m[] = { 1, 0, -1 };

	for (int i = 0; i < 3; ++i) {
		// top line
		m_neighbours[i].dx = m0p[i];
		m_neighbours[i].dy = -1;

		// right line
		m_neighbours[2 + i].dx = 1;
		m_neighbours[2 + i].dy = m0p[i];

		// bottom line
		m_neighbours[4 + i].dx = p0m[i];
		m_neighbours[4 + i].dy = 1;

		// left line
		m_neighbours[6 + i].dx = -1;
		m_neighbours[6 + i].dy = p0m[i];
	}

	BOOST_FOREACH(Neighbour& nbh, m_neighbours) {
		nbh.fdx = nbh.dx;
		nbh.fdy = nbh.dy;
		nbh.blurredPixelOffset = nbh.dy * m_blurredStride + nbh.dx;
		nbh.maskLineOffset = nbh.dy * m_maskStride;
	}
}
