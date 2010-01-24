/*
    Scan Tailor - Interactive post-processing tool for scanned pages.
	Copyright (C)  Joseph Artsimovich <joseph.artsimovich@gmail.com>

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

#ifndef IMAGEPROC_MORPHGRADIENTDETECT_H_
#define IMAGEPROC_MORPHGRADIENTDETECT_H_

class QSize;

namespace imageproc
{

class GrayImage;

/**
 * \brief Morphological gradient detection.
 *
 * This function finds the the difference between the gray level of a pixel
 * and the gray level of the lightest pixel in its neighborhood, which
 * is specified by the \p area parameter.
 * The DarkSide in the name suggests that given a dark-to-light transition,
 * the gradient will be detected on the dark side.
 * \par
 * There is no requirement for the source image to be grayscale.  A grayscale
 * version will be created transparently, if necessary.
 * \par
 * Smoothing the image before calling this function is often a good idea,
 * especially for black and white images.
 */
GrayImage morphGradientDetectDarkSide(GrayImage const& image, QSize const& area);

/**
 * \brief Morphological gradient detection.
 *
 * This function finds the the difference between the gray level of a pixel
 * and the gray level of the darkest pixel in its neighborhood, which
 * is specified by the \p area parameter.
 * The LightSide in the name suggests that given a dark-to-light transition,
 * the gradient will be detected on the light side.
 * \par
 * There is no requirement for the source image to be grayscale.  A grayscale
 * version will be created transparently, if necessary.
 * \par
 * Smoothing the image before calling this function is often a good idea,
 * especially for black and white images.
 */
GrayImage morphGradientDetectLightSide(GrayImage const& image, QSize const& area);

} // namespace imageproc

#endif
