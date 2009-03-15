/*
    Scan Tailor - Interactive post-processing tool for scanned pages.
    Copyright (C) 2007-2009  Joseph Artsimovich <joseph_a@mail.ru>

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

#ifndef IMAGEPROC_COLORINTERPOLATION_H_
#define IMAGEPROC_COLORINTERPOLATION_H_

#include <QColor>

namespace imageproc
{

/**
 * \brief Returns a color between the provided two.
 *
 * Returns a color between \p from and \p to according to \p dist.
 * \p dist 0 corresponds to \p from, while \p dist 1 corresponds to \p to.
 */
QColor colorInterpolation(QColor const& from, QColor const& to, double dist);

} // namespace imageproc

#endif
