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

#ifndef PAGE_SPLIT_PAGESPLITFINDER_H_
#define PAGE_SPLIT_PAGESPLITFINDER_H_

#include "foundation/VirtualFunction.h"
#include <QLineF>
#include <deque>

class QRect;
class QPoint;
class QImage;
class QTransform;
class ImageTransformation;
class DebugImages;
class Span;
class PageLayout;

namespace imageproc
{
	class BinaryImage;
	class BinaryThreshold;
	class SlicedHistogram;
}

namespace page_split
{

class PageSplitFinder
{
public:
	static PageLayout findSplitLine(
		QImage const& input, ImageTransformation const& pre_xform,
		imageproc::BinaryThreshold bw_threshold,
		bool single_page, DebugImages* dbg = 0);
private:
	static PageLayout splitPagesByFindingVerticalLines(
		QImage const& input, ImageTransformation const& pre_xform,
		imageproc::BinaryThreshold bw_threshold,
		bool single_page, DebugImages* dbg);
	
	static PageLayout splitPagesByFindingVerticalWhitespace(
		QImage const& input, ImageTransformation const& pre_xform,
		imageproc::BinaryThreshold const bw_threshold,
		bool const single_page, DebugImages* dbg);
	
	static imageproc::BinaryImage to300DpiBinary(
		QImage const& img, QTransform& xform,
		imageproc::BinaryThreshold threshold);
	
	static imageproc::BinaryImage removeGarbageAnd2xDownscale(
		imageproc::BinaryImage const& image, DebugImages* dbg);
	
	static bool checkForLeftCutoff(imageproc::BinaryImage const& image);
	
	static bool checkForRightCutoff(imageproc::BinaryImage const& image);
	
	static PageLayout findSplitLineDeskewed(
		imageproc::BinaryImage const& input, bool single_page,
		bool left_cutoff, bool right_cutoff, DebugImages* dbg);
	
	static void visualizeSpans(
		DebugImages& dbg, std::deque<Span> const& spans,
		imageproc::BinaryImage const& image, char const* label);
	
	static void removeInsignificantEdgeSpans(std::deque<Span>& spans);
	
	static PageLayout processContentSpansSinglePage(
		std::deque<Span> const& spans,
		int width, int height,
		bool left_cutoff, bool right_cutoff);
		
	static PageLayout processContentSpansTwoPages(
		std::deque<Span> const& spans,
		int width, int height);
	
	static PageLayout processTwoPagesWithSingleSpan(
		Span const& span, int width);
	
	static QLineF vertLine(double x);
};

} // namespace page_split

#endif
