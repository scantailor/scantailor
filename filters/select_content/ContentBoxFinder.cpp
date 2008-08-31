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

#include "ContentBoxFinder.h"
#include "TaskStatus.h"
#include "DebugImages.h"
#include "FilterData.h"
#include "ImageTransformation.h"
#include "ContentSpanFinder.h"
#include "Span.h"
#include "Dpi.h"
#include "imageproc/BinaryImage.h"
#include "imageproc/BinaryThreshold.h"
#include "imageproc/Binarize.h"
#include "imageproc/BWColor.h"
#include "imageproc/Constants.h"
#include "imageproc/Connectivity.h"
#include "imageproc/ConnComp.h"
#include "imageproc/ConnCompEraserExt.h"
#include "imageproc/Transform.h"
#include "imageproc/RasterOp.h"
#include "imageproc/SeedFill.h"
#include "imageproc/Morphology.h"
#include "imageproc/Grayscale.h"
#include "imageproc/SlicedHistogram.h"
#include "imageproc/DentFinder.h"
#include "imageproc/IntegralImage.h"
#include "imageproc/PolygonRasterizer.h"
#include <boost/lambda/lambda.hpp>
#include <boost/lambda/bind.hpp>
#include <QRect>
#include <QRectF>
#include <QPolygonF>
#include <QImage>
#include <QColor>
#include <QPainter>
#include <QTransform>
#include <Qt>
#include <QDebug>
#include <deque>
#include <algorithm>

