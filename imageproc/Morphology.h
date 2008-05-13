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

#ifndef IMAGEPROC_MORPHOLOGY_H_
#define IMAGEPROC_MORPHOLOGY_H_

#include "BWColor.h"

class QSize;
class QRect;
class QPoint;

namespace imageproc
{

class BinaryImage;

class Brick
{
public:
	/**
	 * \brief Constructs a brick with origin at the center.
	 */
	Brick(QSize const& size);
	
	/**
	 * \brief Constructs a brick with origin specified relative to its size.
	 *
	 * For example, a 3x3 brick with origin at the center would be
	 * constructed as follows:
	 * \code
	 * Brick brick(QSize(3, 3), QPoint(1, 1));
	 * \endcode
	 * \note Origin doesn't have to be inside the brick.
	 */
	Brick(QSize const& size, QPoint const& origin);
	
	/**
	 * \brief Get the minimum (inclusive) X offset from the origin.
	 */
	int minX() const { return m_minX; }
	
	/**
	 * \brief Get the maximum (inclusive) X offset from the origin.
	 */
	int maxX() const { return m_maxX; }
	
	/**
	 * \brief Get the minimum (inclusive) Y offset from the origin.
	 */
	int minY() const { return m_minY; }
	
	/**
	 * \brief Get the maximum (inclusive) Y offset from the origin.
	 */
	int maxY() const { return m_maxY; }
	
	int width() const { return m_maxX - m_minX + 1; }
	
	int height() const { return m_maxY - m_minY + 1; }
	
	bool isEmpty() const { return m_minX > m_maxX || m_minY > m_maxY; }
	
	/**
	 * \brief Flips the brick both horizontally and vertically around the origin.
	 */
	void flip();
private:
	int m_minX;
	int m_maxX;
	int m_minY;
	int m_maxY;
};


/**
 * \brief Turn every black pixel into a brick of black pixels.
 *
 * \param src The source image.
 * \param brick The brick to turn each black pixel into.
 * \param dst_area The area in source image coordinates that
 *        will be returned as a destination image. It doesn't have
 *        to fit into the source image area.
 * \param src_surroundings The color of pixels that are assumed to
 *        surround the source image.
 */
BinaryImage dilateBrick(
	BinaryImage const& src, Brick const& brick,
	QRect const& dst_area, BWColor src_surroundings = WHITE);

/**
 * \brief Same as above, but assumes dst_rect == src.rect()
 */
BinaryImage dilateBrick(
	BinaryImage const& src, Brick const& brick,
	BWColor src_surroundings = WHITE);

/**
 * \brief Turn every white pixel into a brick of white pixels.
 *
 * \param src The source image.
 * \param brick The brick to turn each white pixel into.
 * \param dst_area The area in source image coordinates that
 *        will be returned as a destination image. It doesn't have
 *        to fit into the source image area.
 * \param src_surroundings The color of pixels that are assumed to
 *        surround the source image.
 */
BinaryImage erodeBrick(
	BinaryImage const& src, Brick const& brick,
	QRect const& dst_area, BWColor src_surroundings = WHITE);

/**
 * \brief Same as above, but assumes dst_rect == src.rect()
 */
BinaryImage erodeBrick(
	BinaryImage const& src, Brick const& brick,
	BWColor src_surroundings = WHITE);

/**
 * \brief Turn the black areas where the brick doesn't fit, into white.
 *
 * \param src The source image.
 * \param brick The brick to fit into black areas.
 * \param dst_area The area in source image coordinates that
 *        will be returned as a destination image. It doesn't have
 *        to fit into the source image area.
 * \param src_surroundings The color of pixels that are assumed to
 *        surround the source image.  If set to BLACK, a brick will be able
 *        to fit by going partially off-screen (off the source
 *        image area actually).
 */
BinaryImage openBrick(
	BinaryImage const& src, QSize const& brick,
	QRect const& dst_area, BWColor src_surroundings = WHITE);

/**
 * \brief Same as above, but assumes dst_rect == src.rect()
 */
BinaryImage openBrick(
	BinaryImage const& src, QSize const& brick,
	BWColor src_surroundings = WHITE);

/**
 * \brief Turn the white areas where the brick doesn't fit, into black.
 *
 * \param src The source image.
 * \param brick The brick to fit into white areas.
 * \param dst_area The area in source image coordinates that
 *        will be returned as a destination image. It doesn't have
 *        to fit into the source image area.
 * \param src_surroundings The color of pixels that are assumed to
 *        surround the source image.  If set to WHITE, a brick will be able
 *        to fit by going partially off-screen (off the source
 *        image area actually).
 */
BinaryImage closeBrick(
	BinaryImage const& src, QSize const& brick,
	QRect const& dst_area, BWColor src_surroundings = WHITE);

/**
 * \brief Same as above, but assumes dst_rect == src.rect()
 */
BinaryImage closeBrick(
	BinaryImage const& src, QSize const& brick,
	BWColor src_surroundings = WHITE);

} // namespace imageproc

#endif
