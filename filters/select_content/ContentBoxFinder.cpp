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

#include "ContentBoxFinder.h"
#include "TaskStatus.h"
#include "DebugImages.h"
#include "FilterData.h"
#include "ImageTransformation.h"
#include "Dpi.h"
#include "Despeckle.h"
#include "imageproc/BinaryImage.h"
#include "imageproc/BinaryThreshold.h"
#include "imageproc/Binarize.h"
#include "imageproc/BWColor.h"
#include "imageproc/Connectivity.h"
#include "imageproc/ConnComp.h"
#include "imageproc/ConnCompEraserExt.h"
#include "imageproc/Transform.h"
#include "imageproc/RasterOp.h"
#include "imageproc/GrayRasterOp.h"
#include "imageproc/SeedFill.h"
#include "imageproc/Morphology.h"
#include "imageproc/Grayscale.h"
#include "imageproc/SlicedHistogram.h"
#include "imageproc/PolygonRasterizer.h"
#include "imageproc/MaxWhitespaceFinder.h"
#include "imageproc/ConnectivityMap.h"
#include "imageproc/InfluenceMap.h"
#include "imageproc/SEDM.h"
#ifndef Q_MOC_RUN
#include <boost/foreach.hpp>
#endif
#include <QRect>
#include <QRectF>
#include <QPolygonF>
#include <QImage>
#include <QColor>
#include <QPainter>
#include <QPainterPath>
#include <QTransform>
#include <QtGlobal>
#include <Qt>
#include <QDebug>
#include <queue>
#include <vector>
#include <algorithm>
#include <limits>
#include <math.h>
#include <stdlib.h>
#include <limits.h>

#include "CommandLine.h"

namespace select_content
{

using namespace imageproc;

class ContentBoxFinder::Garbage
{
public:
	enum Type { HOR, VERT };
	
	Garbage(Type type, BinaryImage const& garbage);
	
	void add(BinaryImage const& garbage, QRect const& rect);
	
	BinaryImage const& image() const { return m_garbage; }
	
