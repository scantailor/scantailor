/*
    Scan Tailor - Interactive post-processing tool for scanned pages.
    Copyright (C) 2007-2009  Joseph Artsimovich <joseph_a@mail.ru>
    Copyright (C) 2012  Petr Kovar <pejuko@gmail.com>

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

#ifndef SELECT_CONTENT_PAGEFINDER_H_
#define SELECT_CONTENT_PAGEFINDER_H_

#include "imageproc/BinaryThreshold.h"

#include <Qt>

class TaskStatus;
class DebugImages;
class FilterData;
class QImage;
class QRect;
class QRectF;

namespace imageproc
{
	class BinaryImage;
}

namespace select_content
{

class PageFinder
{
public:
	static QRectF findPageBox(
		TaskStatus const& status, FilterData const& data, bool fine_tune=false,
		DebugImages* dbg = 0);
private:
	static QRect detectBorders(QImage const& img);
	static int detectEdge(QImage const& img, int start, int end, int inc, int mid, Qt::Orientation orient);
	static void fineTuneCorners(QImage const& img, QRect &rect);
	static void fineTuneCorner(QImage const& img, int &x, int &y, int inc_x, int inc_y);
};

} // namespace select_content

#endif
