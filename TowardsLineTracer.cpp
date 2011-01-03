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
#include <QRect>
#include <QtGlobal>
#include <boost/foreach.hpp>

using namespace imageproc;

TowardsLineTracer::TowardsLineTracer(
	imageproc::BinaryImage const& content, imageproc::GrayImage const& blurred,
	imageproc::BinaryImage const& mask, QLineF const& line, QPoint const& initial_pos)
:	m_content(content),
	m_pContentData(m_content.data()),
	m_blurred(blurred),
	m_pBlurredData(blurred.data()),
	m_mask(mask),
	m_pMaskData(m_mask.data()),
	m_line(line),
	m_normalTowardsLine(m_line.normalVector().p2() - m_line.p1()),
	m_lastOutputPos(initial_pos),
	m_finished(false)
{
	if (sidesOfLine(m_line, initial_pos, line.p1() + m_normalTowardsLine) > 0) {
		// It points the wrong way -> fix that.
		m_normalTowardsLine = -m_normalTowardsLine;
	}

	setupNeighbours();
}

QPoint const*
TowardsLineTracer::trace(qreal const max_dist)
{
	if (m_finished) {
		return 0;
	}

	qreal const max_sqdist = max_dist * max_dist;
	QRect const image_rect(m_blurred.rect());
	uint32_t const msb = uint32_t(1) << 31;
	
	QPoint cur_pos(m_lastOutputPos);
	QPoint last_content_pos(-1, -1);

	uint32_t const* content_line = m_pContentData + cur_pos.y() * m_content.wordsPerLine();
	uint8_t const* p_blurred = m_pBlurredData + cur_pos.y() * m_blurred.stride() + cur_pos.x();
	uint32_t const* mask_line = m_pMaskData + cur_pos.y() * m_mask.wordsPerLine();

	for (;;) {
		int best_neighbour = -1;
		qreal best_cost = NumericTraits<qreal>::max();
	
		for (int nbh_idx = 0; nbh_idx < 8; ++nbh_idx) {
			Neighbour const& nbh = m_neighbours[nbh_idx];
			
			QPoint const new_pos(cur_pos + nbh.vec);
			if (!image_rect.contains(new_pos)) {
				continue;
			}

			uint32_t const* new_mask_line = mask_line + nbh.maskLineOffset;
			if (!(new_mask_line[new_pos.x() >> 5] & (msb >> (new_pos.x() & 31)))) {
				continue;
			}
			
			qreal const dot = m_normalTowardsLine.dot(nbh.vecF);
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
			m_finished = true;
			break;
		}

		Neighbour const& nbh = m_neighbours[best_neighbour];
		cur_pos += nbh.vec;
		content_line += nbh.contentLineOffset;
		p_blurred += nbh.blurredPixelOffset;
		mask_line += nbh.maskLineOffset;

		bool content_hit = false;
		if (content_line[cur_pos.x() >> 5] & (msb >> (cur_pos.x() & 31))) {
			last_content_pos = cur_pos;
			content_hit = true;
		}
		
		if (sidesOfLine(m_line, cur_pos, m_lastOutputPos) <= 0) {
			// Line reached.
			m_finished = true;
			break;
		}

		if (content_hit) {
			// Our policy is only to output nodes that correspond to a black pixel in content.
			QPoint const vec(cur_pos - m_lastOutputPos);
			if (vec.x() * vec.x() + vec.y() * vec.y() > max_sqdist) {
				// max_dist exceeded.
				break;
			}
		}
	}

	if (last_content_pos.x() != -1) {
		m_lastOutputPos = last_content_pos;
		return &m_lastOutputPos;
	} else {
		return 0;
	}
}

void
TowardsLineTracer::setupNeighbours()
{
	// m_neighbours[0] is top-left neighbour, and then clockwise from there.

	static int const m0p[] = { -1, 0, 1 };
	static int const p0m[] = { 1, 0, -1 };

	for (int i = 0; i < 3; ++i) {
		// top line
		m_neighbours[i].vec.setX(m0p[i]);
		m_neighbours[i].vec.setY(-1);

		// right line
		m_neighbours[2 + i].vec.setX(1);
		m_neighbours[2 + i].vec.setY(m0p[i]);

		// bottom line
		m_neighbours[4 + i].vec.setX(p0m[i]);
		m_neighbours[4 + i].vec.setY(1);

		// left line
		m_neighbours[(6 + i) & 7].vec.setX(-1);
		m_neighbours[(6 + i) & 7].vec.setY(p0m[i]);
	}

	BOOST_FOREACH(Neighbour& nbh, m_neighbours) {
		nbh.vecF[0] = float(nbh.vec.x());
		nbh.vecF[1] = float(nbh.vec.y());
		nbh.contentLineOffset = nbh.vec.y() * m_content.wordsPerLine();
		nbh.blurredPixelOffset = nbh.vec.y() * m_blurred.stride() + nbh.vec.x();
		nbh.maskLineOffset = nbh.vec.y() * m_mask.wordsPerLine();
	}
}
