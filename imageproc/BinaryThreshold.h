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

#ifndef IMAGEPROC_BINARYTHRESHOLD_H_
#define IMAGEPROC_BINARYTHRESHOLD_H_

#include "BWColor.h"

class QImage;

namespace imageproc
{

class GrayscaleHistogram;

/**
 * \brief Defines the gray level threshold that separates black from white.
 *
 * Gray levels in range of [0, threshold) are considered black, while
 * levels in range of [threshold, 255] are considered white.  The threshold
 * itself is considered to be white.
 */
class BinaryThreshold
{
	// Member-wise copying is OK.
public:
	/**
	 * \brief Finds the threshold using Otsu’s thresholding method.
	 */
	static BinaryThreshold otsuThreshold(QImage const& image);
	
	/**
	 * \brief Finds the threshold using Otsu’s thresholding method.
	 */
	static BinaryThreshold otsuThreshold(GrayscaleHistogram const& pixels_by_color);
	
	/**
	 * \brief Image binarization using Mokji's global thresholding method.
	 *
	 * M. M. Mokji, S. A. R. Abu-Bakar: Adaptive Thresholding Based on
	 * Co-occurrence Matrix Edge Information. Asia International Conference on
	 * Modelling and Simulation 2007: 444-450
	 * http://www.academypublisher.com/jcp/vol02/no08/jcp02084452.pdf
	 *
	 * \param image The source image.  May be in any format.
	 * \param max_edge_width The maximum gradient length to consider.
	 * \param min_edge_magnitude The minimum color difference in a gradient.
	 * \return A black and white image.
	 */
	static BinaryThreshold mokjiThreshold(
		QImage const& image,
		unsigned max_edge_width = 3, unsigned min_edge_magnitude = 20);
	
	explicit BinaryThreshold(int threshold) : m_threshold(threshold) {}
	
	operator int() const { return m_threshold; }
	
	BWColor grayToBW(int gray) const { return gray < m_threshold ? BLACK : WHITE; } 
private:
	int m_threshold;
};

} // namespace imageproc

#endif
