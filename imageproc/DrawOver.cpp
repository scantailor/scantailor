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

#include "DrawOver.h"
#include "BinaryImage.h"
#include "RasterOp.h"
#include <QImage>
#include <QRect>
#include <QSize>
#include <stdexcept>
#include <stdint.h>
#include <string.h>
#include <assert.h>

namespace imageproc
{

void drawOver(
	QImage& dst, QRect const& dst_rect,
	QImage const& src, QRect const& src_rect)
{
	if (src_rect.size() != dst_rect.size()) {
		throw std::invalid_argument("drawOver: source and destination areas have different sizes");
	}
	if (dst.format() != src.format()) {
		throw std::invalid_argument("drawOver: source and destination have different formats");
	}
	if (dst_rect.intersected(dst.rect()) != dst_rect) {
		throw std::invalid_argument("drawOver: destination area exceeds the image");
	}
	if (src_rect.intersected(src.rect()) != src_rect) {
		throw std::invalid_argument("drawOver: source area exceeds the image");
	}
	
	uint8_t* dst_line = dst.bits();
	int const dst_bpl = dst.bytesPerLine();
	
	uint8_t const* src_line = src.bits();
	int const src_bpl = src.bytesPerLine();
	
	int const depth = src.depth();
	assert(dst.depth() == depth);
	
	if (depth % 8 != 0) {
		assert(depth == 1);
		
		// Slow but simple.
		BinaryImage dst_bin(dst);
		BinaryImage src_bin(src);
		rasterOp<RopSrc>(dst_bin, dst_rect, src_bin, src_rect.topLeft());
		dst = dst_bin.toQImage().convertToFormat(dst.format());
		// FIXME: we are not preserving the color table.
		
		return;
	}
	
	int const stripe_bytes = src_rect.width() * depth / 8;
	dst_line += dst_bpl * dst_rect.top() + dst_rect.left() * depth / 8;
	src_line += src_bpl * src_rect.top() + src_rect.left() * depth / 8;
	
	for (int i = src_rect.height(); i > 0; --i) {
		memcpy(dst_line, src_line, stripe_bytes);
		dst_line += dst_bpl;
		src_line += src_bpl;
	}
}

} // namespace imageproc