namespace select_content
{

using namespace imageproc;

static double const LIGHTER_REDUCE_FACTOR = 1.7;
static double const LIGHTER_REDUCE_FACTOR_SAFE = LIGHTER_REDUCE_FACTOR - 0.2;

QRectF
ContentBoxFinder::findContentBox(
	TaskStatus const& status, FilterData const& data, DebugImages* dbg)
{
	ImageTransformation xform_150dpi(data.xform());
	xform_150dpi.preScaleToDpi(Dpi(150, 150));
	
	QColor const black(0x00, 0x00, 0x00);
	QImage const gray150(
		transformToGray(
			data.image(), xform_150dpi.transform(),
			xform_150dpi.resultingRect().toRect(), black
		)
	);
	// Note that we fill new areas that appear as a result of
	// rotation with black, not white.  Filling them with white
	// may be bad for detecting the shadow around the page.
	if (dbg) {
		dbg->add(gray150, "gray150");
	}
	
	status.throwIfCancelled();
	
	//BinaryImage bw150(binarizeWolf(gray150, QSize(91, 91)));
	BinaryImage bw150(gray150, data.bwThreshold());
	if (dbg) {
		dbg->add(bw150, "bw150");
	}
	
	status.throwIfCancelled();
	
	BinaryImage hor_shadows_seed(openBrick(bw150, QSize(200, 14), BLACK));
	if (dbg) {
		dbg->add(hor_shadows_seed, "hor_shadows_seed");
	}
	
	status.throwIfCancelled();
	
	BinaryImage ver_shadows_seed(openBrick(bw150, QSize(14, 300), BLACK));
	if (dbg) {
		dbg->add(ver_shadows_seed, "ver_shadows_seed");
	}
	
	status.throwIfCancelled();
	
	rasterOp<RopOr<RopSrc, RopDst> >(hor_shadows_seed, ver_shadows_seed);
	BinaryImage shadows_seed(hor_shadows_seed.release());
	ver_shadows_seed.release();
	if (dbg) {
		dbg->add(shadows_seed, "shadows_seed");
	}
	
	status.throwIfCancelled();
	
	BinaryImage dilated(dilateBrick(bw150, QSize(3, 3)));
	if (dbg) {
		dbg->add(dilated, "dilated");
	}
	
	status.throwIfCancelled();
	
	BinaryImage shadows_dilated(seedFill(shadows_seed, dilated, CONN8));
	dilated.release();
	if (dbg) {
		dbg->add(shadows_dilated, "shadows_dilated");
	}
	
	status.throwIfCancelled();
	
	rasterOp<RopAnd<RopSrc, RopDst> >(shadows_dilated, bw150);
	BinaryImage shadows(shadows_dilated.release());
	if (dbg) {
		dbg->add(shadows, "shadows");
	}
	
	status.throwIfCancelled();
	
	filterShadows(status, shadows, dbg);
	if (dbg) {
		dbg->add(shadows, "filtered_shadows");
	}
	
	status.throwIfCancelled();
	
	BinaryImage content(bw150.release());
	rasterOp<RopSubtract<RopDst, RopSrc> >(content, shadows);
	shadows.release();
	if (dbg) {
		dbg->add(content, "content");
	}
	
	status.throwIfCancelled();
	
	PolygonRasterizer::fillExcept(
		content, WHITE, xform_150dpi.resultingCropArea(), Qt::WindingFill
	);
	if (dbg) {
		dbg->add(content, "page_mask_applied");
	}
	
	status.throwIfCancelled();
	
	BinaryImage large_seed(openBrick(content, QSize(2, 2)));
	content = seedFill(large_seed, content, CONN8);
	large_seed.release();
	if (dbg) {
		dbg->add(content, "small_garbage_removed");
	}
	
	IntegralImage<unsigned> integral_img(content.width(), content.height());
	for (int y = 0; y < content.height(); ++y) {
		integral_img.beginRow();
		uint32_t const* line = content.data() + content.wordsPerLine() * y;
		for (int x = 0; x < content.width(); ++x) {
			unsigned const bit = (line[x >> 5] >> (31 - (x & 31))) & 1;
			integral_img.push(bit);
		}
	}
	
	QSize const window(50, 50);
	for (int y = 0; y < content.height(); ++y) {
		uint32_t const msb = uint32_t(1) << 31;
		uint32_t* line = content.data() + content.wordsPerLine() * y;
		for (int x = 0; x < content.width(); ++x) {
			QRect rect(QPoint(0, 0), window);
			rect.moveCenter(QPoint(x, y));
			rect &= content.rect();
			unsigned const sum = integral_img.sum(rect);
			uint32_t* word = &line[x >> 5];
			if (sum < window.width() * window.height() * 0.02) {
				*word &= ~(msb >> (x & 31));
			}
		}
	}
	if (dbg) {
		dbg->add(content, "remote_removed");
	}
	
	status.throwIfCancelled();
	
	BinaryImage content_lighter(
		getLighterContent(gray150, data.bwThreshold(), content)
	);
	if (dbg) {
		dbg->add(content_lighter, "content_lighter");
	}
	
	status.throwIfCancelled();
	
	if (dbg) {
		BinaryImage garbage(content.size(), WHITE);
		BinaryImage non_garbage(content.size(), WHITE);
		BinaryImage big_dark(content.size(), WHITE);
		
		ConnCompEraserExt eraser(content, CONN8);
		for (ConnComp cc; !(cc = eraser.nextConnComp()).isNull(); ) {
			if (cc.pixCount() < 10 || (cc.width() < 5 && cc.height() < 5)) {
				// Too small.  Probably garbage.
				continue;
			}
			
			BinaryImage const cc_img(eraser.computeConnCompImage());
			
			if (isBigAndDark(cc)) {
				rasterOp<RopOr<RopSrc, RopDst> >(big_dark, cc.rect(), cc_img, QPoint(0, 0));
			}
			
			if (!isWindingComponent(cc_img)) {
				// The connected component doesn't wind that much.
				// It must be a blot or a stroke. We consider such things as garbage.
				
				rasterOp<RopOr<RopSrc, RopDst> >(garbage, cc.rect(), cc_img, QPoint(0, 0));
				continue;
			}
			
			rasterOp<RopOr<RopSrc, RopDst> >(non_garbage, cc.rect(), cc_img, QPoint(0, 0));
		}
		
		QImage garbage_red(garbage.size(), QImage::Format_ARGB32_Premultiplied);
		garbage_red.fill(qRgb(0xff, 0x00, 0x00));
		garbage_red.setAlphaChannel(garbage.inverted().toQImage());
		
		QImage non_garbage_green(non_garbage.size(), QImage::Format_ARGB32_Premultiplied);
		non_garbage_green.fill(qRgb(0x00, 0xff, 0x00));
		non_garbage_green.setAlphaChannel(non_garbage.inverted().toQImage());
		
		QImage dent_stats(content.size(), QImage::Format_ARGB32_Premultiplied);
		dent_stats.fill(qRgb(0xff, 0xff, 0xff));
		
		{
			QPainter painter(&dent_stats);
			painter.drawImage(QPoint(0, 0), garbage_red);
			painter.drawImage(QPoint(0, 0), non_garbage_green);
		}
		dbg->add(dent_stats, "dent_stats");
		dbg->add(big_dark, "big_dark");
	}
	
	QRect content_rect(content.rect());
	
	for (bool first = true;; first = false) {
		status.throwIfCancelled();
		
		QRect old_content_rect(content_rect);
		content_rect = trimLeftRight(content, content_lighter, content_rect);
		if (content_rect.isEmpty()) {
			break;
		}
		if (!first && content_rect == old_content_rect) {
			break;
		}
		
		old_content_rect = content_rect;
		content_rect = trimTopBottom(content, content_lighter, content_rect);
		if (content_rect.isEmpty()) {
			break;
		}
		if (content_rect == old_content_rect) {
			break;
		}
	}
	
	status.throwIfCancelled();
	
	// Transform back from 150dpi.
	QTransform combined_xform(xform_150dpi.transform().inverted());
	combined_xform *= data.xform().transform();
	return combined_xform.map(QRectF(content_rect)).boundingRect();
}

void
ContentBoxFinder::filterShadows(
	TaskStatus const& status, imageproc::BinaryImage& shadows, DebugImages* dbg)
{
	// The input image should only contain shadows from the edges
	// of a page, but in practice it may also contain things like
	// a black table header which white letters on it.  Here we
	// try to filter them out.
	
	// White dots on black background may be a problem for us.
	// They may be misclassified as parts of white letters.
	BinaryImage reduced_dithering(closeBrick(shadows, QSize(1, 2), BLACK));
	reduced_dithering = closeBrick(reduced_dithering, QSize(2, 1), BLACK);
	if (dbg) {
		dbg->add(reduced_dithering, "reduced_dithering");
	}
	
	status.throwIfCancelled();
	
	// Long white vertical lines are definately not spaces between letters.
	BinaryImage vert_whitespace(
		closeBrick(reduced_dithering, QSize(1, 150), BLACK)
	);
	if (dbg) {
		dbg->add(vert_whitespace, "vert_whitespace");
	}
	
	status.throwIfCancelled();
	
	// Join neighboring white letters.
	BinaryImage opened(openBrick(reduced_dithering, QSize(10, 4), BLACK));
	reduced_dithering.release();
	if (dbg) {
		dbg->add(opened, "opened");
	}
	
	status.throwIfCancelled();
	
	// Extract areas that became white as a result of the last operation.
	rasterOp<RopSubtract<RopNot<RopDst>, RopNot<RopSrc> > >(opened, shadows);
	if (dbg) {
		dbg->add(opened, "became white");
	}
	
	status.throwIfCancelled();
	
	// Join the spacings between words together.
	BinaryImage closed(closeBrick(opened, QSize(20, 1), WHITE));
	opened.release();
	rasterOp<RopAnd<RopSrc, RopDst> >(closed, vert_whitespace);
	vert_whitespace.release();
	if (dbg) {
		dbg->add(closed, "closed");
	}
	
	status.throwIfCancelled();
	
	// If we've got long enough and tall enough blocks, we assume they
	// are the text lines.
	opened = openBrick(closed, QSize(50, 10), WHITE);
	closed.release();
	if (dbg) {
		dbg->add(opened, "reopened");
	}
	
	status.throwIfCancelled();
	
	BinaryImage non_shadows(seedFill(opened, shadows, CONN8));
	opened.release();
	if (dbg) {
		dbg->add(non_shadows, "non_shadows");
	}
	
	status.throwIfCancelled();
	
	rasterOp<RopSubtract<RopDst, RopSrc> >(shadows, non_shadows);
}

BinaryImage
ContentBoxFinder::getLighterContent(
	QImage const& gray, BinaryThreshold const reference_threshold,
	BinaryImage const& content_mask)
{
	GrayscaleHistogram const hist(gray, content_mask);
	int num_black_reference = 0;
	for (int i = 0; i < reference_threshold; ++i) {
		num_black_reference += hist[i];
	}
	
	int const max_threshold_delta = 20;
	int new_threshold = std::max(0, reference_threshold - max_threshold_delta);
	int num_black_delta = 0;
	for (int i = new_threshold; i < reference_threshold; ++i) {
		num_black_delta += hist[i];
	}
	
	for (; new_threshold < reference_threshold;
			++new_threshold, num_black_delta -= hist[new_threshold]) {
		int const num_black_new = num_black_reference - num_black_delta;
		if (num_black_reference < num_black_new * LIGHTER_REDUCE_FACTOR_SAFE) {
			break;
		}
	}
	
	BinaryImage content_lighter(gray, BinaryThreshold(new_threshold));
	rasterOp<RopAnd<RopSrc, RopDst> >(content_lighter, content_mask);
	return content_lighter;
}

bool
ContentBoxFinder::isBigAndDark(ConnComp const& cc)
{
	// We are working with 150 dpi images here.
	
	int const min_side = std::min(cc.width(), cc.height());
	if (min_side < 15) {
		return false;
	}
	
	int const square = cc.width() * cc.height();
	
	if (square < 100 * 30) {
		return false;
	}
	
	if (cc.pixCount() < square * 0.3) {
		return false;
	}
	
	return true;
}

bool
ContentBoxFinder::isWindingComponent(imageproc::BinaryImage const& cc_img)
{
	BinaryImage const dents(DentFinder::findDentsAndHoles(cc_img));
	
	int const w = cc_img.width();
	int const h = cc_img.height();
	int const cc_diagonal_squared = w * w + h * h;
	
	return (dents.countBlackPixels() >= cc_diagonal_squared * 0.04);
}

QRect
ContentBoxFinder::trimLeftRight(
	BinaryImage const& img, BinaryImage const& img_lighter, QRect const& area)
{
	using namespace boost::lambda;
	
	SlicedHistogram const hist(img, area, SlicedHistogram::COLS);
	
	ContentSpanFinder span_finder;
	span_finder.setMinContentWidth(10);
	
	// This should be more than the space between letters and less
	// than the space between content and the folding area.  The latter
	// is more important.
	span_finder.setMinWhitespaceWidth(7);
	
	typedef std::deque<Span> SpanList;
	SpanList spans;
	span_finder.find(
		hist,
		bind(&SpanList::push_back, var(spans), ret<Span>(_1 + area.left()))
	);
	
	// Go from top to bottom spans, removing garbage.
	for (; !spans.empty(); spans.pop_front()) {
		Span& span = spans.front();
		QRect span_area(area);
		span_area.setLeft(span.begin());
		span_area.setRight(span.end() - 1);
		QRect const new_area = processColumn(img, img_lighter, span_area);
		if (!new_area.isEmpty()) {
			span = Span(new_area.left(), new_area.right() + 1);
			break;
		}
	}
	
	// Go from bottom to top spans, removing garbage.
	for (; !spans.empty(); spans.pop_back()) {
		Span& span = spans.back();
		QRect span_area(area);
		span_area.setLeft(span.begin());
		span_area.setRight(span.end() - 1);
		QRect const new_area = processColumn(img, img_lighter, span_area);
		if (!new_area.isEmpty()) {
			span = Span(new_area.left(), new_area.right() + 1);
			break;
		}
	}
	
	if (spans.empty()) {
		return QRect();
	}
	
	QRect new_area(area);
	new_area.setLeft(spans.front().begin());
	new_area.setRight(spans.back().end() - 1);
	return new_area;
}

QRect
ContentBoxFinder::trimTopBottom(
	BinaryImage const& img, BinaryImage const& img_lighter, QRect const& area)
{
	using namespace boost::lambda;
	
	SlicedHistogram const hist(img, area, SlicedHistogram::ROWS);
	
	ContentSpanFinder span_finder;
	
	// Reduced because there may be a horizontal line at the top.
	span_finder.setMinContentWidth(5);
	
	span_finder.setMinWhitespaceWidth(10);
	
	typedef std::deque<Span> SpanList;
	SpanList spans;
	span_finder.find(
		hist,
		bind(&SpanList::push_back, var(spans), ret<Span>(_1 + area.top()))
	);
	
	// Go from top to bottom spans, removing garbage.
	for (; !spans.empty(); spans.pop_front()) {
		Span& span = spans.front();
		QRect span_area(area);
		span_area.setTop(span.begin());
		span_area.setBottom(span.end() - 1);
		QRect const new_area = processRow(img, img_lighter, span_area);
		if (!new_area.isEmpty()) {
			span = Span(new_area.top(), new_area.bottom() + 1);
			break;
		}
	}
	
	// Go from bottom to top spans, removing garbage.
	for (; !spans.empty(); spans.pop_back()) {
		Span& span = spans.back();
		QRect span_area(area);
		span_area.setTop(span.begin());
		span_area.setBottom(span.end() - 1);
		QRect const new_area = processRow(img, img_lighter, span_area);
		if (!new_area.isEmpty()) {
			span = Span(new_area.top(), new_area.bottom() + 1);
			break;
		}
	}
	
	if (spans.empty()) {
		return QRect();
	}
	
	QRect new_area(area);
	new_area.setTop(spans.front().begin());
	new_area.setBottom(spans.back().end() - 1);
	return new_area;
}

QRect
ContentBoxFinder::processColumn(
	BinaryImage const& img, BinaryImage const& img_lighter, QRect const& area)
{
	if (area.width() < 8) {
		return QRect();
	}
	
#if 0
	// This prevents us from removing garbage on the sides of a content region.
	if (area.width() > 45) {
		return area;
	}
#endif
	
	int const total_black_pixels = img.countBlackPixels(area);
	
	if (total_black_pixels > img_lighter.countBlackPixels(area) * LIGHTER_REDUCE_FACTOR) {
		/*
		Two possibilities here:
		1. We are dealing with a gradient, most likely the shadow
		from the folding area.
		2. We are dealing with content of slightly lighter color
		than the threshold gray level.  This could be a pencil
		stroke or some other garbage.
		*/
		return QRect();
	}
	
	BinaryImage region(area.size());
	rasterOp<RopSrc>(region, region.rect(), img, area.topLeft());
	
	int non_garbage_pixels = 0;
	int left = area.right();
	int right = area.left();
	
	ConnCompEraserExt eraser(region.release(), CONN8);
	for (ConnComp cc; !(cc = eraser.nextConnComp()).isNull(); ) {
		if (cc.pixCount() < 10 || (cc.width() < 5 && cc.height() < 5)) {
			// Too small, probably garbage.
			continue;
		}
		
		left = std::min(left, area.left() + cc.rect().left());
		right = std::max(right, area.left() + cc.rect().right());
		
		BinaryImage const cc_img(eraser.computeConnCompImage());
		
		if (!isWindingComponent(cc_img)) {
			// This connected component doesn't wind that much.
			// It must be a speckle or a stroke.  Actually, it
			// can also be one of the following symbols:
			// I, 1, l, /, \, ...
			// Usually, misclassifying one or two symbols won't
			// classify the whole region as garbage, but it may
			// shrink the region so that these symbols are out
			// of it.  To prevent that, we update left and right
			// anyway (see above), but we don't update non_garbage_pixels.
			// If it's too low, left and right won't matter.
			continue;
		}
		
		non_garbage_pixels += cc.pixCount();
	}
	
	if (left > right || non_garbage_pixels <= total_black_pixels * 0.3) {
		return QRect();
	} else {
		QRect new_area(area);
		new_area.setLeft(left);
		new_area.setRight(right);
		return new_area;
	}
}

QRect
ContentBoxFinder::processRow(
	BinaryImage const& img, BinaryImage const& img_lighter, QRect const& area)
{
#if 0
	// This prevents us from removing garbage on the sides of a content region.
	if (area.height() > 45) {
		return area;
	}
#endif
	
	int const total_black_pixels = img.countBlackPixels(area);
	int const lighter_black_pixels = img_lighter.countBlackPixels(area);
	
	if (total_black_pixels > lighter_black_pixels * LIGHTER_REDUCE_FACTOR) {
		// Here we have a different situation compared to processColumn().
		// On one hand, we are not going to encounter a shadow from
		// the folding area, but on the other hand we may easily
		// hit pictures, dithered table headers, etc.
		// Still, we'd like to remove pencil strokes and things like that.
		if (lighter_black_pixels < 5) {
			return QRect();
		}
	}
	
	BinaryImage region(area.size());
	rasterOp<RopSrc>(region, region.rect(), img, area.topLeft());
	
	int non_garbage_pixels = 0;
	int top = area.bottom();
	int bottom = area.top();
	
	ConnCompEraserExt eraser(region.release(), CONN8);
	for (ConnComp cc; !(cc = eraser.nextConnComp()).isNull(); ) {
		if (cc.pixCount() < 10 || (cc.width() < 5 && cc.height() < 5)) {
			// Too small, probably garbage.
			continue;
		}
		
		bool const hline = (cc.width() > cc.height() * 20);
		bool const long_hline = hline && (cc.width() > area.width() * 0.7);
		
		bool const short_vline = (
			(cc.height() > cc.width() * 2)
			&& (cc.height() < cc.width() * 15)
		);
		
		bool const big_and_dark = isBigAndDark(cc);
		
		if (!(short_vline || hline || big_and_dark)) {
			BinaryImage const cc_img(eraser.computeConnCompImage());
			
			if (!isWindingComponent(cc_img)) {
				// This connected component doesn't wind that much.
				// It must be a speckle or a stroke.  Actually, it
				// can also be one of the following symbols:
				// I, 1, l, /, \, ...
				// We don't really care about most of them, except
				// for '1', because page numbers such as 1, 11, 111
				// may be misclassified as garbage.  That's why we
				// protect components with high height / width ratio.
				// The enclosing 'if' provides protection.
				continue;
			}
		}
		
		// We don't insist the horizontal line is necessarily
		// non-garbage.  It may for example be the edge of a page.
		// On the other hand, we still append it to non_garbage_pixels,
		// or otherwise it may overwhelm any other non-garbage content
		// in that area.
		if (!hline || long_hline) {
			top = std::min(top, area.top() + cc.rect().top());
			bottom = std::max(bottom, area.top() + cc.rect().bottom());
		}
		non_garbage_pixels += cc.pixCount();
	}
	
	if (top > bottom || non_garbage_pixels <= total_black_pixels * 0.3) {
		return QRect();
	} else {
		QRect new_area(area);
		new_area.setTop(top);
		new_area.setBottom(bottom);
		return new_area;
	}
}

} // namespace select_content
