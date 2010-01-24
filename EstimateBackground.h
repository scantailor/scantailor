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

#ifndef ESTIMATE_BACKGROUND_H_
#define ESTIMATE_BACKGROUND_H_

class ImageTransformation;
class TaskStatus;
class DebugImages;
class QPolygonF;

namespace imageproc
{
	class PolynomialSurface;
	class GrayImage;
}

/**
 * \brief Estimates a grayscale background of a scanned page.
 *
 * \param input The image of a page.
 * \param area_to_consider The area in \p input image coordinates to consider.
 *        The resulting surface will only be valid in that area.
 *        This parameter can be an empty polygon, in which case all of the
 *        \p input image area is considered.
 * \param status The status of a task.  If it's cancelled by another thread,
 *        this function may throw an implementation-defined exception.
 * \param dbg The sink for intermediate images used for debugging purposes.
 *        This argument is optional.
 *
 * This implementation can deal with very complex cases, like a page with
 * a picture covering most of it, but in return, it expects some conditions
 * to be met:
 * -# The orientation must be correct.  To be precise, it can deal with
 *    a more or less vertical folding line, but not a horizontal one.
 * -# The page must have some blank margins.  The margins must be of
 *    a natural origin, or to be precise, there must not be a noticeable
 *    transition between the page background and the margins.
 * -# It's better to provide a single page to this function, not a
 *    two page scan.  When cutting off one of the pages, feel free
 *    to fill areas near the the edges with black.
 * -# This implementation can handle dark surroundings around the page,
 *    provided they touch the edges, but it performs better without them.
 */
imageproc::PolynomialSurface estimateBackground(
	imageproc::GrayImage const& input, QPolygonF const& area_to_consider,
	TaskStatus const& status, DebugImages* dbg = 0);

#endif
