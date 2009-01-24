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
	class SEDM;
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
	class Garbage;
	
	static void segmentGarbage(
		imageproc::BinaryImage const& garbage,
		imageproc::BinaryImage& hor_garbage,
		imageproc::BinaryImage& vert_garbage,
		DebugImages* dbg);
	
	static void trimContentBlocksInPlace(
		imageproc::BinaryImage const& content,
		imageproc::BinaryImage& content_blocks);
	
	static void inPlaceRemoveAreasTouchingBorders(
		imageproc::BinaryImage& content_blocks, DebugImages* dbg);
	
	static imageproc::BinaryImage estimateTextMask(
		imageproc::BinaryImage const& content,
		imageproc::BinaryImage const& content_blocks,
		DebugImages* dbg);
	
	static void filterShadows(
		TaskStatus const& status, imageproc::BinaryImage& shadows,
		DebugImages* dbg);
	
	static QRect trimLeft(
		imageproc::BinaryImage const& content,
		imageproc::BinaryImage const& content_blocks,
		imageproc::BinaryImage const& text_mask, QRect const& area,
		Garbage& garbage, DebugImages* dbg);
	
	static QRect trimRight(
		imageproc::BinaryImage const& content,
		imageproc::BinaryImage const& content_blocks,
		imageproc::BinaryImage const& text_mask, QRect const& area,
		Garbage& garbage, DebugImages* dbg);
	
	static QRect trimTop(
		imageproc::BinaryImage const& content,
		imageproc::BinaryImage const& content_blocks,
		imageproc::BinaryImage const& text_mask, QRect const& area,
		Garbage& garbage, DebugImages* dbg);
	
	static QRect trimBottom(
		imageproc::BinaryImage const& content,
		imageproc::BinaryImage const& content_blocks,
		imageproc::BinaryImage const& text_mask, QRect const& area,
		Garbage& garbage, DebugImages* dbg);
	
	static QRect trim(
		imageproc::BinaryImage const& content,
		imageproc::BinaryImage const& content_blocks,
		imageproc::BinaryImage const& text_mask,
		QRect const& area, QRect const& new_area,
		QRect const& removed_area, Garbage& garbage,
		bool& can_retry_grouped, DebugImages* dbg);
};

} // namespace select_content

#endif
