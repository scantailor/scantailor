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

#ifndef SELECT_CONTENT_CONTENTBOXFINDER_H_
#define SELECT_CONTENT_CONTENTBOXFINDER_H_

#include "imageproc/BinaryThreshold.h"

class TaskStatus;
class DebugImages;
class FilterData;
class QImage;
class QRect;
class QRectF;

namespace imageproc
{
	class BinaryImage;
	class ConnComp;
}

namespace select_content
{

class ContentBoxFinder
{
public:
	static QRectF findContentBox(
		TaskStatus const& status, FilterData const& data,
		DebugImages* dbg = 0);
private:
	static void filterShadows(
		TaskStatus const& status, imageproc::BinaryImage& shadows,
		DebugImages* dbg);
	
	static imageproc::BinaryImage getLighterContent(
		QImage const& gray, imageproc::BinaryThreshold reference_threshold,
		imageproc::BinaryImage const& content_mask);
	
	static bool isBigAndDark(imageproc::ConnComp const& cc);
	
	static bool isWindingComponent(imageproc::BinaryImage const& cc_img);
	
	static QRect trimLeftRight(
		imageproc::BinaryImage const& img,
		imageproc::BinaryImage const& img_lighter, QRect const& area);
	
	static QRect trimTopBottom(
		imageproc::BinaryImage const& img,
		imageproc::BinaryImage const& img_lighter, QRect const& area);
	
	static QRect processColumn(
		imageproc::BinaryImage const& img,
		imageproc::BinaryImage const& img_lighter, QRect const& area);
	
	static QRect processRow(
		imageproc::BinaryImage const& img,
		imageproc::BinaryImage const& img_lighter, QRect const& area);
};

} // namespace select_content

#endif
