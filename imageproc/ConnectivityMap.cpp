/*
    Scan Tailor - Interactive post-processing tool for scanned pages.
    Copyright (C) 2007-2009  Joseph Artsimovich <joseph_a@mail.ru>

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

#include "ConnectivityMap.h"
#include "BinaryImage.h"
#include "InfluenceMap.h"
#include "BitOps.h"
#ifndef Q_MOC_RUN
#include <boost/foreach.hpp>
#endif
#include <QImage>
#include <QColor>
#include <QDebug>
#include <algorithm>
#include <stdexcept>
#include <assert.h>

namespace imageproc
{

uint32_t const ConnectivityMap::BACKGROUND = ~uint32_t(0);
uint32_t const ConnectivityMap::UNTAGGED_FG = BACKGROUND - 1;

ConnectivityMap::ConnectivityMap()
:	m_pData(0),
	m_size(),
	m_stride(0),
	m_maxLabel(0)
{
}

ConnectivityMap::ConnectivityMap(QSize const& size)
:	m_pData(0),
	m_size(size),
	m_stride(0),
	m_maxLabel(0)
{
	if (m_size.isEmpty()) {
		return;
	}
	
	int const width = m_size.width();
	int const height = m_size.height();
	
	m_data.resize((width + 2) * (height + 2), 0);
	m_stride = width + 2;
	m_pData = &m_data[0] + 1 + m_stride;
}

ConnectivityMap::ConnectivityMap(
	BinaryImage const& image, Connectivity const conn)
:	m_pData(0),
	m_size(image.size()),
	m_stride(0),
	m_maxLabel(0)
{
	if (m_size.isEmpty()) {
		return;
	}
	
	int const width = m_size.width();
	int const height = m_size.height();
	
	m_data.resize((width + 2) * (height + 2), BACKGROUND);
	m_stride = width + 2;
	m_pData = &m_data[0] + 1 + m_stride;
	
	uint32_t* dst = m_pData;
	int const dst_stride = m_stride;
	
	uint32_t const* src = image.data();
	int const src_stride = image.wordsPerLine();
	
	uint32_t const msb = uint32_t(1) << 31;
	for (int y = 0; y < height; ++y) {
		for (int x = 0; x < width; ++x) {
			if (src[x >> 5] & (msb >> (x & 31))) {
				dst[x] = UNTAGGED_FG;
			}
		}
		src += src_stride;
		dst += dst_stride;
	}
	
	assignIds(conn);
}

ConnectivityMap::ConnectivityMap(ConnectivityMap const& other)
:	m_data(other.m_data),
	m_pData(0),
	m_size(other.size()),
	m_stride(other.stride()),
	m_maxLabel(other.m_maxLabel)
{
	if (!m_size.isEmpty()) {
		m_pData = &m_data[0] + m_stride + 1;
	}
}

ConnectivityMap::ConnectivityMap(InfluenceMap const& imap)
:	m_pData(0),
	m_size(imap.size()),
	m_stride(imap.stride()),
	m_maxLabel(imap.maxLabel())
{
	if (m_size.isEmpty()) {
		return;
	}
	
	m_data.resize((m_size.width() + 2) * (m_size.height() + 2));
	copyFromInfluenceMap(imap);
}

ConnectivityMap&
ConnectivityMap::operator=(ConnectivityMap const& other)
{
	ConnectivityMap(other).swap(*this);
	return *this;
}

ConnectivityMap&
ConnectivityMap::operator=(InfluenceMap const& imap)
{
	if (m_size == imap.size() && !m_size.isEmpty()) {
		// Common case optimization.
		copyFromInfluenceMap(imap);
	} else {
		ConnectivityMap(imap).swap(*this);
	}
	return *this;
}

void
ConnectivityMap::swap(ConnectivityMap& other)
{
	m_data.swap(other.m_data);
	std::swap(m_pData, other.m_pData);
	std::swap(m_size, other.m_size);
	std::swap(m_stride, other.m_stride);
	std::swap(m_maxLabel, other.m_maxLabel);
}

void
ConnectivityMap::addComponent(BinaryImage const& image)
{
	if (m_size != image.size()) {
		throw std::invalid_argument("ConnectivityMap::addComponent: sizes don't match");
	}
	if (m_size.isEmpty()) {
		return;
	}
	
	int const width = m_size.width();
	int const height = m_size.height();
	
	uint32_t* dst = m_pData;
	int const dst_stride = m_stride;
	
	uint32_t const* src = image.data();
	int const src_stride = image.wordsPerLine();
	
	uint32_t const new_label = m_maxLabel + 1;
	uint32_t const msb = uint32_t(1) << 31;
	for (int y = 0; y < height; ++y) {
		for (int x = 0; x < width; ++x) {
			if (src[x >> 5] & (msb >> (x & 31))) {
				dst[x] = new_label;
			}
		}
		src += src_stride;
		dst += dst_stride;
	}
	
	m_maxLabel = new_label;
}

QImage
ConnectivityMap::visualized(QColor bgcolor) const
{
	if (m_size.isEmpty()) {
		return QImage();
	}

	int const width = m_size.width();
	int const height = m_size.height();
	
	// Convert to premultiplied RGBA.
	bgcolor = bgcolor.toRgb();
	bgcolor.setRedF(bgcolor.redF() * bgcolor.alphaF());
	bgcolor.setGreenF(bgcolor.greenF() * bgcolor.alphaF());
	bgcolor.setBlueF(bgcolor.blueF() * bgcolor.alphaF());

	QImage dst(m_size, QImage::Format_ARGB32);
	dst.fill(bgcolor.rgba());
	
	uint32_t const* src_line = m_pData;
	int const src_stride = m_stride;
	
	uint32_t* dst_line = reinterpret_cast<uint32_t*>(dst.bits());
	int const dst_stride = dst.bytesPerLine() / sizeof(uint32_t);
	
	for (int y = 0; y < height; ++y) {
		for (int x = 0; x < width; ++x) {
			uint32_t const val = src_line[x];
			if (val == 0) {
				continue;
			}
			
			int const bits_unused = countMostSignificantZeroes(val);
			uint32_t const reversed = reverseBits(val) >> bits_unused;
			uint32_t const mask = ~uint32_t(0) >> bits_unused;
			
			double const H = 0.99 * (double(reversed) / mask);
			double const S = 1.0;
			double const V = 1.0;
			QColor color;
			color.setHsvF(H, S, V, 1.0);
			
			dst_line[x] = color.rgba();
		}
		src_line += src_stride;
		dst_line += dst_stride;
	}
	
	return dst;
}

void
ConnectivityMap::copyFromInfluenceMap(InfluenceMap const& imap)
{
	assert(!imap.size().isEmpty());
	assert(imap.size() == m_size);
	
	int const width = m_size.width() + 2;
	int const height = m_size.height() + 2;
	
	uint32_t* dst = &m_data[0];
	InfluenceMap::Cell const* src = imap.paddedData();
	for (int todo = width * height; todo > 0; --todo) {
		*dst = src->label;
		++dst;
		++src;
	}
}

void
ConnectivityMap::assignIds(Connectivity const conn)
{
	uint32_t const num_initial_tags = initialTagging();
	std::vector<uint32_t> table(num_initial_tags, 0);
	
	switch (conn) {
		case CONN4:
			spreadMin4();
			break;
		case CONN8:
			spreadMin8();
			break;
	}
	
	markUsedIds(table);
	
	uint32_t next_label = 1;
	for (uint32_t i = 0; i < num_initial_tags; ++i) {
		if (table[i]) {
			table[i] = next_label;
			++next_label;
		}
	}
	
	remapIds(table);
	
	m_maxLabel = next_label - 1;
}

/**
 * Tags every object pixel that has a non-object pixel to the left.
 */
