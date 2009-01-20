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

#ifndef IMAGEPROC_ADJUST_BRIGHTNESS_H_
#define IMAGEPROC_ADJUST_BRIGHTNESS_H_

class QImage;

namespace imageproc
{

/**
 * \brief Recalculates every pixel to make its brightness match the provided level.
 *
 * \param rgb_image The image to adjust brightness in.  Must be
 *        QImage::Format_RGB32 or QImage::Format_ARGB32.
 *        The alpha channel won't be modified.
 * \param brightness A grayscale image representing the desired brightness
 *        of each pixel.  Must be QImage::Format_Indexed8, have a grayscale
 *        palette and have the same size as \p rgb_image.
 * \param wr The weighting factor for red color component in the brightness
 *        image.
 * \param wb The weighting factor for blue color component in the brightness
 *        image.
 *
 * The brightness values are normally a weighted sum of red, green and blue
 * color components.  The formula is:
 * \code
 * brightness = R * wr + G * wg + B * wb;
 * \endcode
 * This function takes wr and wb arguments, and calculates wg as 1.0 - wr - wb.
 */
void adjustBrightness(
	QImage& rgb_image, QImage const& brightness, double wr, double wb);

/**
 * \brief A custom version of adjustBrightness().
 *
 * Same as adjustBrightness(), but the weighting factors used in the YUV
 * color space are assumed.
 */
void adjustBrightnessYUV(QImage& rgb_image, QImage const& brightness);

/**
 * \brief A custom version of adjustBrightness().
 *
 * Same as adjustBrightness(), but the weighting factors used by
 * toGrayscale() and qGray() are assumed.
 */
void adjustBrightnessGrayscale(QImage& rgb_image, QImage const& brightness);

} // namespace imageproc

#endif
