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

#ifndef IMAGEPROC_SEEDFILL_H_
#define IMAGEPROC_SEEDFILL_H_

#include "BinaryImage.h"
#include "Connectivity.h"

class QImage;

namespace imageproc
{

BinaryImage seedFill(
	BinaryImage const& seed, BinaryImage const& mask,
	Connectivity connectivity);

/**
 * \brief Spread darker colors from seed as long as mask allows it.
 *
 * The result of this operation is an image where some areas are lighter
 * than in \p mask, because there were no dark path from them to dark
 * areas in \p seed.
 */
QImage seedFillGray(
	QImage const& seed, QImage const& mask, Connectivity connectivity);

} // namespace imageproc

#endif
