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

#ifndef IMAGEPROC_CONNCOMPERASEREXT_H_
#define IMAGEPROC_CONNCOMPERASEREXT_H_

#include "NonCopyable.h"
#include "Connectivity.h"
#include "ConnCompEraser.h"
#include "ConnComp.h"
#include "BinaryImage.h"

class QRect;

namespace imageproc
{

/**
 * \brief Same as ConnCompEraser, except it provides images of connected components.
 */
class ConnCompEraserExt
{
	DECLARE_NON_COPYABLE(ConnCompEraserExt)
public:
	/**
	 * \brief Constructor.
	 *
	 * \param image The image from which connected components are to be erased.
	 *        If you don't need the original image, pass image.release(), to
	 *        avoid unnecessary copy-on-write.
	 * \param conn Defines which neighbouring pixels are considered to be connected.
	 */
	ConnCompEraserExt(BinaryImage const& image, Connectivity conn);
	
	/**
	 * \brief Erase the next connected component and return its bounding box.
	 *
	 * If there are no black pixels remaining, returns a null ConnComp.
	 */
	ConnComp nextConnComp();
	
	/**
	 * \brief Computes the image of the last connected component
	 *        returned by nextConnComp().
	 *
	 * In case nextConnComp() returned a null component or was never called,
	 * a null BinaryImage is returned.
	 */
	BinaryImage computeConnCompImage() const;
	
	/**
	 * \brief Computes the image of the last connected component
	 *        returned by nextConnComp().
	 *
	 * The image may have some white padding on the left, to make
	 * its left coordinate word-aligned.  This is useful if you
	 * are going to draw the component back to its position.
	 * Word-aligned connected components are faster to both
	 * extract and draw than non-aligned ones.
	 * \param rect If specified, the position and size of the
	 *        aligned image, including padding, will be written into it.
	 *
	 * In case nextConnComp() returned a null component or was never called,
	 * a null BinaryImage is returned.
	 */
	BinaryImage computeConnCompImageAligned(QRect* rect = 0) const;
private:
	ConnCompEraser m_eraser;
	
	BinaryImage computeDiffImage(QRect const& rect) const;
	
	/**
	 * m_lastImage is always one step behind of m_eraser.image().
	 * It contains the last connected component erased from m_eraser.image().
	 */
	BinaryImage m_lastImage;
	ConnComp m_lastCC;
};

} // namespace imageproc

#endif
