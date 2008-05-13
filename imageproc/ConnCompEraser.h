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

#ifndef IMAGEPROC_CONNCOMPERASER_H_
#define IMAGEPROC_CONNCOMPERASER_H_

#include "NonCopyable.h"
#include "Connectivity.h"
#include "ConnComp.h"
#include "BinaryImage.h"
#include <stack>
#include <stdint.h>

namespace imageproc
{

class ConnComp;

/**
 * \brief Erases connected components one by one and returns their bounding boxes.
 */
class ConnCompEraser
{
	DECLARE_NON_COPYABLE(ConnCompEraser)
public:
	/**
	 * \brief Constructor.
	 *
	 * \param image The image from which connected components are to be erased.
	 *        If you don't need the original image, pass image.release(), to
	 *        avoid unnecessary copy-on-write.
	 * \param conn Defines which neighbouring pixels are considered to be connected.
	 */
	ConnCompEraser(BinaryImage const& image, Connectivity conn);
	
	/**
	 * \brief Erase the next connected component and return its bounding box.
	 *
	 * If there are no black pixels remaining, returns a null ConnComp.
	 */
	ConnComp nextConnComp();
	
	/**
	 * \brief Returns the image in its present state.
	 *
	 * Every time nextConnComp() is called, a connected component
	 * is erased from the image, assuming there was one.
	 */
	BinaryImage const& image() const { return m_image; }
private:
	struct Segment
	{
		uint32_t* line; /**< Pointer to the beginning of the line. */
		int xleft;  /**< Leftmost pixel to process. */
		int xright; /**< Rightmost pixel to process. */
		int y;      /**< y value of the line to be processed. */
		int dy;     /**< Vertical direction: 1 or -1. */
		int dy_wpl; /**< words_per_line or -words_per_line. */
	};
	
	struct BBox;
	
	void pushSegSameDir(Segment const& seg, int xleft, int xright, BBox& bbox);
	
	void pushSegInvDir(Segment const& seg, int xleft, int xright, BBox& bbox);
	
	void pushInitialSegments();
	
	bool moveToNextBlackPixel();
	
	ConnComp eraseConnComp4();
	
	ConnComp eraseConnComp8();
	
	static uint32_t getBit(uint32_t const* line, int x);
	
	static void clearBit(uint32_t* line, int x);
	
	BinaryImage m_image;
	uint32_t* m_pLine;
	int const m_width;
	int const m_height;
	int const m_wpl;
	Connectivity const m_connectivity;
	std::stack<Segment> m_segStack;
	int m_x;
	int m_y;
};

} // namespace imageproc

#endif
