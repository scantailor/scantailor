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

#ifndef IMAGEPROC_SAVGOLFILTER_H_
#define IMAGEPROC_SAVGOLFILTER_H_

class QImage;
class QSize;

namespace imageproc
{

/**
 * \brief Performs a grayscale smoothing using the Savitzky-Golay method.
 *
 * \param src The source image.  It doesn't have to be grayscale, but
 *        the resulting image will be grayscale anyway.
 * \param window_size The apperture size.  The restriction on the
 *        apperture is that its area (width*height) must exceed
 *        (order + 1)^2.  If a window doesn't fit completely into
 *        the image, no filtering will be performed.
 * \param order The order of a polynomial.  At 300 dpi good results
 *        are obtained from order 4 and 7x7 window.
 * \return The filtered grayscale image.
 */
QImage savGolFilter(QImage const& src, QSize const& window_size, int order);

} // namespace imageproc

#endif
