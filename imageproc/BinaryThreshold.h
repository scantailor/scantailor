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
	
	explicit BinaryThreshold(int threshold) : m_threshold(threshold) {}
	
	operator int() const { return m_threshold; }
	
	BWColor grayToBW(int gray) const { return gray < m_threshold ? BLACK : WHITE; } 
private:
	int m_threshold;
};

} // namespace imageproc

#endif
