/*
    Scan Tailor - Interactive post-processing tool for scanned pages.
    Copyright (C) 2007-2009  Joseph Artsimovich <joseph_a@mail.ru>

    Based on code from the ANIMAL image processing library.
    Copyright (C) 2002,2003  Ricardo Fabbri <rfabbri@if.sc.usp.br>

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

#include "SEDM.h"
#include "BinaryImage.h"
#include "ConnectivityMap.h"
#include <algorithm>
#include <string.h>
#include <math.h>
#include <assert.h>

namespace imageproc
{

// Note that -1 is an implementation detail.
// It exists to make sure INF_DIST + 1 doesn't overflow.
uint32_t const SEDM::INF_DIST = ~uint32_t(0) - 1;

SEDM::SEDM()
:	m_pData(0),
	m_size(),
	m_stride(0)
{
}

SEDM::SEDM(
	BinaryImage const& image, DistType const dist_type,
	Borders const borders)
:	m_pData(0),
	m_size(image.size()),
	m_stride(0)
{
	if (image.isNull()) {
		return;
	}
	
	int const width = m_size.width();
	int const height = m_size.height();
	
	m_data.resize((width + 2) * (height + 2), INF_DIST);
	m_stride = width + 2;
	m_pData = &m_data[0] + m_stride + 1;
	
	if (borders & DIST_TO_TOP_BORDER) {
		memset(&m_data[0], 0, m_stride * sizeof(m_data[0]));
	}
	if (borders & DIST_TO_BOTTOM_BORDER) {
		memset(
			&m_data[m_data.size() - m_stride],
			0, m_stride * sizeof(m_data[0])
		);
	}
	if (borders & (DIST_TO_LEFT_BORDER|DIST_TO_RIGHT_BORDER)) {
		int const last = m_stride - 1;
		uint32_t* line = &m_data[0];
		for (int todo = height + 2; todo > 0; --todo) {
			if (borders & DIST_TO_LEFT_BORDER) {
				line[0] = 0;
			}
			if (borders & DIST_TO_RIGHT_BORDER) {
				line[last] = 0;
			}
			line += m_stride;
		}
	}
	
	uint32_t initial_distance[2];
	if (dist_type == DIST_TO_WHITE) {
		initial_distance[0] = 0; // white
		initial_distance[1] = INF_DIST; // black
	} else {
		initial_distance[0] = INF_DIST; // white
		initial_distance[1] = 0; // black
	}
	
	uint32_t* p_dist = m_pData;
	uint32_t const* img_line = image.data();
	int const img_stride = image.wordsPerLine();
	for (int y = 0; y < height; ++y) {
		for (int x = 0; x < width; ++x, ++p_dist) {
			uint32_t word = img_line[x >> 5];
			word >>= 31 - (x & 31);
			*p_dist = initial_distance[word & 1];
		}
		p_dist += 2;
		img_line += img_stride;
	}
	
	processColumns();
	processRows();
}

SEDM::SEDM(ConnectivityMap& cmap)
:	m_pData(0),
	m_size(cmap.size()),
	m_stride(0)
{
	if (m_size.isEmpty()) {
		return;
	}
	
	int const width = m_size.width();
	int const height = m_size.height();
	
	m_data.resize((width + 2) * (height + 2), INF_DIST);
	m_stride = width + 2;
	m_pData = &m_data[0] + m_stride + 1;
	
	uint32_t* p_dist = m_pData;
	uint32_t const* p_label = cmap.data();
	for (int y = 0; y < height; ++y) {
		for (int x = 0; x < width; ++x, ++p_dist, ++p_label) {
			if (*p_label) {
				*p_dist = 0;
			}
		}
		p_dist += 2;
		p_label += 2;
	}
	
	processColumns(cmap);
	processRows(cmap);
}

SEDM::SEDM(SEDM const& other)
:	m_data(other.m_data),
	m_pData(0),
	m_size(other.m_size),
	m_stride(other.m_stride)
{
	if (!m_size.isEmpty()) {
		m_pData = &m_data[0] + m_stride + 1;
	}
}

SEDM&
SEDM::operator=(SEDM const& other)
{
	SEDM(other).swap(*this);
	return *this;
}

void
SEDM::swap(SEDM& other)
{
	m_data.swap(other.m_data);
	std::swap(m_pData, other.m_pData);
	std::swap(m_size, other.m_size);
	std::swap(m_stride, other.m_stride);
}

inline uint32_t
SEDM::distSq(
	int const x1, int const x2, uint32_t const dy_sq)
{
	if (dy_sq == INF_DIST) {
		return INF_DIST;
	}
	int const dx = x1 - x2;
	uint32_t const dx_sq = dx * dx;
	return dx_sq + dy_sq;
}

void
SEDM::processColumns()
{
	int const width = m_size.width() + 2;
	int const height = m_size.height() + 2;
	
	uint32_t* p_sqd = &m_data[0];
	for (int x = 0; x < width; ++x, ++p_sqd) {
		// (d + 1)^2 = d^2 + 2d + 1
		uint32_t b = 1; // 2d + 1 in the above formula.
		for (int todo = height - 1; todo > 0; --todo) {
			uint32_t const sqd = *p_sqd + b;
			p_sqd += width;
			if (*p_sqd > sqd) {
				*p_sqd = sqd;
				b += 2;
			} else {
				b = 1;
			}
		}
		
		b = 1;
		for (int todo = height - 1; todo > 0; --todo) {
			uint32_t const sqd = *p_sqd + b;
			p_sqd -= width;
			if (*p_sqd > sqd) {
				*p_sqd = sqd;
				b += 2;
			} else {
				b = 1;
			}
		}
	}
}

void
SEDM::processColumns(ConnectivityMap& cmap)
{
	int const width = m_size.width() + 2;
	int const height = m_size.height() + 2;
	
	uint32_t* p_sqd = &m_data[0];
	uint32_t* p_label = cmap.paddedData();
	for (int x = 0; x < width; ++x, ++p_sqd, ++p_label) {
		// (d + 1)^2 = d^2 + 2d + 1
		uint32_t b = 1; // 2d + 1 in the above formula.
		for (int todo = height - 1; todo > 0; --todo) {
			uint32_t const sqd = *p_sqd + b;
			p_sqd += width;
			p_label += width;
			if (sqd < *p_sqd) {
				*p_sqd = sqd;
				*p_label = p_label[-width];
				b += 2;
			} else {
				b = 1;
			}
		}
		
		b = 1;
		for (int todo = height - 1; todo > 0; --todo) {
			uint32_t const sqd = *p_sqd + b;
			p_sqd -= width;
			p_label -= width;
			if (sqd < *p_sqd) {
				*p_sqd = sqd;
				*p_label = p_label[width];
				b += 2;
			} else {
				b = 1;
			}
		}
	}
}

void
SEDM::processRows()
{
	int const width = m_size.width() + 2;
	int const height = m_size.height() + 2;
	
	std::vector<int> s(width, 0);
	std::vector<int> t(width, 0);
	std::vector<uint32_t> row_copy(width, 0);
	
	uint32_t* line = &m_data[0];
	for (int y = 0; y < height; ++y, line += width) {
		int q = 0;
		s[0] = 0;
		t[0] = 0;
		for (int x = 1; x < width; ++x) {
			while (q >= 0 && distSq(t[q], s[q], line[s[q]])
					> distSq(t[q], x, line[x])) {
				--q;
			}
			
			if (q < 0) {
				q = 0;
				s[0] = x;
			} else {
				int const x2 = s[q];
				if (line[x] != INF_DIST && line[x2] != INF_DIST) {
					int w = (x * x + line[x]) - (x2 * x2 + line[x2]);
					w /= (x - x2) << 1;
					++w;
					if ((unsigned)w < (unsigned)width) {
						++q;
						s[q] = x;
						t[q] = w;
					}
				}
			}
		}
		
		memcpy(&row_copy[0], line, width * sizeof(*line));
		
		for (int x = width - 1; x >= 0; --x) {
			int const x2 = s[q];
			line[x] = distSq(x, x2, row_copy[x2]);
			if (x == t[q]) {
				--q;
			}
		}
	}
}

void
SEDM::processRows(ConnectivityMap& cmap)
{
	int const width = m_size.width() + 2;
	int const height = m_size.height() + 2;
	
	std::vector<int> s(width, 0);
	std::vector<int> t(width, 0);
	std::vector<uint32_t> row_copy(width, 0);
	std::vector<uint32_t> cmap_row_copy(width, 0);
	
	uint32_t* line = &m_data[0];
	uint32_t* cmap_line = cmap.paddedData();
	for (int y = 0; y < height; ++y, line += width, cmap_line += width) {
		int q = 0;
		s[0] = 0;
		t[0] = 0;
		for (int x = 1; x < width; ++x) {
			while (q >= 0 && distSq(t[q], s[q], line[s[q]])
					> distSq(t[q], x, line[x])) {
				--q;
			}
			
			if (q < 0) {
				q = 0;
				s[0] = x;
			} else {
				int const x2 = s[q];
				if (line[x] != INF_DIST && line[x2] != INF_DIST) {
					int w = (x * x + line[x]) - (x2 * x2 + line[x2]);
					w /= (x - x2) << 1;
					++w;
					if ((unsigned)w < (unsigned)width) {
						++q;
						s[q] = x;
						t[q] = w;
					}
				}
			}
		}
		
		memcpy(&row_copy[0], line, width * sizeof(*line));
		memcpy(&cmap_row_copy[0], cmap_line, width * sizeof(*cmap_line));
		
		for (int x = width - 1; x >= 0; --x) {
			int const x2 = s[q];
			line[x] = distSq(x, x2, row_copy[x2]);
			cmap_line[x] = cmap_row_copy[x2];
			if (x == t[q]) {
				--q;
			}
		}
	}
}

} // namespace imageproc
