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

#ifndef IMAGEPROC_SEEDFILL_H_
#define IMAGEPROC_SEEDFILL_H_

#include "BinaryImage.h"
#include "Connectivity.h"

class QImage;

namespace imageproc
{

class GrayImage;

/**
 * \brief Spread black pixels from seed as long as mask allows it.
 *
 * This operation retains black connected components from \p mask that are
 * tagged by at least one black pixel in \p seed.  The rest do not appear
 * in the result.
 * \par
 * \p seed is allowed to contain black pixels that are not in \p mask.
 * They will be ignored and will not appear in the resulting image.
 * \par
 * The underlying code implements Luc Vincent's iterative seed-fill
 * algorithm: http://www.vincent-net.com/luc/papers/93ieeeip_recons.pdf
 */
BinaryImage seedFill(
	BinaryImage const& seed, BinaryImage const& mask,
	Connectivity connectivity);

/**
 * \brief Spread darker colors from seed as long as mask allows it.
 *
 * The result of this operation is an image where some areas are lighter
 * than in \p mask, because there were no dark paths linking them to dark
 * areas in \p seed.
 * \par
 * \p seed is allowed to contain pixels darker than the corresponding pixels
 * in \p mask.  Such pixels will be made equal to their mask values.
 * \par
 * The underlying code implements Luc Vincent's hybrid seed-fill algorithm:
 * http://www.vincent-net.com/luc/papers/93ieeeip_recons.pdf
 */
GrayImage seedFillGray(
	GrayImage const& seed, GrayImage const& mask, Connectivity connectivity);

/**
 * \brief A faster, in-place version of seedFillGray().
 */
void seedFillGrayInPlace(
	GrayImage& seed, GrayImage const& mask, Connectivity connectivity);

/**
 * \brief A slower but more simple implementation of seedFillGray().
 *
 * This function should not be used for anything but testing the correctness
 * of the fast and complex implementation that is seedFillGray().
 */
GrayImage seedFillGraySlow(
	GrayImage const& seed, GrayImage const& mask, Connectivity connectivity);

} // namespace imageproc

#endif
