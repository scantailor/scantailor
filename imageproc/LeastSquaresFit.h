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

#ifndef IMAGEPROC_LEAST_SQUARES_FIT_H_
#define IMAGEPROC_LEAST_SQUARES_FIT_H_

class QSize;

namespace imageproc
{

/**
 * \brief Solves C * x - d = r, |r| = min!
 *
 * \param C_size Dimensions of the C matrix.
 * \param C The C matrix stored linearly in row-major order. It's contents
 *        won't be preserved.
 * \param x The resulting vector of C_size.width() elements.
 * \param d The d vector of C_size.height() elements.  It's contents won't
 *        be preserved.
 */
void leastSquaresFit(QSize const& C_size, double* C, double* x, double* d);

}

#endif
