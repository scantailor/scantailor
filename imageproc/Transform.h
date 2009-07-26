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

#ifndef IMAGEPROC_TRANSFORM_H_
#define IMAGEPROC_TRANSFORM_H_

#include <QSizeF>

class QImage;
class QRect;
class QTransform;
class QColor;

namespace imageproc
{

/**
 * \brief Apply an affine transformation to the image.
 *
 * \param src The source image.
 * \param xform The transformation from source to destination.
 *        Only affine transformations are supported.
 * \param dst_rect The area in source image coordinates to return
 *        as a destination image.
 * \param background_color Used to fill areas not represented in the source image.
 * \param weak_background If set to true, \p background_color is only taken
 *        into account if a target pixel maps to an area completely outside of
 *        the source image.  That is, if at least one source image pixel
 *        influences a particular target pixel, then any background pixels
 *        that may also influence that target pixel are ignored.\n
 *        If set to false, source image pixels and background pixels are
 *        treated equally.
 * \param min_mapping_area Defines the minimum rectangle in the source image
 *        that maps to a destination pixel.  This can be used to control
 *        smoothing.
 * \return The transformed image.  It's format may differ from the
 *         source image format, for example Format_Indexed8 may
 *         be transformed to Format_RGB32, if the source image
 *         contains colors other than shades of gray.
 */
QImage transform(
	QImage const& src, QTransform const& xform,
	QRect const& dst_rect, QColor const& background_color,
	bool weak_background = false,
	QSizeF const& min_mapping_area = QSizeF(0.9, 0.9));

/**
 * \brief Apply an affine transformation to the image.
 *
 * Same as transform(), except the source image image is converted
 * to grayscale before transforming it.  The resulting image
 * will be grayscale as well.
 */
QImage transformToGray(
	QImage const& src, QTransform const& xform,
	QRect const& dst_rect, QColor const& background_color,
	bool weak_background = false,
	QSizeF const& min_mapping_area = QSizeF(0.9, 0.9));

} // namespace imageproc

#endif