uint32_t
ConnectivityMap::initialTagging()
{
	int const width = m_size.width();
	int const height = m_size.height();
	
	uint32_t next_label = 1;
	
	uint32_t* line = m_pData;
	int const stride = m_stride;
	
	for (int y = 0; y < height; ++y, line += stride) {
		for (int x = 0; x < width; ++x) {
			if (line[x - 1] == BACKGROUND
					&& line[x] == UNTAGGED_FG) {
				line[x] = next_label;
				++next_label;
			}
		}
	}
	
	return next_label - 1;
}

void
ConnectivityMap::spreadMin4()
{
	int const width = m_size.width();
	int const height = m_size.height();
	int const stride = m_stride;
	
	uint32_t* line = m_pData;
	uint32_t* prev_line = m_pData - stride;
	
	// Top to bottom.
	for (int y = 0; y < height; ++y) {
		// Left to right.
		for (int x = 0; x < width; ++x) {
			if (line[x] == BACKGROUND) {
				continue;
			}
			line[x] = std::min(
				prev_line[x],
				std::min(line[x - 1], line[x])
			);
		}
		
		prev_line = line;
		line += stride;
	}
	
	prev_line = line;
	line -= stride;
	
	FastQueue<uint32_t*> queue;
	
	// Bottom to top.
	for (int y = height - 1; y >= 0; --y) {
		// Right to left.
		for (int x = width - 1; x >= 0; --x) {
			if (line[x] == BACKGROUND) {
				continue;
			}
			
			uint32_t const new_val = std::min(
				line[x + 1], prev_line[x]
			);
			
			if (new_val >= line[x]) {
				continue;
			}
			
			line[x] = new_val;
			
			// We compare new_val + 1 < neighbor + 1 to
			// make BACKGROUND neighbors overflow and become
			// zero.
			uint32_t const nvp1 = new_val + 1;
			if (nvp1 < line[x + 1] + 1 || nvp1 < prev_line[x] + 1) {
				queue.push(&line[x]);
			}
		}
		prev_line = line;
		line -= stride;
	}
	
	processQueue4(queue);
}

