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


#ifndef IMAGEPROC_ORTHOGONAL_ROTATION_H_
#define IMAGEPROC_ORTHOGONAL_ROTATION_H_

class QRect;

namespace imageproc
{

class BinaryImage;

/**
 * \brief Rotation by 0, 90, 180 or 270 degrees.
 *
 * \param src The source image.  May be null, in which case
 *        a null rotated image will be returned.
 * \param src_rect The area that is to be rotated.
 * \param degrees The rotation angle in degrees.  The angle
 *        must be a multiple of 90.  Positive values indicate
 *        clockwise rotation.
 * \return The rotated area of the source image.  The dimensions
 *         of the returned image will correspond to \p src_rect,
 *         possibly with width and height swapped.
 */
BinaryImage orthogonalRotation(
	BinaryImage const& src, QRect const& src_rect, int degrees);

/**
 * \brief Rotation by 90, 180 or 270 degrees.
 *
 * This is an overload provided for convenience.
 * It rotates the whole image, not a portion of it.
 */
BinaryImage orthogonalRotation(BinaryImage const& src, int degrees);

} // namespace imageproc

#endif
