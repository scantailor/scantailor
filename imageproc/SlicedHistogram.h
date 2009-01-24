/*
    Scan Tailor - Interactive post-processing tool for scanned pages.
    Copyright (C) 2007-2008  Joseph Artsimovich <joseph_a@mail.ru>

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

#ifndef IMAGEPROC_SLICEDHISTOGRAM_H_
#define IMAGEPROC_SLICEDHISTOGRAM_H_

#include <vector>
#include <stddef.h>

class QRect;

namespace imageproc
{

class BinaryImage;

/**
 * \brief Calculates and stores the number of black pixels
 *        in each horizontal or vertical line.
 */
class SlicedHistogram
{
	// Member-wise copying is OK.
public:
	enum Type {
		ROWS, /**< Process horizontal lines. */
		COLS  /**< Process vertical lines. */
	};
	
	/**
	 * \brief Constructs an empty histogram.
	 */
	SlicedHistogram();
	
	/**
	 * \brief Calculates the histogram of the whole image.
	 *
	 * \param image The image to process.  A null image will produce
	 *        an empty histogram.
	 * \param type Specifies whether to process columns or rows.
	 */
	SlicedHistogram(BinaryImage const& image, Type type);
	
	/**
	 * \brief Calculates the histogram of a portion of the image.
	 *
	 * \param image The image to process.  A null image will produce
	 *        an empty histogram, provided that \p area is also null.
	 * \param area The area of the image to process.  The first value
	 *        in the histogram will correspond to the first line in this area.
	 * \param type Specifies whether to process columns or rows.
	 *
	 * \exception std::invalid_argument If \p area is not completely
	 *            within image.rect().
	 */
	SlicedHistogram(BinaryImage const& image, QRect const& area, Type type);
	
	size_t size() const { return m_data.size(); }
	
	void setSize(size_t size) { m_data.resize(size); }
	
	int const& operator[](size_t idx) const { return m_data[idx]; }
	
	int& operator[](size_t idx) { return m_data[idx]; }
private:
	void processHorizontalLines(BinaryImage const& image, QRect const& area);
	
	void processVerticalLines(BinaryImage const& image, QRect const& area);
	
	std::vector<int> m_data;
};

} // namespace imageproc

#endif