void
ConnectivityMap::spreadMin8()
{
	int const width = m_size.width();
	int const height = m_size.height();
	int const stride = m_stride;
	
	uint32_t* line = m_pData;
	uint32_t* prev_line = m_pData - stride;
	
	// Top to bottom.
	for (int y = 0; y < height; ++y) {
		// Left to right.
		for (int x = 0; x < width; ++x) {
			if (line[x] == BACKGROUND) {
				continue;
			}
			line[x] = std::min(
				std::min(
					std::min(prev_line[x - 1], prev_line[x]),
					std::min(prev_line[x + 1], line[x - 1])
				),
				line[x]
			);
		}
		
		prev_line = line;
		line += stride;
	}
	
	prev_line = line;
	line -= stride;
	
	FastQueue<uint32_t*> queue;
	
	// Bottom to top.
	for (int y = height - 1; y >= 0; --y) {
		for (int x = width - 1; x >= 0; --x) {
			if (line[x] == BACKGROUND) {
				continue;
			}
			
			uint32_t const new_val = std::min(
				std::min(prev_line[x - 1], prev_line[x]),
				std::min(prev_line[x + 1], line[x + 1])
			);
			
			if (new_val >= line[x]) {
				continue;
			}
			
			line[x] = new_val;
			
			// We compare new_val + 1 < neighbor + 1 to
			// make BACKGROUND neighbors overflow and become
			// zero.
			uint32_t const nvp1 = new_val + 1;
			if (nvp1 < prev_line[x - 1] + 1
					|| nvp1 < prev_line[x] + 1
					|| nvp1 < prev_line[x + 1] + 1
					|| nvp1 < line[x + 1] + 1) {
				queue.push(&line[x]);
			}
		}
		
		prev_line = line;
		line -= stride;
	}
	
	processQueue8(queue);
}

void
ConnectivityMap::processNeighbor(
	FastQueue<uint32_t*>& queue,
	uint32_t const this_val, uint32_t* neighbor)
{
	// *neighbor + 1 will overflow if *neighbor == BACKGROUND,
	// which is what we want.
	if (this_val + 1 < *neighbor + 1) {
		*neighbor = this_val;
		queue.push(neighbor);
	}
}

void
ConnectivityMap::processQueue4(FastQueue<uint32_t*>& queue)
{
	int const stride = m_stride;
	
	while (!queue.empty()) {
		uint32_t* p = queue.front();
		queue.pop();
		
		uint32_t const this_val = *p;
		
		// Northern neighbor.
		p -= stride;
		processNeighbor(queue, this_val, p);
		
		// Eastern neighbor.
		p += stride + 1;
		processNeighbor(queue, this_val, p);
		
		// Southern neighbor.
		p += stride - 1;
		processNeighbor(queue, this_val, p);
		
		// Western neighbor.
		p -= stride + 1;
		processNeighbor(queue, this_val, p);
	}
}

void
ConnectivityMap::processQueue8(FastQueue<uint32_t*>& queue)
{
	int const stride = m_stride;
	
	while (!queue.empty()) {
		uint32_t* p = queue.front();
		queue.pop();
		
		uint32_t const this_val = *p;
		
		// Northern neighbor.
		p -= stride;
		processNeighbor(queue, this_val, p);
		
		// North-eastern neighbor.
		++p;
		processNeighbor(queue, this_val, p);
		
		// Eastern neighbor.
		p += stride;
		processNeighbor(queue, this_val, p);
		
		// South-eastern neighbor.
		p += stride;
		processNeighbor(queue, this_val, p);
		
		// Southern neighbor.
		--p;
		processNeighbor(queue, this_val, p);
		
		// South-western neighbor.
		--p;
		processNeighbor(queue, this_val, p);
		
		// Western neighbor.
		p -= stride;
		processNeighbor(queue, this_val, p);
		
		// North-western neighbor.
		p -= stride;
		processNeighbor(queue, this_val, p);
	}
}

void
ConnectivityMap::markUsedIds(std::vector<uint32_t>& used_map) const
{
	int const width = m_size.width();
	int const height = m_size.height();
	int const stride = m_stride;
	
	uint32_t const* line = m_pData;
	
	// Top to bottom.
	for (int y = 0; y < height; ++y, line += stride) {
		// Left to right.
		for (int x = 0; x < width; ++x) {
			if (line[x] == BACKGROUND) {
				continue;
			}
			used_map[line[x] - 1] = 1;
		}
	}
}

void
ConnectivityMap::remapIds(std::vector<uint32_t> const& map)
{
	BOOST_FOREACH(uint32_t& label, m_data) {
		if (label == BACKGROUND) {
			label = 0;
		} else {
			label = map[label - 1];
		}
	}
}

} // namespace imageproc
