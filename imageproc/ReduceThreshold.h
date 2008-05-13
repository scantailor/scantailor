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

#ifndef IMAGEPROC_REDUCETHRESHOLD_H_
#define IMAGEPROC_REDUCETHRESHOLD_H_

#include "BinaryImage.h"

namespace imageproc
{

/**
 * \brief Performs 2x horizontal and vertical downscaling on 1-bit images.
 *
 * The dimensions of the target image will be:
 * \code
 * dst_width = max(1, floor(src_width / 2));
 * dst_height = max(1, floor(src_height / 2));
 * \endcode
 * \n
 * Processing a null image results in a null image.
 * Processing a non-null image never results in a null image.\n
 * \n
 * A 2x2 square in the source image becomes 1 pixel in the target image.
 * The threshold parameter controls how many black pixels need to be in
 * the source 2x2 square in order to make the destination pixel black.
 * Valid threshold values are 1, 2, 3 and 4.\n
 * \n
 * If the source image is a line 1 pixel thick, downscaling will be done
 * as if the line was thickened to 2 pixels by duplicating existing pixels.\n
 * \n
 * It is possible to do a cascade of reductions:
 * \code
 * BinaryImage out = ReduceThreshold(input)(4)(4)(3);
 * \endcode
 */
class ReduceThreshold
{
public:
	/**
	 * \brief Constructor.  Doesn't do any work by itself.
	 */
	ReduceThreshold(BinaryImage const& image);
	
	/**
	 * \brief Implicit conversion to BinaryImage.
	 */
	operator BinaryImage const&() const { return m_image; }
	
	/**
	 * \brief Returns a reference to the reduced image.
	 */
	BinaryImage const& image() const { return m_image; }
	
	/**
	 * \brief Performs a reduction and returns *this.
	 */
	ReduceThreshold& reduce(int threshold);
	
	/**
	 * \brief Operator () performs a reduction and returns *this.
	 */
	ReduceThreshold& operator()(int threshold) {
		return reduce(threshold);
	}
private:
	void reduceHorLine(int threshold);
	
	void reduceVertLine(int threshold);
	
	/**
	 * \brief The result of a previous reduction.
	 */
	BinaryImage m_image;
};


} // namespace imageproc

#endif
