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


#ifndef IMAGEPROC_SHEAR_H_
#define IMAGEPROC_SHEAR_H_

#include "BWColor.h"

namespace imageproc
{

class BinaryImage;

/**
 * \brief Horizontal shear.
 *
 * \param src The source image.
 * \param dst The desctination image.
 * \param shear The shift of each next line relative to the previous one.
 * \param y_origin The y value where a line would have a zero shift.
 *        Note that a value of 1.0 doesn't mean that line 1
 *        is to have zero shift.  The value of 1.5 would mean that.
 * \param background_color The color used to fill areas not represented
 *        in the source image.
 * \note The source and destination images must have the same size.
 */
void hShearFromTo(
	BinaryImage const& src, BinaryImage& dst, double shear,
	double y_origin, BWColor background_color);

/**
 * \brief Vertical shear.
 *
 * \param src The source image.
 * \param dst The destination image.
 * \param shear The shift of each next line relative to the previous one.
 * \param x_origin The x value where a line would have a zero shift.
 *        Note that a value of 1.0 doesn't mean that line 1
 *        is to have zero shift.  The value of 1.5 would mean that.
 * \param background_color The color used to fill areas not represented
 *        in the source image.
 * \note The source and destination images must have the same size.
 */
void vShearFromTo(
	BinaryImage const& src, BinaryImage& dst, double shear,
	double x_origin, BWColor background_color);

/**
 * \brief Horizontal shear returning a new image.
 *
 * Same as hShearFromTo(), but creates and returns the destination image.
 */
BinaryImage hShear(
	BinaryImage const& src, double shear,
	double y_origin, BWColor background_color);

/**
 * \brief Vertical shear returning a new image.
 *
 * Same as vShearFromTo(), but creates and returns the destination image.
 */
BinaryImage vShear(
	BinaryImage const& src, double shear,
	double x_origin, BWColor background_color);

/**
 * \brief In-place horizontal shear.
 *
 * Same as hShearFromTo() with src and dst being the same image.
 */
void hShearInPlace(
	BinaryImage& image, double shear,
	double y_origin, BWColor background_color);

/**
 * \brief In-place vertical shear.
 *
 * Same as vShearFromTo() with src and dst being the same image.
 */
void vShearInPlace(
	BinaryImage& image, double shear,
	double x_origin, BWColor background_color);

} // namespace imageproc

#endif
