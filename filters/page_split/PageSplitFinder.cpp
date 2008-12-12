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

#include "PageSplitFinder.h"
#include "PageLayout.h"
#include "OrthogonalRotation.h"
#include "VertLineFinder.h"
#include "ContentSpanFinder.h"
#include "DebugImages.h"
#include "Dpi.h"
#include "Dpm.h"
#include "ImageTransformation.h"
#include "foundation/Span.h"
#include "imageproc/Binarize.h"
#include "imageproc/BinaryThreshold.h"
#include "imageproc/BWColor.h"
#include "imageproc/Morphology.h"
#include "imageproc/Connectivity.h"
#include "imageproc/SeedFill.h"
#include "imageproc/ReduceThreshold.h"
#include "imageproc/ConnComp.h"
#include "imageproc/ConnCompEraserExt.h"
#include "imageproc/Connectivity.h"
#include "imageproc/SkewFinder.h"
#include "imageproc/Constants.h"
#include "imageproc/RasterOp.h"
#include "imageproc/Shear.h"
#include "imageproc/OrthogonalRotation.h"
#include "imageproc/Scale.h"
#include "imageproc/SlicedHistogram.h"
#include "imageproc/Transform.h"
#include "imageproc/Grayscale.h"
#include <boost/foreach.hpp>
#include <boost/lambda/lambda.hpp>
#include <boost/lambda/bind.hpp>
#include <QRect>
#include <QSize>
#include <QImage>
#include <QPointF>
#include <QPoint>
#include <QPainter>
#include <QColor>
#include <QPen>
#include <QBrush>
#include <QTransform>
#include <QtGlobal>
#include <QDebug>
#include <vector>
#include <utility>
#include <algorithm>
#include <limits>
#include <math.h>
#include <stdint.h>
#include <assert.h>