	SEDM const& sedm();
private:
	imageproc::BinaryImage m_garbage;
	SEDM m_sedm;
	SEDM::Borders m_sedmBorders;
	bool m_sedmUpdatePending;
};


namespace
{

struct PreferHorizontal
{
	bool operator()(QRect const& lhs, QRect const& rhs) const {
		return lhs.width() * lhs.width() * lhs.height()
			< rhs.width() * rhs.width() * rhs.height();
	}
};

struct PreferVertical
{
	bool operator()(QRect const& lhs, QRect const& rhs) const {
		return lhs.width() * lhs.height() * lhs.height()
			< rhs.width() * rhs.height() * rhs.height();
	}
};

} // anonymous namespace

QRectF
ContentBoxFinder::findContentBox(
	TaskStatus const& status, FilterData const& data, DebugImages* dbg)
{
	ImageTransformation xform_150dpi(data.xform());
	xform_150dpi.preScaleToDpi(Dpi(150, 150));

	if (xform_150dpi.resultingRect().toRect().isEmpty()) {
		return QRectF();
	}
	
	uint8_t const darkest_gray_level = darkestGrayLevel(data.grayImage());
	QColor const outside_color(darkest_gray_level, darkest_gray_level, darkest_gray_level);

	QImage gray150(
		transformToGray(
			data.grayImage(), xform_150dpi.transform(),
			xform_150dpi.resultingRect().toRect(),
			OutsidePixels::assumeColor(outside_color)
		)
	);
	// Note that we fill new areas that appear as a result of
	// rotation with black, not white.  Filling them with white
	// may be bad for detecting the shadow around the page.
	if (dbg) {
		dbg->add(gray150, "gray150");
	}
	
	BinaryImage bw150(binarizeWolf(gray150, QSize(51, 51), 50));
	if (dbg) {
		dbg->add(bw150, "bw150");
	}
	
	PolygonRasterizer::fillExcept(
		bw150, BLACK, xform_150dpi.resultingPreCropArea(), Qt::WindingFill
	);
	if (dbg) {
		dbg->add(bw150, "page_mask_applied");
	}
	
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
	
	BinaryImage shadows_seed(hor_shadows_seed.release());
	rasterOp<RopOr<RopSrc, RopDst> >(shadows_seed, ver_shadows_seed);
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
	BinaryImage garbage(shadows_dilated.release());
	if (dbg) {
		dbg->add(garbage, "shadows");
	}
	
	status.throwIfCancelled();
	
	filterShadows(status, garbage, dbg);
	if (dbg) {
		dbg->add(garbage, "filtered_shadows");
	}
	
	status.throwIfCancelled();
	
	BinaryImage content(bw150.release());
	rasterOp<RopSubtract<RopDst, RopSrc> >(content, garbage);
	if (dbg) {
		dbg->add(content, "content");
	}
	
	status.throwIfCancelled();
	
	CommandLine const& cli = CommandLine::get();
	Despeckle::Level despeckleLevel = Despeckle::NORMAL;
	if (cli.hasContentRect()) {
		despeckleLevel = cli.getContentDetection();
	}

	BinaryImage despeckled(Despeckle::despeckle(content, Dpi(150, 150), despeckleLevel, status, dbg));
	if (dbg) {
		dbg->add(despeckled, "despeckled");
	}
	
	status.throwIfCancelled();
	
	BinaryImage content_blocks(content.size(), BLACK);
	int const area_threshold = std::min(content.width(), content.height());
	
	{
		MaxWhitespaceFinder hor_ws_finder(PreferHorizontal(), despeckled);
		
		for (int i = 0; i < 80; ++i) {
			QRect ws(hor_ws_finder.next(hor_ws_finder.MANUAL_OBSTACLES));
			if (ws.isNull()) {
				break;
			}
			if (ws.width() * ws.height() < area_threshold) {
				break;
			}
			content_blocks.fill(ws, WHITE);
			int const height_fraction = ws.height() / 5;
			ws.setTop(ws.top() + height_fraction);
			ws.setBottom(ws.bottom() - height_fraction);
			hor_ws_finder.addObstacle(ws);
		}
	}
	
	{
		MaxWhitespaceFinder vert_ws_finder(PreferVertical(), despeckled);
		
		for (int i = 0; i < 40; ++i) {
			QRect ws(vert_ws_finder.next(vert_ws_finder.MANUAL_OBSTACLES));
			if (ws.isNull()) {
				break;
			}
			if (ws.width() * ws.height() < area_threshold) {
				break;
			}
			content_blocks.fill(ws, WHITE);
			int const width_fraction = ws.width() / 5;
			ws.setLeft(ws.left() + width_fraction);
			ws.setRight(ws.right() - width_fraction);
			vert_ws_finder.addObstacle(ws);
		}
	}
	
	if (dbg) {
		dbg->add(content_blocks, "content_blocks");
	}
	
	trimContentBlocksInPlace(despeckled, content_blocks);
	if (dbg) {
		dbg->add(content_blocks, "initial_trimming");
	}

	// Do some more whitespace finding.  This should help us separate
	// blocks that don't belong together.
	{
		BinaryImage tmp(content);
		rasterOp<RopOr<RopNot<RopSrc>, RopDst> >(tmp, content_blocks);
		MaxWhitespaceFinder ws_finder(tmp.release(), QSize(4, 4));
		
		for (int i = 0; i < 10; ++i) {
			QRect ws(ws_finder.next());
			if (ws.isNull()) {
				break;
			}
			if (ws.width() * ws.height() < area_threshold) {
				break;
			}
			content_blocks.fill(ws, WHITE);
		}
	}
	if (dbg) {
		dbg->add(content_blocks, "more_whitespace");
	}
	
	trimContentBlocksInPlace(despeckled, content_blocks);
	if (dbg) {
		dbg->add(content_blocks, "more_trimming");
	}
	
	despeckled.release();
	
	inPlaceRemoveAreasTouchingBorders(content_blocks, dbg);
	if (dbg) {
		dbg->add(content_blocks, "except_bordering");
	}
	
	BinaryImage text_mask(estimateTextMask(content, content_blocks, dbg));
	if (dbg) {
		QImage text_mask_visualized(content.size(), QImage::Format_ARGB32_Premultiplied);
		text_mask_visualized.fill(0xffffffff); // Opaque white.
		
		QPainter painter(&text_mask_visualized);
		
		QImage tmp(content.size(), QImage::Format_ARGB32_Premultiplied);
		tmp.fill(0xff64dd62); // Opaque light green.
		tmp.setAlphaChannel(text_mask.inverted().toQImage());
		painter.drawImage(QPoint(0, 0), tmp);
		
		tmp.fill(0xe0000000); // Mostly transparent black.
		tmp.setAlphaChannel(content.inverted().toQImage());
		painter.drawImage(QPoint(0, 0), tmp);
		
		painter.end();
		
		dbg->add(text_mask_visualized, "text_mask");
	}
	
	// Make text_mask strore the actual content pixels that are text.
	rasterOp<RopAnd<RopSrc, RopDst> >(text_mask, content);
	
	QRect content_rect(content_blocks.contentBoundingBox());
	
	// Temporarily reuse hor_shadows_seed and ver_shadows_seed.
	// It's OK they are null.
	segmentGarbage(garbage, hor_shadows_seed, ver_shadows_seed, dbg);
	garbage.release();
	
	if (dbg) {
		dbg->add(hor_shadows_seed, "initial_hor_garbage");
		dbg->add(ver_shadows_seed, "initial_vert_garbage");
	}
	
	Garbage hor_garbage(Garbage::HOR, hor_shadows_seed.release());
	Garbage vert_garbage(Garbage::VERT, ver_shadows_seed.release());
	
	enum Side { LEFT = 1, RIGHT = 2, TOP = 4, BOTTOM = 8 };
	int side_mask = LEFT|RIGHT|TOP|BOTTOM;
	
	while (side_mask && !content_rect.isEmpty()) {
		QRect old_content_rect;
		
		if (side_mask & LEFT) {
			side_mask &= ~LEFT;
			old_content_rect = content_rect;
			content_rect = trimLeft(
				content, content_blocks, text_mask,
				content_rect, vert_garbage, dbg
			);
			
			status.throwIfCancelled();
			
			if (content_rect.isEmpty()) {
				break;
			}
			if (old_content_rect != content_rect) {
				side_mask |= LEFT|TOP|BOTTOM;
			}
		}
		
		if (side_mask & RIGHT) {
			side_mask &= ~RIGHT;
			old_content_rect = content_rect;
			content_rect = trimRight(
				content, content_blocks, text_mask,
				content_rect, vert_garbage, dbg
			);
			
			status.throwIfCancelled();
			
			if (content_rect.isEmpty()) {
				break;
			}
			if (old_content_rect != content_rect) {
				side_mask |= RIGHT|TOP|BOTTOM;
			}
		}
		
		if (side_mask & TOP) {
			side_mask &= ~TOP;
			old_content_rect = content_rect;
			content_rect = trimTop(
				content, content_blocks, text_mask,
				content_rect, hor_garbage, dbg
			);
			
			status.throwIfCancelled();
			
			if (content_rect.isEmpty()) {
				break;
			}
			if (old_content_rect != content_rect) {
				side_mask |= TOP|LEFT|RIGHT;
			}
		}
		
		if (side_mask & BOTTOM) {
			side_mask &= ~BOTTOM;
			old_content_rect = content_rect;
			content_rect = trimBottom(
				content, content_blocks, text_mask,
				content_rect, hor_garbage, dbg
			);
			
			status.throwIfCancelled();
			
			if (content_rect.isEmpty()) {
				break;
			}
			if (old_content_rect != content_rect) {
				side_mask |= BOTTOM|LEFT|RIGHT;
			}
		}
		
		if (content_rect.width() < 8 || content_rect.height() < 8) {
			content_rect = QRect();
			break;
		} else if (content_rect.width() < 30 &&
				content_rect.height() >
				content_rect.width() * 20) {
			content_rect = QRect();
			break;
		}
	}
	
	// Transform back from 150dpi.
	QTransform combined_xform(xform_150dpi.transform().inverted());
	combined_xform *= data.xform().transform();
	return combined_xform.map(QRectF(content_rect)).boundingRect();
}

namespace
{

struct Bounds
{
	// All are inclusive.
	int left;
	int right;
	int top;
	int bottom;
	
