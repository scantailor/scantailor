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

#ifndef IMAGEPROC_DRAWOVER_H_
#define IMAGEPROC_DRAWOVER_H_

class QImage;
class QRect;

namespace imageproc
{

/**
 * \brief Overdraws a portion of one image with a portion of another.
 *
 * \param dst The destination image.  Can be in any format, as long
 *        as the source image has the same format.
 * \param dst_rect The area of the destination image to be overdrawn.
 *        This area must lie completely within the destination
 *        image, and its size must match the size of \p src_rect.
 * \param src The source image.  Can be in any format, as long
 *        as the destination image has the same format.
 * \param src_rect The area of the source image to draw over
 *        the destination image.  This area must lie completely
 *        within the source image, and its size must match the
 *        size of \p dst_rect.
 */
void drawOver(
	QImage& dst, QRect const& dst_rect,
	QImage const& src, QRect const& src_rect);

} // namespace imageproc

#endif
