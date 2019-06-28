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
 * The Savitzky-Golay method is equivalent to fitting a small neighborhood
 * around each pixel to a polynomial, and then recalculating the pixel
 * value from it.  In practice, it achieves the same results without fitting
 * a polynomial for every pixel, so it performs quite well.
 *
 * \param src The source image.  It doesn't have to be grayscale, but
 *        the resulting image will be grayscale anyway.
 * \param window_size The aperture size.  If it doesn't completely
 *        fit the image area, no filtering will take place.
 * \param hor_degree The degree of a polynomial in horizontal direction.
 * \param vert_degree The degree of a polynomial in vertical direction.
 * \return The filtered grayscale image.
 *
 * \note The window size and degrees are not completely independent.
 *       The following inequality must be fulfilled:
 * \code
 *       window_width * window_height >= (hor_degree + 1) * (vert_degree + 1)
 * \endcode
 * Good results for 300 dpi scans are achieved with 7x7 window and 4x4 degree.
 */
QImage savGolFilter(
	QImage const& src, QSize const& window_size,
	int hor_degree, int vert_degree);

} // namespace imageproc

#endif