	Bounds() : left(INT_MAX), right(INT_MIN), top(INT_MAX), bottom(INT_MIN) {}
	
	bool isInside(int x, int y) const {
		if (x < left) {
			return false;
		} else if (x > right) {
			return false;
		} else if (y < top) {
			return false;
		} else if (y > bottom) {
			return false;
		} else {
			return true;
		}
	}
	
	void forceInside(int x, int y) {
		if (x < left) {
			left = x;
		}
		if (x > right) {
			right = x;
		}
		if (y < top) {
			top = y;
		}
		if (y > bottom) {
			bottom = y;
		}
	}
};

} // anonymous namespace

void
ContentBoxFinder::trimContentBlocksInPlace(
	imageproc::BinaryImage const& content,
	imageproc::BinaryImage& content_blocks)
{
	ConnectivityMap const cmap(content_blocks, CONN4);
	std::vector<Bounds> bounds(cmap.maxLabel() + 1);
	
	int width = content.width();
	int height = content.height();
	uint32_t const msb = uint32_t(1) << 31;
	
	uint32_t const* content_line = content.data();
	int const content_stride = content.wordsPerLine();
	uint32_t const* cmap_line = cmap.data();
	int const cmap_stride = cmap.stride();
	for (int y = 0; y < height; ++y) {
		for (int x = 0; x < width; ++x) {
			uint32_t const label = cmap_line[x];
			if (label == 0) {
				continue;
			}
			if (content_line[x >> 5] & (msb >> (x & 31))) {
				bounds[label].forceInside(x, y);
			}
		}
		cmap_line += cmap_stride;
		content_line += content_stride;
	}
	
	uint32_t* cb_line = content_blocks.data();
	int const cb_stride = content_blocks.wordsPerLine();
	cmap_line = cmap.data();
	for (int y = 0; y < height; ++y) {
		for (int x = 0; x < width; ++x) {
			uint32_t const label = cmap_line[x];
			if (label == 0) {
				continue;
			}
			if (!bounds[label].isInside(x, y)) {
				cb_line[x >> 5] &= ~(msb >> (x & 31));
			}
		}
		cmap_line += cmap_stride;
		cb_line += cb_stride;
	}
}

void
ContentBoxFinder::inPlaceRemoveAreasTouchingBorders(
	imageproc::BinaryImage& content_blocks, DebugImages* dbg)
{
	// We could just do a seed fill from borders, but that
	// has the potential to remove too much.  Instead, we
	// do something similar to a seed fill, but with a limited
	// spread distance.
	
	
	int const width = content_blocks.width();
	int const height = content_blocks.height();
	
	uint16_t const max_spread_dist = std::min(width, height) / 4;
	
	std::vector<uint16_t> map((width + 2) * (height + 2), ~uint16_t(0));
	
	uint32_t* cb_line = content_blocks.data();
	int const cb_stride = content_blocks.wordsPerLine();
	uint16_t* map_line = &map[0] + width + 3;
	int const map_stride = width + 2;
	for (int y = 0; y < height; ++y) {
		for (int x = 0; x < width; ++x) {
			uint32_t mask = cb_line[x >> 5] >> (31 - (x & 31));
			mask &= uint32_t(1);
			--mask;
			
			// WHITE -> max, BLACK -> 0
			map_line[x] = static_cast<uint16_t>(mask);
		}
		map_line += map_stride;
		cb_line += cb_stride;
	}
	
	std::queue<uint16_t*> queue;
	
	// Initialize border seeds.
	map_line = &map[0] + width + 3;
	for (int x = 0; x < width; ++x) {
		if (map_line[x] == 0) {
			map_line[x] = max_spread_dist;
			queue.push(&map_line[x]);
		}
	}
	for (int y = 1; y < height - 1; ++y) {
		if (map_line[0] == 0) {
			map_line[0] = max_spread_dist;
			queue.push(&map_line[0]);
		}
		if (map_line[width - 1] == 0) {
			map_line[width - 1] = max_spread_dist;
			queue.push(&map_line[width - 1]);
		}
		map_line += map_stride;
	}
	for (int x = 0; x < width; ++x) {
		if (map_line[x] == 0) {
			map_line[x] = max_spread_dist;
			queue.push(&map_line[x]);
		}
	}
	
	if (queue.empty()) {
		// Common case optimization.
		return;
	}
	
	while (!queue.empty()) {
		uint16_t* cell = queue.front();
		queue.pop();
		
		assert(*cell != 0);
		uint16_t const new_dist = *cell - 1;
		
		uint16_t* nbh = cell - map_stride;
		if (new_dist > *nbh) {
			*nbh = new_dist;
			queue.push(nbh);
		}
		
		nbh = cell - 1;
		if (new_dist > *nbh) {
			*nbh = new_dist;
			queue.push(nbh);
		}
		
		nbh = cell + 1;
		if (new_dist > *nbh) {
			*nbh = new_dist;
			queue.push(nbh);
		}
		
		nbh = cell + map_stride;
		if (new_dist > *nbh) {
			*nbh = new_dist;
			queue.push(nbh);
		}
	}
	
	cb_line = content_blocks.data();
	map_line = &map[0] + width + 3;
	uint32_t const msb = uint32_t(1) << 31;
	for (int y = 0; y < height; ++y) {
		for (int x = 0; x < width; ++x) {
			if (map_line[x] + 1 > 1) { // If not 0 or ~uint16_t(0)
				cb_line[x >> 5] &= ~(msb >> (x & 31));
			}
		}
		map_line += map_stride;
		cb_line += cb_stride;
	}
}

void
ContentBoxFinder::segmentGarbage(
	imageproc::BinaryImage const& garbage,
	imageproc::BinaryImage& hor_garbage,
	imageproc::BinaryImage& vert_garbage,
	DebugImages* dbg)
{
	hor_garbage = openBrick(garbage, QSize(200, 1), WHITE);
	
	QRect rect(garbage.rect());
	rect.setHeight(1);
	rasterOp<RopOr<RopSrc, RopDst> >(
		hor_garbage, rect, garbage, rect.topLeft()
	);
	rect.moveBottom(garbage.rect().bottom());
	rasterOp<RopOr<RopSrc, RopDst> >(
		hor_garbage, rect, garbage, rect.topLeft()
	);
	
	vert_garbage = openBrick(garbage, QSize(1, 200), WHITE);
	
	rect = garbage.rect();
	rect.setWidth(1);
	rasterOp<RopOr<RopSrc, RopDst> >(
		vert_garbage, rect, garbage, rect.topLeft()
	);
	rect.moveRight(garbage.rect().right());
	rasterOp<RopOr<RopSrc, RopDst> >(
		vert_garbage, rect, garbage, rect.topLeft()
	);
	
	ConnectivityMap cmap(garbage.size());
	
	cmap.addComponent(vert_garbage);
	vert_garbage.fill(WHITE);
	cmap.addComponent(hor_garbage);
	hor_garbage.fill(WHITE);
	
	InfluenceMap imap(cmap, garbage);
	cmap = ConnectivityMap();
	
	int const width = garbage.width();
	int const height = garbage.height();
	
	InfluenceMap::Cell* imap_line = imap.data();
	int const imap_stride = imap.stride();
	
	uint32_t* vg_line = vert_garbage.data();
	int const vg_stride = vert_garbage.wordsPerLine();
	uint32_t* hg_line = hor_garbage.data();
	int const hg_stride = hor_garbage.wordsPerLine();
	
	uint32_t const msb = uint32_t(1) << 31;
	
	for (int y = 0; y < height; ++y) {
		for (int x = 0; x < width; ++x) {
			switch (imap_line[x].label) {
				case 1:
					vg_line[x >> 5] |= msb >> (x & 31);
					break;
				case 2:
					hg_line[x >> 5] |= msb >> (x & 31);
					break;
			}
		}
		imap_line += imap_stride;
		vg_line += vg_stride;
		hg_line += hg_stride;
	}
	
	BinaryImage unconnected_garbage(garbage);
	rasterOp<RopSubtract<RopDst, RopSrc> >(unconnected_garbage, hor_garbage);
	rasterOp<RopSubtract<RopDst, RopSrc> >(unconnected_garbage, vert_garbage);
	
	rasterOp<RopOr<RopSrc, RopDst> >(hor_garbage, unconnected_garbage);
	rasterOp<RopOr<RopSrc, RopDst> >(vert_garbage, unconnected_garbage);
}

imageproc::BinaryImage
ContentBoxFinder::estimateTextMask(
	imageproc::BinaryImage const& content,
	imageproc::BinaryImage const& content_blocks,
	DebugImages* dbg)
{
	// We differentiate between a text line and a slightly skewed straight
	// line (which may have a fill factor similar to that of text) by the
	// presence of ultimate eroded points.
	
	BinaryImage const ueps(
		SEDM(content, SEDM::DIST_TO_BLACK, SEDM::DIST_TO_NO_BORDERS)
		.findPeaksDestructive()
	);
	if (dbg) {
		QImage canvas(content_blocks.toQImage().convertToFormat(QImage::Format_ARGB32_Premultiplied));
		QPainter painter;
		painter.begin(&canvas);
		QImage overlay(canvas.size(), canvas.format());
		overlay.fill(0xff0000ff); // opaque blue
		overlay.setAlphaChannel(content.inverted().toQImage());
		painter.drawImage(QPoint(0, 0), overlay);
		
		BinaryImage ueps_on_content_blocks(content_blocks);
		rasterOp<RopAnd<RopSrc, RopDst> >(ueps_on_content_blocks, ueps);
		
		overlay.fill(0xffffff00); // opaque yellow
		overlay.setAlphaChannel(ueps_on_content_blocks.inverted().toQImage());
		painter.drawImage(QPoint(0, 0), overlay);
		
		painter.end();
		dbg->add(canvas, "ueps");
	}
	
	BinaryImage text_mask(content.size(), WHITE);
	
	int const min_text_height = 6;
	
	ConnCompEraserExt eraser(content_blocks, CONN4);
	for (;;) {
		ConnComp const cc(eraser.nextConnComp());
		if (cc.isNull()) {
			break;
		}
		
		BinaryImage cc_img(eraser.computeConnCompImage());
		BinaryImage content_img(cc_img.size());
		rasterOp<RopSrc>(
			content_img, content_img.rect(),
			content, cc.rect().topLeft()
		);
		
		// Note that some content may actually be not masked
		// by content_blocks, because we build content_blocks
		// based on despeckled content image.
		rasterOp<RopAnd<RopSrc, RopDst> >(content_img, cc_img);
		
		SlicedHistogram const hist(content_img, SlicedHistogram::ROWS);
		SlicedHistogram const block_hist(cc_img, SlicedHistogram::ROWS);
		
		assert(hist.size() != 0);
		
		typedef std::pair<int const*, int const*> Range;
		std::vector<Range> ranges;
		std::vector<Range> splittable_ranges;
		splittable_ranges.push_back(
			Range(&hist[0], &hist[hist.size() - 1])
		);
		
		std::vector<int> max_forward(hist.size());
		std::vector<int> max_backwards(hist.size());
		
		// Try splitting text lines.
		while (!splittable_ranges.empty()) {
			int const* const first = splittable_ranges.back().first;
			int const* const last = splittable_ranges.back().second;
			splittable_ranges.pop_back();
			
			if (last - first < min_text_height - 1) {
				// Just ignore such a small segment.
				continue;
			}
			
			// Fill max_forward and max_backwards.
			{
				int prev = *first;
				for (int i = 0; i <= last - first; ++i) {
					prev = std::max(prev, first[i]);
					max_forward[i] = prev;
				}
				prev = *last;
				for (int i = 0; i <= last - first; ++i) {
					prev = std::max(prev, last[-i]);
					max_backwards[i] = prev;
				}
			}
			
			int best_magnitude = std::numeric_limits<int>::min();
			int const* best_split_pos = 0;
			assert(first != last);
			for (int const* p = first + 1; p != last; ++p) {
				int const peak1 = max_forward[p - (first + 1)];
				int const peak2 = max_backwards[(last - 1) - p];
				if (*p * 3.5 > 0.5 * (peak1 + peak2)) {
					continue;
				}
				int const shoulder1 = peak1 - *p;
				int const shoulder2 = peak2 - *p;
				if (shoulder1 <= 0 || shoulder2 <= 0) {
					continue;
				}
				if (std::min(shoulder1, shoulder2) * 20 <
						std::max(shoulder1, shoulder2)) {
					continue;
				}
				
				int const magnitude = shoulder1 + shoulder2;
				if (magnitude > best_magnitude) {
					best_magnitude = magnitude;
					best_split_pos = p;
				}
			}
			
			if (best_split_pos) {
				splittable_ranges.push_back(
					Range(first, best_split_pos - 1)
				);
				splittable_ranges.push_back(
					Range(best_split_pos + 1, last)
				);
			} else {
				ranges.push_back(Range(first, last));
			}
		}
		
		BOOST_FOREACH (Range const range, ranges) {
			int const first = range.first - &hist[0];
			int const last = range.second - &hist[0];
			if (last - first < min_text_height - 1) {
				continue;
			}
			
			int64_t weighted_y = 0;
			int total_weight = 0;
			for (int i = first; i <= last; ++i) {
				int const val = hist[i];
				weighted_y += val * i;
				total_weight += val;
			}
			
			if (total_weight == 0) {
				//qDebug() << "no black pixels at all";
				continue;
			}
			
			double const min_fill_factor = 0.22;
			double const max_fill_factor = 0.65;
			
			int const center_y = (weighted_y + total_weight / 2) / total_weight;
			int top = center_y - min_text_height / 2;
			int bottom = top + min_text_height - 1;
			int num_black = 0;
			int num_total = 0;
			int max_width = 0;
			if (top < first || bottom > last) {
				continue;
			}
			for (int i = top; i <= bottom; ++i) {
				num_black += hist[i];
				num_total += block_hist[i];
				max_width = std::max(max_width, block_hist[i]);
			}
			if (num_black < num_total * min_fill_factor) {
				//qDebug() << "initial fill factor too low";
				continue;
			}
			if (num_black > num_total * max_fill_factor) {
				//qDebug() << "initial fill factor too high";
				continue;
			}
			
			// Extend the top and bottom of the text line.
			while ((top > first || bottom < last) &&
					abs((center_y - top) - (bottom - center_y)) <= 1) {
				int const new_top = (top > first) ? top - 1 : top;
				int const new_bottom = (bottom < last) ? bottom + 1 : bottom; 
				num_black += hist[new_top] + hist[new_bottom];
				num_total += block_hist[new_top] + block_hist[new_bottom];
				if (num_black < num_total * min_fill_factor) {
					break;
				}
				max_width = std::max(max_width, block_hist[new_top]);
				max_width = std::max(max_width, block_hist[new_bottom]);
				top = new_top;
				bottom = new_bottom;
			}
			
			if (num_black > num_total * max_fill_factor) {
				//qDebug() << "final fill factor too high";
				continue;
			}
			
			if (max_width < (bottom - top + 1) * 0.6) {
				//qDebug() << "aspect ratio too low";
				continue;
			}
			
			QRect line_rect(cc.rect());
			line_rect.setTop(cc.rect().top() + top);
			line_rect.setBottom(cc.rect().top() + bottom);
			
			// Check if there are enough ultimate eroded points on the line.
			int ueps_todo = int(0.4 * line_rect.width() / line_rect.height());
			if (ueps_todo) {
				BinaryImage line_ueps(line_rect.size());
				rasterOp<RopSrc>(line_ueps, line_ueps.rect(), content_blocks, line_rect.topLeft());
				rasterOp<RopAnd<RopSrc, RopDst> >(line_ueps, line_ueps.rect(), ueps, line_rect.topLeft());
				ConnCompEraser ueps_eraser(line_ueps, CONN4);
				ConnComp cc;
				for (; ueps_todo && !(cc = ueps_eraser.nextConnComp()).isNull(); --ueps_todo) {
					// Erase components until ueps_todo reaches zero or there are no more components.
				}
				if (ueps_todo) {
					// Not enough ueps were found.
					//qDebug() << "Not enough UEPs.";
					continue;
				}
			}
			
			// Write this block to the text mask.
			rasterOp<RopOr<RopSrc, RopDst> >(
				text_mask, line_rect, cc_img, QPoint(0, top)
			);
		}
	}
	
	return text_mask;
}

QRect
ContentBoxFinder::trimLeft(
	imageproc::BinaryImage const& content,
	imageproc::BinaryImage const& content_blocks,
	imageproc::BinaryImage const& text, QRect const& area,
	Garbage& garbage, DebugImages* const dbg)
{
	SlicedHistogram const hist(content_blocks, area, SlicedHistogram::COLS);
	
	size_t start = 0;
	while (start < hist.size()) {
		size_t first_ws = start;
		for (; first_ws < hist.size() && hist[first_ws] != 0; ++first_ws) {
			// Skip non-empty columns.
		}
		size_t first_non_ws = first_ws;
		for (; first_non_ws < hist.size() && hist[first_non_ws] == 0; ++first_non_ws) {
			// Skip empty columns.
		}
		
		first_ws += area.left();
		first_non_ws += area.left();
		
		QRect new_area(area);
		new_area.setLeft(first_non_ws);
		if (new_area.isEmpty()) {
			return area;
		}
		
		QRect removed_area(area);
		removed_area.setRight(first_ws - 1);
		if (removed_area.isEmpty()) {
			return new_area;
		}
		
		bool can_retry_grouped = false;
		QRect const res = trim(
			content, content_blocks, text,
			area, new_area, removed_area,
			garbage, can_retry_grouped, dbg
		);
		if (can_retry_grouped) {
			start = first_non_ws - area.left();
		} else {
			return res;
		}
	}
	
	return area;
}

QRect
ContentBoxFinder::trimRight(
	imageproc::BinaryImage const& content,
	imageproc::BinaryImage const& content_blocks,
	imageproc::BinaryImage const& text, QRect const& area,
	Garbage& garbage, DebugImages* const dbg)
{
	SlicedHistogram const hist(content_blocks, area, SlicedHistogram::COLS);
	
	int start = hist.size() - 1;
	while (start >= 0) {
		int first_ws = start;
		for (; first_ws >= 0 && hist[first_ws] != 0; --first_ws) {
			// Skip non-empty columns.
		}
		int first_non_ws = first_ws;
		for (; first_non_ws >= 0 && hist[first_non_ws] == 0; --first_non_ws) {
			// Skip empty columns.
		}
		
		first_ws += area.left();
		first_non_ws += area.left();
		
		QRect new_area(area);
		new_area.setRight(first_non_ws);
		if (new_area.isEmpty()) {
			return area;
		}
		
		QRect removed_area(area);
		removed_area.setLeft(first_ws + 1);
		if (removed_area.isEmpty()) {
			return new_area;
		}
		
		bool can_retry_grouped = false;
		QRect const res = trim(
			content, content_blocks, text,
			area, new_area, removed_area,
			garbage, can_retry_grouped, dbg
		);
		if (can_retry_grouped) {
			start = first_non_ws - area.left();
		} else {
			return res;
		}
	}
	
	return area;
}

QRect
ContentBoxFinder::trimTop(
	imageproc::BinaryImage const& content,
	imageproc::BinaryImage const& content_blocks,
	imageproc::BinaryImage const& text, QRect const& area,
	Garbage& garbage, DebugImages* const dbg)
{
	SlicedHistogram const hist(content_blocks, area, SlicedHistogram::ROWS);
	
	size_t start = 0;
	while (start < hist.size()) {
		size_t first_ws = start;
		for (; first_ws < hist.size() && hist[first_ws] != 0; ++first_ws) {
			// Skip non-empty columns.
		}
		size_t first_non_ws = first_ws;
		for (; first_non_ws < hist.size() && hist[first_non_ws] == 0; ++first_non_ws) {
			// Skip empty columns.
		}
		
		first_ws += area.top();
		first_non_ws += area.top();
		
		QRect new_area(area);
		new_area.setTop(first_non_ws);
		if (new_area.isEmpty()) {
			return area;
		}
		
		QRect removed_area(area);
		removed_area.setBottom(first_ws - 1);
		if (removed_area.isEmpty()) {
			return new_area;
		}
		
		bool can_retry_grouped = false;
		QRect const res = trim(
			content, content_blocks, text,
			area, new_area, removed_area,
			garbage, can_retry_grouped, dbg
		);
		if (can_retry_grouped) {
			start = first_non_ws - area.top();
		} else {
			return res;
		}
	}
	
	return area;
}

QRect
ContentBoxFinder::trimBottom(
	imageproc::BinaryImage const& content,
	imageproc::BinaryImage const& content_blocks,
	imageproc::BinaryImage const& text, QRect const& area,
	Garbage& garbage, DebugImages* const dbg)
{
	SlicedHistogram const hist(content_blocks, area, SlicedHistogram::ROWS);
	
	int start = hist.size() - 1;
	while (start >= 0) {
		int first_ws = start;
		for (; first_ws >= 0 && hist[first_ws] != 0; --first_ws) {
			// Skip non-empty columns.
		}
		int first_non_ws = first_ws;
		for (; first_non_ws >= 0 && hist[first_non_ws] == 0; --first_non_ws) {
			// Skip empty columns.
		}
		
		first_ws += area.top();
		first_non_ws += area.top();
		
		QRect new_area(area);
		new_area.setBottom(first_non_ws);
		if (new_area.isEmpty()) {
			return area;
		}
		
		QRect removed_area(area);
		removed_area.setTop(first_ws + 1);
		if (removed_area.isEmpty()) {
			return new_area;
		}
		
		bool can_retry_grouped = false;
		QRect const res = trim(
			content, content_blocks, text,
			area, new_area, removed_area,
			garbage, can_retry_grouped, dbg
		);
		if (can_retry_grouped) {
			start = first_non_ws - area.top();
		} else {
			return res;
		}
	}
	
	return area;
}

QRect
ContentBoxFinder::trim(
	imageproc::BinaryImage const& content,
	imageproc::BinaryImage const& content_blocks,
	imageproc::BinaryImage const& text,
	QRect const& area, QRect const& new_area,
	QRect const& removed_area, Garbage& garbage,
	bool& can_retry_grouped, DebugImages* const dbg)
{
	can_retry_grouped = false;
	
	QImage visualized;
	
	if (dbg) {
		visualized = QImage(
			content_blocks.size(),
			QImage::Format_ARGB32_Premultiplied
		);
		QPainter painter(&visualized);
		painter.drawImage(QPoint(0, 0), content_blocks.toQImage());
		
		QPainterPath outer_path;
		outer_path.addRect(visualized.rect());
		QPainterPath inner_path;
		inner_path.addRect(area);
		
		// Fill already rejected area with translucent gray.
		painter.setPen(Qt::NoPen);
		painter.setBrush(QColor(0x00, 0x00, 0x00, 50));
		painter.drawPath(outer_path.subtracted(inner_path));
	}
	
	// Don't trim too much.
	while (removed_area.width() * removed_area.height() >
			0.3 * (new_area.width() * new_area.height())) {
		// It's a loop just to be able to break from it.
		
		// There is a special case when there is nothing but
		// garbage on the page.  Let's try to handle it here.
		if (removed_area.width() < 6 || removed_area.height() < 6) {
			break;
		}
		
		if (dbg) {
			QPainter painter(&visualized);
			painter.setPen(Qt::NoPen);
			painter.setBrush(QColor(0x5f, 0xdf, 0x57, 50));
			painter.drawRect(removed_area);
			painter.drawRect(new_area);
			painter.end();
			dbg->add(visualized, "trim_too_much");
		}
		return area;
	}
	
	int const content_pixels = content.countBlackPixels(removed_area);
	
	bool const vertical_cut = (
		new_area.top() == area.top()
		&& new_area.bottom() == area.bottom()
	);
	//qDebug() << "vertical cut: " << vertical_cut;
	
	// Ranged from 0.0 to 1.0.  When it's less than 0.5, objects
	// are more likely to be considered as garbage.  When it's
	// more than 0.5, objects are less likely to be considered
	// as garbage.
	double proximity_bias = vertical_cut ? 0.5 : 0.65;
	
	int const num_text_pixels = text.countBlackPixels(removed_area);
	if (num_text_pixels == 0) {
		proximity_bias = vertical_cut ? 0.4 : 0.5;
	} else {
		int total_pixels = content_pixels;
		total_pixels += garbage.image().countBlackPixels(removed_area);
		
		//qDebug() << "num_text_pixels = " << num_text_pixels;
		//qDebug() << "total_pixels = " << total_pixels;
		++total_pixels; // just in case
		
		double const min_text_influence = 0.2;
		double const max_text_influence = 1.0;
		int const upper_threshold = 5000;
		double text_influence = max_text_influence;
		if (num_text_pixels < upper_threshold) {
			text_influence = min_text_influence +
					(max_text_influence - min_text_influence)
					* log((double)num_text_pixels) / log((double)upper_threshold);
		}
		//qDebug() << "text_influence = " << text_influence;
		
		proximity_bias += (1.0 - proximity_bias) * text_influence
				* num_text_pixels / total_pixels;
		proximity_bias = qBound(0.0, proximity_bias, 1.0);
	}
	
	BinaryImage remaining_content(content_blocks.size(), WHITE);
	rasterOp<RopSrc>(
		remaining_content, new_area,
		content, new_area.topLeft()
	);
	rasterOp<RopAnd<RopSrc, RopDst> >(
		remaining_content, new_area,
		content_blocks, new_area.topLeft()
	);
	
	SEDM const dm_to_others(
		remaining_content, SEDM::DIST_TO_BLACK,
		SEDM::DIST_TO_NO_BORDERS
	);
	remaining_content.release();
	
	double sum_dist_to_garbage = 0;
	double sum_dist_to_others = 0;
	
	uint32_t const* cb_line = content_blocks.data();
	int const cb_stride = content_blocks.wordsPerLine();
	uint32_t const msb = uint32_t(1) << 31;
	
	uint32_t const* dm_garbage_line = garbage.sedm().data();
	uint32_t const* dm_others_line = dm_to_others.data();
	int const dm_stride = dm_to_others.stride();
	
	int count = 0;
	cb_line += cb_stride * removed_area.top();
	dm_garbage_line += dm_stride * removed_area.top();
	dm_others_line += dm_stride * removed_area.top();
	for (int y = removed_area.top(); y <= removed_area.bottom(); ++y) {
		for (int x = removed_area.left(); x <= removed_area.right(); ++x) {
			if (cb_line[x >> 5] & (msb >> (x & 31))) {
				sum_dist_to_garbage += sqrt((double)dm_garbage_line[x]);
				sum_dist_to_others += sqrt((double)dm_others_line[x]);
				++count;
			}
		}
		cb_line += cb_stride;
		dm_garbage_line += dm_stride;
		dm_others_line += dm_stride;
	}
	
	//qDebug() << "proximity_bias = " << proximity_bias;
	//qDebug() << "sum_dist_to_garbage = " << sum_dist_to_garbage;
	//qDebug() << "sum_dist_to_others = " << sum_dist_to_others;
	//qDebug() << "count = " << count;
	
	sum_dist_to_garbage *= proximity_bias;
	sum_dist_to_others *= 1.0 - proximity_bias;
	
	if (sum_dist_to_garbage < sum_dist_to_others) {
		garbage.add(content, removed_area);
		if (dbg) {
			QPainter painter(&visualized);
			painter.setPen(Qt::NoPen);
			painter.setBrush(QColor(0x5f, 0xdf, 0x57, 50));
			painter.drawRect(new_area);
			painter.setBrush(QColor(0xff, 0x20, 0x1e, 50));
			painter.drawRect(removed_area);
			painter.end();
			dbg->add(visualized, "trimmed");
		}
		return new_area;
	} else {
		if (dbg) {
			QPainter painter(&visualized);
			painter.setPen(Qt::NoPen);
			painter.setBrush(QColor(0x5f, 0xdf, 0x57, 50));
			painter.drawRect(removed_area);
			painter.drawRect(new_area);
			painter.end();
			dbg->add(visualized, "not_trimmed");
		}
		can_retry_grouped = (proximity_bias < 0.85);
		return area;
	}
}

void
ContentBoxFinder::filterShadows(
	TaskStatus const& status, imageproc::BinaryImage& shadows,
	DebugImages* const dbg)
{
	// The input image should only contain shadows from the edges
	// of a page, but in practice it may also contain things like
	// a black table header which white letters on it.  Here we
	// try to filter them out.
	
#if 1
	// Shadows that touch borders are genuine and should not be removed.
	BinaryImage borders(shadows.size(), WHITE);
	borders.fillExcept(borders.rect().adjusted(1, 1, -1, -1), BLACK);
	
	BinaryImage touching_shadows(seedFill(borders, shadows, CONN8));
	rasterOp<RopXor<RopSrc, RopDst> >(shadows, touching_shadows);
	if (dbg) {
		dbg->add(shadows, "non_border_shadows");
	}
	
	if (shadows.countBlackPixels()) {
		BinaryImage inv_shadows(shadows.inverted());
		BinaryImage mask(seedFill(borders, inv_shadows, CONN8));
		borders.release();
		rasterOp<RopOr<RopNot<RopDst>, RopSrc> >(mask, shadows);
		if (dbg) {
			dbg->add(mask, "shadows_no_holes");
		}
		
		BinaryImage text_mask(estimateTextMask(inv_shadows, mask, dbg));
		inv_shadows.release();
		mask.release();
		text_mask = seedFill(text_mask, shadows, CONN8);
		if (dbg) {
			dbg->add(text_mask, "misclassified_shadows");
		}
		rasterOp<RopXor<RopSrc, RopDst> >(shadows, text_mask);
	}
	
	rasterOp<RopOr<RopSrc, RopDst> >(shadows, touching_shadows);
#else
	// White dots on black background may be a problem for us.
	// They may be misclassified as parts of white letters.
	BinaryImage reduced_dithering(closeBrick(shadows, QSize(1, 2), BLACK));
	reduced_dithering = closeBrick(reduced_dithering, QSize(2, 1), BLACK);
	if (dbg) {
		dbg->add(reduced_dithering, "reduced_dithering");
	}
	
	status.throwIfCancelled();
	
	// Long white vertical lines are definitely not spaces between letters.
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
#endif
}


/*====================== ContentBoxFinder::Garbage =====================*/

ContentBoxFinder::Garbage::Garbage(
	Type const type, BinaryImage const& garbage)
:	m_garbage(garbage),
	m_sedmBorders(
		type == VERT
		? SEDM::DIST_TO_VERT_BORDERS
		: SEDM::DIST_TO_HOR_BORDERS
	),
	m_sedmUpdatePending(true)
{
}

void
ContentBoxFinder::Garbage::add(
	BinaryImage const& garbage, QRect const& rect)
{
	rasterOp<RopOr<RopSrc, RopDst> >(
		m_garbage, rect, garbage, rect.topLeft()
	);
	m_sedmUpdatePending = true;
}

SEDM const&
ContentBoxFinder::Garbage::sedm()
{
	if (m_sedmUpdatePending) {
		m_sedm = SEDM(m_garbage, SEDM::DIST_TO_BLACK, m_sedmBorders);
	}
	return m_sedm;
}

} // namespace select_content