namespace page_split
{

using namespace imageproc;

namespace
{

struct CenterComparator
{
	bool operator()(QLineF const& line1, QLineF const& line2) const {
		double const line1_x_center = 0.5 * (line1.p1().x() + line1.p2().x());
		double const line2_x_center = 0.5 * (line2.p1().x() + line2.p2().x());
		return line1_x_center < line2_x_center;
	}
};

PageLayout selectSinglePageSplitLine(
	std::vector<QLineF> const& ltr_lines, QSize const& image_size,
	QImage const& hor_shadows, DebugImages* dbg)
{
	if (dbg) {
		dbg->add(hor_shadows, "hor_shadows");
	}
	
	if (ltr_lines.empty()) {
		return PageLayout();
	}
	
	QRect left_area(hor_shadows.rect());
	left_area.setWidth(std::min(20, hor_shadows.width()));
	QRect right_area(left_area);
	right_area.moveRight(hor_shadows.rect().right());
	
	BinaryImage const hor_shadows_bin(binarizeOtsu(hor_shadows));
	if (dbg) {
		dbg->add(hor_shadows_bin, "hor_shadows_bin");
	}
	unsigned const left_sum = hor_shadows_bin.countBlackPixels(left_area);
	unsigned const right_sum = hor_shadows_bin.countBlackPixels(right_area);
	
	if (left_sum == 0 && right_sum == 0) {
		// Looks like this scan doesn't have a horizontal shadow that
		// touches the left or the right edge.  This probably means it
		// doesn't have a split line there as well, and those that
		// we found are false positives.
		return PageLayout();
	}
	
	if (ltr_lines.size() == 1) {
		QLineF const& line = ltr_lines.front();
		double const x_center = 0.5 * (line.p1().x() + line.p2().x());
		if (x_center < 0.5 * image_size.width()) {
			return PageLayout(line, false, true);
		} else {
			return PageLayout(line, true, false);
		}
	}
	
	if (left_sum > right_sum) {
		// Probably the horizontal shadow from a page touches the left
		// border, which means that the left page was cut off.
		return PageLayout(ltr_lines.front(), false, true);
	} else {
		return PageLayout(ltr_lines.back(), true, false);
	}
}

PageLayout selectTwoPageSplitLine(
	std::vector<QLineF> const& ltr_lines, QSize const& image_size)
{
	int const width = image_size.width();
	
	if (ltr_lines.empty()) {
		return PageLayout();
	} else if (ltr_lines.size() == 1) {
		return PageLayout(ltr_lines.front(), true, true);
	}
	
	// Find the line closest to the center.
	double const global_center = 0.5 * width;
	double min_distance = std::numeric_limits<double>::max();
	QLineF const* best_line = 0;
	BOOST_FOREACH (QLineF const& line, ltr_lines) {
		double const line_center = 0.5 * (line.p1().x() + line.p2().x());
		double const distance = fabs(line_center - global_center);
		if (distance < min_distance) {
			min_distance = distance;
			best_line = &line;
		}
	}
	
	return PageLayout(*best_line, true, true);
}

} // anonymous namespace


PageLayout
PageSplitFinder::findSplitLine(
	QImage const& input, ImageTransformation const& pre_xform,
	BinaryThreshold const bw_threshold,
	bool const single_page, DebugImages* dbg)
{
	PageLayout layout(
		splitPagesByFindingVerticalLines(
			input, pre_xform, bw_threshold, single_page, dbg
		)
	);
	
	if (layout.isNull()) {
		layout = splitPagesByFindingVerticalWhitespace(
			input, pre_xform, bw_threshold, single_page, dbg
		);
	}
	
	return layout;
}

PageLayout
PageSplitFinder::splitPagesByFindingVerticalWhitespace(
	QImage const& input, ImageTransformation const& pre_xform,
	BinaryThreshold const bw_threshold,
	bool const single_page, DebugImages* dbg)
{
	QTransform xform;
	
	// Convert to B/W and rotate.
	BinaryImage img(to300DpiBinary(input, xform, bw_threshold));
	img = orthogonalRotation(img, pre_xform.preRotation().toDegrees());
	if (dbg) {
		dbg->add(img, "bw300");
	}
	
	img = removeGarbageAnd2xDownscale(img, dbg);
	xform.scale(0.5, 0.5);
	if (dbg) {
		dbg->add(img, "no_garbage");
	}
	
	// From now on we work with 150 dpi images.
	
	bool const left_cutoff = checkForLeftCutoff(img);
	bool const right_cutoff = checkForRightCutoff(img);
	
	SkewFinder skew_finder;
	// We work with 150dpi image, so no further reduction.
	skew_finder.setCoarseReduction(0);
	skew_finder.setFineReduction(0);
	skew_finder.setDesiredAccuracy(0.5); // fine accuracy is not required.
	Skew const skew(skew_finder.findSkew(img));
	if (skew.angle() != 0.0 && skew.confidence() >= Skew::GOOD_CONFIDENCE) {
		int const w = img.width();
		int const h = img.height();
		double const angle_deg = skew.angle();
		double const tg = tan(angle_deg * constants::DEG2RAD);
		
		int const margin = (int)ceil(fabs(0.5 * h * tg));
		int const new_width = w - margin * 2;
		if (new_width > 0) {
			hShearInPlace(img, tg, 0.5 * h, WHITE);
			BinaryImage new_img(new_width, h);
			rasterOp<RopSrc>(new_img, new_img.rect(), img, QPoint(margin, 0));
			img.swap(new_img);
			if (dbg) {
				dbg->add(img, "shear_applied");
			}
			
			QTransform t1;
			t1.translate(-0.5 * w, -0.5 * h);
			QTransform t2;
			t2.shear(tg, 0.0);
			QTransform t3;
			t3.translate(0.5 * w - margin, 0.5 * h);
			xform = xform * t1 * t2 * t3;
		}
	}
	
	PageLayout const layout(
		findSplitLineDeskewed(
			img, single_page, left_cutoff, right_cutoff, dbg
		)
	);
	return layout.transformed(xform.inverted());
}

PageLayout
PageSplitFinder::splitPagesByFindingVerticalLines(
	QImage const& input, ImageTransformation const& pre_xform,
	imageproc::BinaryThreshold bw_threshold,
	bool const single_page, DebugImages* dbg)
{
	QImage hor_shadows;
	
	int const max_lines = 8;
	std::vector<QLineF> lines(
		VertLineFinder::findLines(
			input, pre_xform, max_lines, dbg,
			single_page ? &hor_shadows : 0
		)
	);
	
	if (lines.empty()) {
		return PageLayout();
	}
	
	std::sort(lines.begin(), lines.end(), CenterComparator());
	
	if (single_page) {
		return selectSinglePageSplitLine(
			lines, input.size(), hor_shadows, dbg
		);
	} else {
		return selectTwoPageSplitLine(lines, input.size());
	}
}

imageproc::BinaryImage
PageSplitFinder::to300DpiBinary(
	QImage const& img, QTransform& xform,
	BinaryThreshold const binary_threshold)
{
	double const xfactor = (300.0 * constants::DPI2DPM) / img.dotsPerMeterX();
	double const yfactor = (300.0 * constants::DPI2DPM) / img.dotsPerMeterY();
	if (fabs(xfactor - 1.0) < 0.1 && fabs(yfactor - 1.0) < 0.1) {
		return BinaryImage(img, binary_threshold);
	}
	
	QTransform scale_xform;
	scale_xform.scale(xfactor, yfactor);
	xform *= scale_xform;
	QSize const new_size(
		(int)ceil(xfactor * img.width()),
		(int)ceil(yfactor * img.height())
	);
	
	QImage const new_image(scaleToGray(img, new_size));
	return BinaryImage(new_image, binary_threshold);
}

BinaryImage
PageSplitFinder::removeGarbageAnd2xDownscale(
	BinaryImage const& image, DebugImages* dbg)
{
	BinaryImage reduced(ReduceThreshold(image)(2));
	if (dbg) {
		dbg->add(reduced, "reduced");
	}
	
	// Remove anything not connected to a bar of at least 4 pixels long.
	BinaryImage non_garbage_seed(openBrick(reduced, QSize(4, 1)));
	BinaryImage non_garbage_seed2(openBrick(reduced, QSize(1, 4)));
	rasterOp<RopOr<RopSrc, RopDst> >(non_garbage_seed, non_garbage_seed2);
	non_garbage_seed2.release();
	reduced = seedFill(non_garbage_seed, reduced, CONN8);
	non_garbage_seed.release();
	
	if (dbg) {
		dbg->add(reduced, "garbage_removed");
	}
	
	BinaryImage hor_seed(openBrick(reduced, QSize(200, 14), BLACK));
	BinaryImage ver_seed(openBrick(reduced, QSize(14, 300), BLACK));
	
	rasterOp<RopOr<RopSrc, RopDst> >(hor_seed, ver_seed);
	BinaryImage seed(hor_seed.release());
	ver_seed.release();
	if (dbg) {
		dbg->add(seed, "shadows_seed");
	}
	
	BinaryImage dilated(dilateBrick(reduced, QSize(3, 3)));
	
	BinaryImage shadows_dilated(seedFill(seed, dilated, CONN8));
	dilated.release();
	if (dbg) {
		dbg->add(shadows_dilated, "shadows_dilated");
	}
	
	rasterOp<RopSubtract<RopDst, RopSrc> >(reduced, shadows_dilated);
	return reduced;
}

bool
PageSplitFinder::checkForLeftCutoff(BinaryImage const& image)
{
	int const margin = 2; // Some scanners leave garbage near page borders.
	int const width = 3;
	QRect rect(margin, 0, width, image.height());
	rect.adjust(0, margin, 0, -margin);
	return image.countBlackPixels(rect) != 0;
}

bool
PageSplitFinder::checkForRightCutoff(BinaryImage const& image)
{
	int const margin = 2; // Some scanners leave garbage near page borders.
	int const width = 3;
	QRect rect(image.width() - margin - width, 0, width, image.height());
	rect.adjust(0, margin, 0, -margin);
	return image.countBlackPixels(rect) != 0;
}

PageLayout
PageSplitFinder::findSplitLineDeskewed(
	BinaryImage const& input, bool const single_page,
	bool const left_cutoff, bool const right_cutoff,
	DebugImages* dbg)
{
	using namespace boost::lambda;
	
	int const width = input.width();
	int const height = input.height();
	
	BinaryImage cc_img(input.size(), WHITE);

	{
		ConnCompEraser cc_eraser(input, CONN8);
		ConnComp cc;
		while (!(cc = cc_eraser.nextConnComp()).isNull()) {
			if (cc.width() < 5 || cc.height() < 5) {
				continue;
			}
			if ((double)cc.height() / cc.width() > 6) {
				continue;
			}
			cc_img.fill(cc.rect(), BLACK);
		}
	}
	
	if (dbg) {
		dbg->add(cc_img, "cc_img");
	}
	
	ContentSpanFinder span_finder;
	span_finder.setMinContentWidth(2);
	span_finder.setMinWhitespaceWidth(8);
	
	std::deque<Span> spans;
	SlicedHistogram hist(cc_img, SlicedHistogram::COLS);
	span_finder.find(hist, bind(&std::deque<Span>::push_back, var(spans), _1));
	
	if (dbg) {
		visualizeSpans(*dbg, spans, input, "spans");
	}
	
	if (single_page) {
		return processContentSpansSinglePage(
			spans, width, height, left_cutoff, right_cutoff
		);
	} else {
		// This helps if we have 2 pages with one page containing nothing
		// but a small amount of garbage.
		removeInsignificantEdgeSpans(spans);
		if (dbg) {
			visualizeSpans(*dbg, spans, input, "spans_refined");
		}
		
		return processContentSpansTwoPages(spans, width, height);
	}
}

void
PageSplitFinder::visualizeSpans(
	DebugImages& dbg, std::deque<Span> const& spans,
	BinaryImage const& image, char const* label)
{
	int const height = image.height();
	
	QImage spans_img(
		image.toQImage().convertToFormat(
			QImage::Format_ARGB32_Premultiplied
		)
	);
	
	{
		QPainter painter(&spans_img);
		QBrush const brush(QColor(0xff, 0x00, 0x00, 0x50));
		BOOST_FOREACH(Span const& span, spans) {
			QRect const rect(span.begin(), 0, span.width(), height);
			painter.fillRect(rect, brush);
		}
	}
	dbg.add(spans_img, label);
}

void
PageSplitFinder::removeInsignificantEdgeSpans(std::deque<Span>& spans)
{
	if (spans.empty()) {
		return;
	}
	
	// GapInfo.first: the amount of content preceding this gap.
	// GapInfo.second: the amount of content following this gap.
	typedef std::pair<int, int> GapInfo;
	
	std::vector<GapInfo> gaps(spans.size() - 1);
	
	int sum = 0;
	for (unsigned i = 0; i < gaps.size(); ++i) {
		sum += spans[i].width();
		gaps[i].first = sum;
	}
	sum = 0;
	for (int i = gaps.size() - 1; i >= 0; --i) {
		sum += spans[i + 1].width();
		gaps[i].second = sum;
	}
	int const total = sum + spans[0].width();
	
	int may_be_removed = total / 15;
	
	do {
		Span const& first = spans.front();
		Span const& last = spans.back();
		if (&first == &last) {
			break;
		}
		if (first.width() < last.width()) {
			if (first.width() > may_be_removed) {
				break;
			}
			may_be_removed -= first.width();
			spans.pop_front();
		} else {
			if (last.width() > may_be_removed) {
				break;
			}
			may_be_removed -= last.width();
			spans.pop_back();
		}
	} while (!spans.empty());
}

PageLayout
PageSplitFinder::processContentSpansSinglePage(
	std::deque<Span> const& spans, int const width, int const height,
	bool const left_cutoff, bool const right_cutoff)
{
	// Just to be able to break from it.
	while (left_cutoff && !right_cutoff) {
		double x;
		if (spans.empty()) {
			x = 0;
		} else if (spans.front().begin() > 0) {
			x = 0.5 * spans.front().begin();
		} else {
			if (spans.front().width() > width / 2) {
				break;
			} else if (spans.size() > 1) {
				x = Span(spans[0], spans[1]).center();
			} else {
				x = std::min(spans[0].end() + 20, width);
			}
		}
		return PageLayout(vertLine(x), false, true);
	}
	
	// Just to be able to break from it.
	while (right_cutoff && !left_cutoff) {
		double x;
		if (spans.empty()) {
			x = width;
		} else if (spans.back().end() < width) {
			x = Span(spans.back(), width).center();
		} else {
			if (spans.back().width() > width / 2) {
				break;
			} else if (spans.size() > 1) {
				x = Span(spans[spans.size() - 2], spans.back()).center();
			} else {
				x = std::max(spans.back().begin() - 20, 0);
			}
		}
		return PageLayout(vertLine(x), true, false);
	}
	
	if (spans.empty()) {
		return PageLayout(vertLine(0), false, true);
	} else {
		if (spans.front().begin() < width - spans.back().end()) {
			return PageLayout(vertLine(0), false, true);
		} else {
			return PageLayout(vertLine(width), true, false);
		}
	}
}

PageLayout
PageSplitFinder::processContentSpansTwoPages(
	std::deque<Span> const& spans, int const width, int const height)
{
	double x;
	if (spans.empty()) {
		x = 0.5 * width;
	} else if (spans.size() == 1) {
		return processTwoPagesWithSingleSpan(spans.front(), width);
	} else {
		// GapInfo.first: the amount of content preceding this gap.
		// GapInfo.second: the amount of content following this gap.
		typedef std::pair<int, int> GapInfo;
		
		std::vector<GapInfo> gaps(spans.size() - 1);
#if 0
		int sum = 0;
		for (unsigned i = 0; i < gaps.size(); ++i) {
			sum += spans[i].width();
			gaps[i].first = sum;
		}
		sum = 0;
		for (int i = gaps.size() - 1; i >= 0; --i) {
			sum += spans[i + 1].width();
			gaps[i].second = sum;
		}
#else
		int const content_begin = spans.front().begin();
		int const content_end = spans.back().end();
		for (unsigned i = 0; i < gaps.size(); ++i) {
			gaps[i].first = spans[i].end() - content_begin;
			gaps[i].second = content_end - spans[i + 1].begin();
		}
#endif
		
		int best_gap = 0;
		double best_ratio = 0;
		for (unsigned i = 0; i < gaps.size(); ++i) {
			double const min = std::min(gaps[i].first, gaps[i].second);
			double const max = std::max(gaps[i].first, gaps[i].second);
			double const ratio = min / max;
			if (ratio > best_ratio) {
				best_ratio = ratio;
				best_gap = i;
			}
		}
		
		if (best_ratio < 0.25) {
			// Probably one of the pages is just empty.
			return processTwoPagesWithSingleSpan(
				Span(content_begin, content_end), width
			);
		}
		
		double const acceptable_ratio = best_ratio * 0.90;
		
		int widest_gap = best_gap;
		int max_width = Span(spans[best_gap], spans[best_gap + 1]).width();
		for (int i = best_gap - 1; i >= 0; --i) {
			double const min = std::min(gaps[i].first, gaps[i].second);
			double const max = std::max(gaps[i].first, gaps[i].second);
			double const ratio = min / max;
			if (ratio < acceptable_ratio) {
				break;
			}
			int const width = Span(spans[i], spans[i + 1]).width();
			if (width > max_width) {
				max_width = width;
				widest_gap = i;
			}
		}
		for (unsigned i = best_gap + 1; i < gaps.size(); ++i) {
			double const min = std::min(gaps[i].first, gaps[i].second);
			double const max = std::max(gaps[i].first, gaps[i].second);
			double const ratio = min / max;
			if (ratio < acceptable_ratio) {
				break;
			}
			int const width = Span(spans[i], spans[i + 1]).width();
			if (width > max_width) {
				max_width = width;
				widest_gap = i;
			}
		}
		
		Span const gap(spans[widest_gap],  spans[widest_gap + 1]);
		x = gap.center();
	}
	return PageLayout(vertLine(x), true, true);
}

PageLayout
PageSplitFinder::processTwoPagesWithSingleSpan(Span const& span, int width)
{
	double const page_center = 0.5 * width;
	double const box_center = span.center();
	double const box_half_width = 0.5 * span.width();
	double const distance_to_page_center =
		fabs(page_center - box_center) - box_half_width;
	
	double x;
	
	if (distance_to_page_center > 15) {
		x = page_center;
	} else {
		Span const left_ws(0, span);
		Span const right_ws(span, width);
		if (left_ws.width() > right_ws.width()) {
			x = std::max(0, span.begin() - 15);
		} else {
			x = std::min(width, span.end() + 15);
		}
	}
	
	return PageLayout(vertLine(x), true, true);
}

QLineF
PageSplitFinder::vertLine(double x)
{
	return QLineF(x, 0.0, x, 1.0);
}

} // namespace page_split
