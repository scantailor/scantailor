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

#include "ConnCompEraserExt.h"
#include "RasterOp.h"
#include <QRect>
#ifndef Q_MOC_RUN
#include <boost/cstdint.hpp>
#endif
#include <string.h>
#include <stddef.h>
#include <assert.h>

namespace imageproc
{

ConnCompEraserExt::ConnCompEraserExt(
	BinaryImage const& image, Connectivity const conn)
:	m_eraser(image, conn),
	m_lastImage(image)
{
}

ConnComp
ConnCompEraserExt::nextConnComp()
{
	if (!m_lastCC.isNull()) {
		// Propagate the changes from m_eraser.image() to m_lastImage.
		// We could copy the whole image, but instead we copy just
		// the affected area, extending it to word boundries.
		QRect const& rect = m_lastCC.rect();
		BinaryImage const& src = m_eraser.image();
		size_t const src_wpl = src.wordsPerLine();
		size_t const dst_wpl = m_lastImage.wordsPerLine();
		size_t const first_word_idx = rect.left() / 32;
		// Note: rect.right() == rect.x() + rect.width() - 1
		size_t const span_length = (rect.right() + 31) / 32 - first_word_idx;
		size_t const src_initial_offset = rect.top() * src_wpl + first_word_idx;
		size_t const dst_initial_offset = rect.top() * dst_wpl + first_word_idx;
		uint32_t const* src_pos = src.data() + src_initial_offset;
		uint32_t* dst_pos = m_lastImage.data() + dst_initial_offset;
		for (int i = rect.height(); i > 0; --i) {
			memcpy(dst_pos, src_pos, span_length * 4);
			src_pos += src_wpl;
			dst_pos += dst_wpl;
		}
	}
	
	m_lastCC = m_eraser.nextConnComp();
	return m_lastCC;
}

BinaryImage
ConnCompEraserExt::computeConnCompImage() const
{
	if (m_lastCC.isNull()) {
		return BinaryImage();
	}
	
	return computeDiffImage(m_lastCC.rect());
}

BinaryImage
ConnCompEraserExt::computeConnCompImageAligned(QRect* rect) const
{
	if (m_lastCC.isNull()) {
		return BinaryImage();
	}
	
	QRect r(m_lastCC.rect());
	r.setX((r.x() >> 5) << 5);
	if (rect) {
		*rect = r;
	}
	return computeDiffImage(r);
}

BinaryImage
ConnCompEraserExt::computeDiffImage(QRect const& rect) const
{
	BinaryImage diff(rect.width(), rect.height());
	rasterOp<RopSrc>(diff, diff.rect(), m_eraser.image(), rect.topLeft());
	rasterOp<RopXor<RopSrc, RopDst> >(diff, diff.rect(), m_lastImage, rect.topLeft());
	return diff;
}

} // namespace imageproc
