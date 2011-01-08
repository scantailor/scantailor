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

#ifndef GRID_H_
#define GRID_H_

#include <boost/scoped_array.hpp>

template<typename Node>
class Grid
{
public:
	/**
	 * \brief Creates a width x height grid with specified padding on each side.
	 */
	Grid(int width, int height, int padding);

	void initPadding(Node const& padding_node);

	void initInterior(Node const& interior_node);

	/**
	 * \brief Returns a pointer to the beginning of unpadded data.
	 */
	Node* data() { return m_pData; }

	/**
	 * \brief Returns a pointer to the beginning of unpadded data.
	 */
	Node const* data() const { return m_pData; }

	/**
	 * \brief Returns a pointer to the beginning of padded data.
	 */
	Node* paddedData() { return m_storage.get(); }

	/**
	 * \brief Returns a pointer to the beginning of padded data.
	 */
	Node const* paddedData() const { return m_storage.get(); }

	/**
	 * Returns the number of nodes in a row, including padding nodes.
	 */
	int stride() const { return m_stride; }

	/**
	 * Returns the number of nodes in a row, excluding padding nodes.
	 */
	int width() const { return m_width; }

	/**
	 * Returns the number of nodes in a column, excluding padding nodes.
	 */
	int height() const { return m_height; }

	/**
	 * Returns the number of padding layers from each side.
	 */
	int padding() const { return m_padding; }
private:
	boost::scoped_array<Node> m_storage;
	Node* m_pData;
	int m_width;
	int m_height;
	int m_stride;
	int m_padding;
};


template<typename Node>
Grid<Node>::Grid(int width, int height, int padding)
:	m_storage(new Node[(width + padding*2) * (height + padding*2)]),
	m_pData(m_storage.get() + (width + padding*2)*padding + padding),
	m_width(width),
	m_height(height),
	m_stride(width + padding*2),
	m_padding(padding)
{
}

template<typename Node>
void
Grid<Node>::initPadding(Node const& padding_node)
{
	if (m_padding == 0) {
		// No padding.
		return;
	}

	Node* line = m_storage.get();
	for (int row = 0; row < m_padding; ++row) {
		for (int x = 0; x < m_stride; ++x) {
			line[x] = padding_node;
		}
		line += m_stride;
	}

	for (int y = 0; y < m_height; ++y) {
		for (int col = 0; col < m_padding; ++col) {
			line[col] = padding_node;
		}
		for (int col = m_stride - m_padding; col < m_stride; ++col) {
			line[col] = padding_node;
		}
		line += m_stride;
	}

	for (int row = 0; row < m_padding; ++row) {
		for (int x = 0; x < m_stride; ++x) {
			line[x] = padding_node;
		}
		line += m_stride;
	}
}

template<typename Node>
void
Grid<Node>::initInterior(Node const& interior_node)
{
	Node* line = m_pData;
	for (int y = 0; y < m_height; ++y) {
		for (int x = 0; x < m_width; ++x) {
			line[x] = interior_node;
		}
		line += m_stride;
	}
}

#endif
