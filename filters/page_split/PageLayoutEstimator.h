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

#ifndef PAGE_SPLIT_PAGELAYOUTESTIMATOR_H_
#define PAGE_SPLIT_PAGELAYOUTESTIMATOR_H_

#include "foundation/VirtualFunction.h"
#include "LayoutType.h"
#include <QLineF>
#include <deque>
#include <memory>

class QRect;
class QPoint;
class QImage;
class QTransform;
class ImageTransformation;
class DebugImages;
class Span;

namespace imageproc
{
	class BinaryImage;
	class BinaryThreshold;
}

namespace page_split
{

class PageLayout;

class PageLayoutEstimator
{
public:
	/**
	 * \brief Estimates the page layout according to the provided layout type.
	 *
	 * \param layout_type The type of a layout to detect.  If set to
	 *        something other than Rule::AUTO_DETECT, the returned
	 *        layout will have the same type.
	 * \param input The input image.  Will be converted to grayscale unless
	 *        it's already grayscale.
	 * \param pre_xform The logical transformation applied to the input image.
	 *        The resulting page layout will be in transformed coordinates.
	 * \param bw_threshold The global binarization threshold for the
	 *        input image.
	 * \param dbg An optional sink for debugging images.
	 * \return The estimated PageLayout of type consistent with the
	 *         requested layout type.
	 */
	static PageLayout estimatePageLayout(
		LayoutType layout_type, QImage const& input,
		ImageTransformation const& pre_xform,
		imageproc::BinaryThreshold bw_threshold,
		DebugImages* dbg = 0);
private:
	static std::auto_ptr<PageLayout> tryCutAtFoldingLine(
		LayoutType layout_type, QImage const& input,
		ImageTransformation const& pre_xform, DebugImages* dbg);
		
	static PageLayout cutAtWhitespace(
		LayoutType layout_type, QImage const& input,
		ImageTransformation const& pre_xform,
		imageproc::BinaryThreshold const bw_threshold,
		DebugImages* dbg);
	
	static PageLayout cutAtWhitespaceDeskewed150(
		LayoutType layout_type, int num_pages,
		imageproc::BinaryImage const& input,
		bool left_offcut, bool right_offcut, DebugImages* dbg);
	
	static imageproc::BinaryImage to300DpiBinary(
		QImage const& img, QTransform& xform,
		imageproc::BinaryThreshold threshold);
	
	static imageproc::BinaryImage removeGarbageAnd2xDownscale(
		imageproc::BinaryImage const& image, DebugImages* dbg);
	
	static bool checkForLeftOffcut(imageproc::BinaryImage const& image);
	
	static bool checkForRightOffcut(imageproc::BinaryImage const& image);
	
	static void visualizeSpans(
		DebugImages& dbg, std::deque<Span> const& spans,
		imageproc::BinaryImage const& image, char const* label);
	
	static void removeInsignificantEdgeSpans(std::deque<Span>& spans);
	
	static PageLayout processContentSpansSinglePage(
		LayoutType layout_type,
		std::deque<Span> const& spans,
		int width, int height,
		bool left_offcut, bool right_offcut);
		
	static PageLayout processContentSpansTwoPages(
		LayoutType layout_type,
		std::deque<Span> const& spans,
		int width, int height);
	
	static PageLayout processTwoPagesWithSingleSpan(
		Span const& span, int width, int height);
	
	static QLineF vertLine(double x);
};

} // namespace page_split

#endif
