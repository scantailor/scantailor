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

#ifndef DEWARPING_DETECT_VERT_CONTENT_BOUNDS_H_
#define DEWARPING_DETECT_VERT_CONTENT_BOUNDS_H_

#include <QLineF>
#include <utility>

class DebugImages;

namespace imageproc
{
	class BinaryImage;
}

namespace dewarping
{

/**
 * \brief Detect the left and right content boundaries.
 *
 * \param image The image to work on.
 * \return A pair of left, right boundaries.  These lines will span
 *         from top to bottom of the image, and may be partially or even
 *         completely outside of its bounds.
 *
 * \note This function assumes a clean image, that is no clutter
 *       or speckles, at least not outside of the content area.
 */
std::pair<QLineF, QLineF> detectVertContentBounds(
	imageproc::BinaryImage const& image, DebugImages* dbg);

} // namespace dewarping

#endif
