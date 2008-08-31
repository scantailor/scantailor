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

#ifndef IMAGEPROC_MORPHGRADIENTDETECT_H_
#define IMAGEPROC_MORPHGRADIENTDETECT_H_

class QImage;
class QSize;

namespace imageproc
{

/**
 * \brief Morphological gradient detection.
 *
 * The gradient of a pixel is the difference between its gray level
 * and the gray level of the lightest pixel in its neighborhood, which
 * is specified by the \p window parameter.
 * \par
 * There is no requirement for the source image to be grayscale.  A grayscale
 * version will be created transparently, if necessary.
 * \par
 * Smoothing the image before calling this function is often a good idea,
 * especially for black and white images.
 */
QImage morphGradientDetect(QImage const& image, QSize const& window);

} // namespace imageproc

#endif
