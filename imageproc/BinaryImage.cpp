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

#include "BinaryImage.h"
#include "ByteOrder.h"
#include "BitOps.h"
#include <QAtomicInt>
#include <QImage>
#include <QRect>
#include <new>
#include <memory>
#include <stdexcept>
#include <algorithm>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

namespace imageproc
{

class BinaryImage::SharedData
{
private:
	/**
	 * Resolves the ambiguity of:
	 * \code
	 * void SharedData::operator delete(void*, size_t);
	 * \endcode
	 * Which may be interpreted as both a placement and non-placement delete.
	 */
	struct NumWords
	{
		size_t numWords;

		NumWords(size_t num_words) : numWords(num_words) {}
	};
public:
	static SharedData* create(size_t num_words) {
		return new(NumWords(num_words)) SharedData();
	}
	
	uint32_t* data() { return m_data; }
	
	uint32_t const* data() const { return m_data; }
	
	bool isShared() const {
		return m_refCounter.fetchAndAddRelaxed(0) > 1;
	}
	
	void ref() const { m_refCounter.ref(); }
	
	void unref() const;
	
	static void* operator new(size_t size, NumWords num_words);
	
	static void operator delete(void* addr, NumWords num_words);
private:
	SharedData() : m_refCounter(1) {}
	
	SharedData& operator=(SharedData const&); // forbidden
	
	mutable QAtomicInt m_refCounter;
	uint32_t m_data[1]; // more data follows
};

BinaryImage::BinaryImage()
:	m_pData(0),
	m_width(0),
	m_height(0),
	m_wpl(0)
{
}

BinaryImage::BinaryImage(int const width, int const height)
:	m_width(width),
	m_height(height),
	m_wpl((width + 31) / 32)
{
	if (m_width > 0 && m_height > 0) {
		m_pData = SharedData::create(m_height * m_wpl);
	} else {
		throw std::invalid_argument("BinaryImage dimensions are wrong");
	}
}

BinaryImage::BinaryImage(QSize const size)
:	m_width(size.width()),
	m_height(size.height()),
	m_wpl((size.width() + 31) / 32)
{
	if (m_width > 0 && m_height > 0) {
		m_pData = SharedData::create(m_height * m_wpl);
	} else {
		throw std::invalid_argument("BinaryImage dimensions are wrong");
	}
}

BinaryImage::BinaryImage(int const width, int const height, BWColor const color)
:	m_width(width),
	m_height(height),
	m_wpl((width + 31) / 32)
{
	if (m_width > 0 && m_height > 0) {
		m_pData = SharedData::create(m_height * m_wpl);
	} else {
		throw std::invalid_argument("BinaryImage dimensions are wrong");
	}
	fill(color);
}

BinaryImage::BinaryImage(QSize const size, BWColor const color)
:	m_width(size.width()),
	m_height(size.height()),
	m_wpl((size.width() + 31) / 32)
{
	if (m_width > 0 && m_height > 0) {
		m_pData = SharedData::create(m_height * m_wpl);
	} else {
		throw std::invalid_argument("BinaryImage dimensions are wrong");
	}
	fill(color);
}

BinaryImage::BinaryImage(int const width, int const height, SharedData* const data)
:	m_pData(data),
	m_width(width),
	m_height(height),
	m_wpl((width + 31) / 32)
{
}

BinaryImage::BinaryImage(BinaryImage const& other)
:	m_pData(other.m_pData),
	m_width(other.m_width),
	m_height(other.m_height),
	m_wpl(other.m_wpl)
{
	if (m_pData) {
		m_pData->ref();
	}
}

BinaryImage::BinaryImage(QImage const& image, BinaryThreshold const threshold)
:	m_pData(0),
	m_width(0),
	m_height(0),
	m_wpl(0)
{
	QRect const image_rect(image.rect());
	
	switch (image.format()) {
		case QImage::Format_Invalid:
			break;
		case QImage::Format_Mono:
			*this = fromMono(image);
			break;
		case QImage::Format_MonoLSB:
			*this = fromMonoLSB(image);
			break;
		case QImage::Format_Indexed8:
			*this = fromIndexed8(image, image_rect, threshold);
			break;
		case QImage::Format_RGB32:
		case QImage::Format_ARGB32:
			*this = fromRgb32(image, image_rect, threshold);
			break;
		case QImage::Format_ARGB32_Premultiplied:
			*this = fromArgb32Premultiplied(image, image_rect, threshold);
			break;
		case QImage::Format_RGB16:
			*this = fromRgb16(image, image_rect, threshold);
			break;
		default:
			throw std::runtime_error("Unsupported QImage format");
	}
}

BinaryImage::BinaryImage(
	QImage const& image, QRect const& rect, BinaryThreshold const threshold)
:	m_pData(0),
	m_width(0),
	m_height(0),
	m_wpl(0)
{
	if (rect.isEmpty()) {
		return;
	} else if (rect.intersected(image.rect()) != rect) {
		throw std::invalid_argument("BinaryImage: rect exceedes the QImage");
	}
	
	switch (image.format()) {
		case QImage::Format_Invalid:
			break;
		case QImage::Format_Mono:
			*this = fromMono(image, rect);
			break;
		case QImage::Format_MonoLSB:
			*this = fromMonoLSB(image, rect);
			break;
		case QImage::Format_Indexed8:
			*this = fromIndexed8(image, rect, threshold);
			break;
		case QImage::Format_RGB32:
		case QImage::Format_ARGB32:
			*this = fromRgb32(image, rect, threshold);
			break;
		case QImage::Format_ARGB32_Premultiplied:
			*this = fromArgb32Premultiplied(image, rect, threshold);
			break;
		case QImage::Format_RGB16:
			*this = fromRgb16(image, rect, threshold);
			break;
		default:
			throw std::runtime_error("Unsupported QImage format");
	}
}

BinaryImage::~BinaryImage()
{
	if (m_pData) {
		m_pData->unref();
	}
}

BinaryImage&
BinaryImage::operator=(BinaryImage const& other)
{
	BinaryImage(other).swap(*this);
	return *this;
}

void
BinaryImage::swap(BinaryImage& other)
{
	std::swap(m_pData, other.m_pData);
	std::swap(m_width, other.m_width);
	std::swap(m_height, other.m_height);
	std::swap(m_wpl, other.m_wpl);
}

void
BinaryImage::invert()
{
	if (isNull()) {
		return;
	}
	
	size_t const num_words = m_height * m_wpl;
	
	assert(m_pData);
	if (!m_pData->isShared()) {
		// In-place operation
		uint32_t* data = this->data();
		for (size_t i = 0; i < num_words; ++i, ++data) {
			*data = ~*data;
		}
	} else {
		SharedData* new_data = SharedData::create(num_words);
		
		uint32_t const* src_data = m_pData->data();
		uint32_t* dst_data = new_data->data();
		for (size_t i = 0; i < num_words; ++i, ++src_data, ++dst_data) {
			*dst_data = ~*src_data;
		}
		
		m_pData->unref();
		m_pData = new_data;
	}
}

BinaryImage
BinaryImage::inverted() const
{
	if (isNull()) {
		return BinaryImage();
	}
	
	size_t const num_words = m_height * m_wpl;
	SharedData* new_data = SharedData::create(num_words);
	
	uint32_t const* src_data = m_pData->data();
	uint32_t* dst_data = new_data->data();
	for (size_t i = 0; i < num_words; ++i, ++src_data, ++dst_data) {
		*dst_data = ~*src_data;
	}
	
	return BinaryImage(m_width, m_height, new_data);
}

void
BinaryImage::fill(BWColor const color)
{
	if (isNull()) {
		throw std::logic_error("Attempt to fill a null BinaryImage!");
	}
	
	int const pattern = (color == BLACK) ? ~0 : 0;
	memset(data(), pattern, m_height * m_wpl * 4);
}

void
BinaryImage::fill(QRect const& rect, BWColor const color)
{
	if (rect.isEmpty()) {
		return;
	}
	
	fillRectImpl(data(), rect.intersected(this->rect()), color);
}

void
BinaryImage::fillExcept(QRect const& rect, BWColor const color)
{
	if (isNull()) {
		throw std::logic_error("Attempt to fill a null BinaryImage!");
	}
	
	if (rect.contains(this->rect())) {
		return;
	}
	
	QRect const bounded_rect(rect.intersected(this->rect()));
	if (bounded_rect.isEmpty()) {
		fill(color);
		return;
	}
	
	int const pattern = (color == BLACK) ? ~0 : 0;
	uint32_t* const data = this->data(); // this will call copyIfShared()
	
	if (bounded_rect.top() > 0) {
		memset(data, pattern, bounded_rect.top() * m_wpl * 4);
	}
	
	int y_top = bounded_rect.top();
	if (bounded_rect.left() > 0) {
		QRect const left_rect(
			0, y_top,
			bounded_rect.left(), bounded_rect.height()
		);
		fillRectImpl(data, left_rect, color);
	}
	
	int const x_left = bounded_rect.left() + bounded_rect.width();
	if (x_left < m_width) {
		QRect const right_rect(
			x_left, y_top,
			m_width - x_left, bounded_rect.height()
		);
		fillRectImpl(data, right_rect, color);
	}
	
	y_top = bounded_rect.top() + bounded_rect.height();
	if (y_top < m_height) {
		memset(data + y_top * m_wpl, pattern, (m_height - y_top) * m_wpl * 4);
	}
}

void
BinaryImage::fillFrame(
	QRect const& outer_rect, QRect const& inner_rect, BWColor const color)
{
	if (isNull()) {
		throw std::logic_error("Attempt to fill a null BinaryImage!");
	}
	
	QRect const bounded_outer_rect(outer_rect.intersected(this->rect()));
	QRect const bounded_inner_rect(inner_rect.intersected(bounded_outer_rect));
	if (bounded_inner_rect == bounded_outer_rect) {
		return;
	} else if (bounded_inner_rect.isEmpty()) {
		fill(bounded_outer_rect, color);
		return;
	}
	
	uint32_t* const data = this->data();
	
	QRect top_rect(bounded_outer_rect);
	top_rect.setBottom(bounded_inner_rect.top() - 1);
	if (top_rect.height() != 0) {
		fillRectImpl(data, top_rect, color);
	}
	
	QRect left_rect(bounded_inner_rect);
	left_rect.setLeft(bounded_outer_rect.left());
	left_rect.setRight(bounded_inner_rect.left() - 1);
	if (left_rect.width() != 0) {
		fillRectImpl(data, left_rect, color);
	}
	
	QRect right_rect(bounded_inner_rect);
	right_rect.setRight(bounded_outer_rect.right());
	right_rect.setLeft(bounded_inner_rect.right() + 1);
	if (right_rect.width() != 0) {
		fillRectImpl(data, right_rect, color);
	}
	
	QRect bottom_rect(bounded_outer_rect);
	bottom_rect.setTop(bounded_inner_rect.bottom() + 1);
	if (bottom_rect.height() != 0) {
		fillRectImpl(data, bottom_rect, color);
	}
}

int
BinaryImage::countBlackPixels() const
{
	return countBlackPixels(rect());
}

int
BinaryImage::countWhitePixels() const
{
	return countWhitePixels(rect());
}

int
BinaryImage::countBlackPixels(QRect const& rect) const
{
	QRect const r(rect.intersected(this->rect()));
	if (r.isEmpty()) {
		return 0;
	}
	
	int const top = r.top();
	int const bottom = r.bottom();
	int const first_word_idx = r.left() >> 5;
	int const last_word_idx = r.right() >> 5; // r.right() is within rect
	uint32_t const first_word_mask = ~uint32_t(0) >> (r.left() & 31);
	int const last_word_unused_bits = (last_word_idx << 5) + 31 - r.right();
	uint32_t const last_word_mask = ~uint32_t(0) << last_word_unused_bits;
	uint32_t const* line = data() + top * m_wpl;
	
	int count = 0;
	
	if (first_word_idx == last_word_idx) {
		if (r.width() == 1) {
			for (int y = top; y <= bottom; ++y, line += m_wpl) {
				count += (line[first_word_idx] >> last_word_unused_bits) & 1;
			}
		} else {
			uint32_t const mask = first_word_mask & last_word_mask;
			for (int y = top; y <= bottom; ++y, line += m_wpl) {
				count += countNonZeroBits(line[first_word_idx] & mask);
			}
		}
	} else {
		for (int y = top; y <= bottom; ++y, line += m_wpl) {
			int idx = first_word_idx;
			count += countNonZeroBits(line[idx] & first_word_mask);
			for (++idx; idx != last_word_idx; ++idx) {
				count += countNonZeroBits(line[idx]);
			}
			count += countNonZeroBits(line[idx] & last_word_mask);
		}
	}
	
	return count;
}

int
BinaryImage::countWhitePixels(QRect const& rect) const
{
	QRect const r(rect.intersected(this->rect()));
	if (r.isEmpty()) {
		return 0;
	}
	
	return r.width() * r.height() - countBlackPixels(r);
}

QRect
BinaryImage::contentBoundingBox(BWColor const content_color) const
{
	if (isNull()) {
		return QRect();
	}
	
	int const w = m_width;
	int const h = m_height;
	int const wpl = m_wpl;
	int const last_word_idx = (w - 1) >> 5;
	int const last_word_bits = w - (last_word_idx << 5);
	int const last_word_unused_bits = 32 - last_word_bits;
	uint32_t const last_word_mask = ~uint32_t(0) << last_word_unused_bits;
	uint32_t const modifier = (content_color == WHITE) ? ~uint32_t(0) : 0;
	uint32_t const* const data = this->data();
	
	int bottom = -1; // inclusive
	uint32_t const* line = data + h * wpl;
	for (int y = h - 1; y >= 0; --y) {
		line -= wpl;
		if (!isLineMonotone(line, last_word_idx, last_word_mask, modifier)) {
			bottom = y;
			break;
		}
	}
	
	if (bottom == -1) {
		return QRect();
	}
	
	int top = bottom;
	line = data;
	for (int y = 0; y < bottom; ++y, line += wpl) {
		if (!isLineMonotone(line, last_word_idx, last_word_mask, modifier)) {
			top = y;
			break;
		}
	}
	
	// These are offsets from the corresponding side.
	int left = w;
	int right = w;
	
	assert(line == data + top * wpl);
	
	// All lines above top and below bottom are empty.
	for (int y = top; y <= bottom; ++y, line += wpl) {
		if (left != 0) {
			left = leftmostBitOffset(line, left, modifier);
		}
		if (right != 0) {
			uint32_t const word =
				(line[last_word_idx] ^ modifier) >> last_word_unused_bits;
			if (word) {
				int const offset = countLeastSignificantZeroes(word);
				if (offset < right) {
					right = offset;
				}
			} else if (right > last_word_bits) {
				right -= last_word_bits;
				right = rightmostBitOffset(
					line + last_word_idx, right, modifier
				);
				right += last_word_bits;
			}
		}
	}
	
	// bottom is inclusive, right is a positive offset from width.
	return QRect(left, top, w - right - left, bottom - top + 1);
}

uint32_t*
BinaryImage::data()
{
	if (isNull()) {
		return 0;
	}
	
	copyIfShared();
	return m_pData->data();
}

uint32_t const*
BinaryImage::data() const
{
	if (isNull()) {
		return 0;
	}
	return m_pData->data();
}

QImage
BinaryImage::toQImage() const
{
	if (isNull()) {
		return QImage();
	}
	
	QImage dst(m_width, m_height, QImage::Format_Mono);
	assert(dst.bytesPerLine() % 4 == 0);
	dst.setNumColors(2);
	dst.setColor(0, 0xffffffff);
	dst.setColor(1, 0xff000000);
	int const dst_wpl = dst.bytesPerLine() / 4;
	uint32_t* dst_line = (uint32_t*)dst.bits();
	uint32_t const* src_line = data();
	int const src_wpl = m_wpl;
	
	for (int i = m_height; i > 0; --i) {
		for (int j = 0; j < src_wpl; ++j) {
			dst_line[j] = htonl(src_line[j]);
		}
		src_line += src_wpl;
		dst_line += dst_wpl;
	}
	
	return dst;
}

QImage
BinaryImage::toAlphaMask(QColor const& color) const
{
	if (isNull()) {
		return QImage();
	}

	int const alpha = color.alpha();
	int const red = (color.red() * alpha + 128) / 255;
	int const green = (color.green() * alpha + 128) / 255;
	int const blue = (color.blue() * alpha + 128) / 255;
	uint32_t const colors[] = {
		0, // replaces white
		qRgba(red, green, blue, alpha) // replaces black
	};
	
	int const width = m_width;
	int const height = m_height;

	QImage dst(width, height, QImage::Format_ARGB32_Premultiplied);
	assert(dst.bytesPerLine() % 4 == 0);
	
	int const dst_stride = dst.bytesPerLine() / 4;
	uint32_t* dst_line = (uint32_t*)dst.bits();
	
	uint32_t const* src_line = data();
	int const src_stride = m_wpl;
	
	for (int y = 0; y < height; y++) {
		for (int x = 0; x < width; ++x) {
			dst_line[x] = colors[(src_line[x >> 5] >> (31 - (x & 31))) & 1];
		}
		src_line += src_stride;
		dst_line += dst_stride;
	}
	
	return dst;
}

void
BinaryImage::copyIfShared()
{
	assert(m_pData);
	if (!m_pData->isShared()) {
		return;
	}
	
	size_t const num_words = m_height * m_wpl;
	SharedData* new_data = SharedData::create(num_words);
	memcpy(new_data->data(), m_pData->data(), num_words * 4);
	m_pData->unref();
	m_pData = new_data;
}

void
BinaryImage::fillRectImpl(uint32_t* const data, QRect const& rect, BWColor const color)
{
	uint32_t const pattern = (color == BLACK) ? ~uint32_t(0) : 0;
	
	if (rect.x() == 0 && rect.width() == m_width) {
		memset(data + rect.y() * m_wpl, pattern, rect.height() * m_wpl * 4);
		return;
	}
	
	uint32_t const first_word_idx = rect.left() >> 5;
	// Note: rect.right() == rect.left() + rect.width() - 1
	uint32_t const last_word_idx = rect.right() >> 5;
	uint32_t const first_word_mask = ~uint32_t(0) >> (rect.left() & 31);
	uint32_t const last_word_mask = ~uint32_t(0) << (31 - (rect.right() & 31));
	uint32_t* line = data + rect.top() * m_wpl;
	
	if (first_word_idx == last_word_idx) {
		line += first_word_idx;
		uint32_t const mask = first_word_mask & last_word_mask;
		for (int i = rect.height(); i > 0; --i, line += m_wpl) {
			*line = (*line & ~mask) | (pattern & mask);
		}
		return;
	}
	
	for (int i = rect.height(); i > 0; --i, line += m_wpl) {
		// First word in a line.
		uint32_t* pword = &line[first_word_idx];
		*pword = (*pword & ~first_word_mask) | (pattern & first_word_mask);
		
		uint32_t* last_pword = &line[last_word_idx];
		for (++pword; pword != last_pword; ++pword) {
			*pword = pattern;
		}
		
		// Last word in a line.
		*pword = (*pword & ~last_word_mask) | (pattern & last_word_mask);
	}
}

BinaryImage
BinaryImage::fromMono(QImage const& image)
{
	int const width = image.width();
	int const height = image.height();
	
	assert(image.bytesPerLine() % 4 == 0);
	int const src_wpl = image.bytesPerLine() / 4;
	uint32_t const* src_line = (uint32_t const*)image.bits();
	
	BinaryImage dst(width, height);
	int const dst_wpl = dst.wordsPerLine();
	uint32_t* dst_line = dst.data();
	
	uint32_t modifier = ~uint32_t(0);
	if (image.numColors() >= 2) {
		if (qGray(image.color(0)) > qGray(image.color(1))) {
			// if color 0 is lighter than color 1
			modifier = ~modifier;
		}
	}
	
	for (int i = height; i > 0; --i) {
		for (int j = 0; j < dst_wpl; ++j) {
			dst_line[j] = ntohl(src_line[j]) ^ modifier;
		}
		src_line += src_wpl;
		dst_line += dst_wpl;
	}
	
	return dst;
}

BinaryImage
BinaryImage::fromMono(QImage const& image, QRect const& rect)
{
	int const width = rect.width();
	int const height = rect.height();
	
	assert(image.bytesPerLine() % 4 == 0);
	int const src_wpl = image.bytesPerLine() / 4;
	uint32_t const* src_line = (uint32_t const*)image.bits();
	src_line += rect.top() * src_wpl;
	src_line += rect.left() >> 5;
	int const word1_unused_bits = rect.left() & 31;
	int const word2_unused_bits = 32 - word1_unused_bits;
	
	BinaryImage dst(width, height);
	int const dst_wpl = dst.wordsPerLine();
	uint32_t* dst_line = dst.data();
	int const dst_last_word_unused_bits = (dst_wpl << 5) - width;
	
	uint32_t modifier = ~uint32_t(0);
	if (image.numColors() >= 2) {
		if (qGray(image.color(0)) > qGray(image.color(1))) {
			// if color 0 is lighter than color 1
			modifier = ~modifier;
		}
	}
	
	if (word1_unused_bits == 0) {
		// It's not just an optimization.  The code in the other branch
		// is not going to work for this case because uint32_t << 32
		// does not actually clear the word.
		for (int i = height; i > 0; --i) {
			for (int j = 0; j < dst_wpl; ++j) {
				dst_line[j] = ntohl(src_line[j]) ^ modifier;
			}
			src_line += src_wpl;
			dst_line += dst_wpl;
		}
	} else {
		int const last_word_idx = (width - 1) >> 5;
		for (int i = height; i > 0; --i) {
			int j = 0;
			uint32_t next_word = ntohl(src_line[j]);
			for (; j < last_word_idx; ++j) {
				uint32_t const this_word = next_word;
				next_word = ntohl(src_line[j + 1]);
				uint32_t const dst_word = (this_word << word1_unused_bits)
					| (next_word >> word2_unused_bits);
				dst_line[j] = dst_word ^ modifier;
			}
			
			// The last word needs special attention, because src_line[j + 1]
			// might be outside of the image buffer.
			uint32_t last_word = next_word << word1_unused_bits;
			if (dst_last_word_unused_bits < word1_unused_bits) {
				last_word |= ntohl(src_line[j + 1]) >> word2_unused_bits;
			}
			dst_line[j] = last_word ^ modifier;
			
			src_line += src_wpl;
			dst_line += dst_wpl;
		}
	}
	
	return dst;
}

BinaryImage
BinaryImage::fromMonoLSB(QImage const& image)
{
	return fromMono(image.convertToFormat(QImage::Format_Mono));
}

BinaryImage
BinaryImage::fromMonoLSB(QImage const& image, QRect const& rect)
{
	return fromMono(image.convertToFormat(QImage::Format_Mono), rect);
}

BinaryImage
BinaryImage::fromIndexed8(
	QImage const& image, QRect const& rect, int const threshold)
{
	int const width = rect.width();
	int const height = rect.height();
	
	int const src_bpl = image.bytesPerLine();
	uint8_t const* src_line = image.bits();
	src_line += rect.top() * src_bpl + rect.left();
	
	BinaryImage dst(width, height);
	int const dst_wpl = dst.wordsPerLine();
	uint32_t* dst_line = dst.data();
	int const last_word_idx = (width - 1) >> 5;
	int const last_word_bits = width - (last_word_idx << 5);
	int const last_word_unused_bits = 32 - last_word_bits;
	
	int const num_colors = image.numColors();
	assert(num_colors <= 256);
	int color_to_gray[256];
	int color_idx = 0;
	for (; color_idx < num_colors; ++color_idx) {
		color_to_gray[color_idx] = qGray(image.color(color_idx));
	}
	for (; color_idx < 256; ++color_idx) {
		color_to_gray[color_idx] = 0; // just in case
	}
	
	for (int i = height; i > 0; --i) {
		for (int j = 0; j < last_word_idx; ++j) {
			uint8_t const* const src_pos = &src_line[j << 5];
			uint32_t word = 0;
			for (int bit = 0; bit < 32; ++bit) {
				word <<= 1;
				if (color_to_gray[src_pos[bit]] < threshold) {
					word |= uint32_t(1);
				}
			}
			dst_line[j] = word;
		}
		
		// Handle the last word.
		uint8_t const* const src_pos = &src_line[last_word_idx << 5];
		uint32_t word = 0;
		for (int bit = 0; bit < last_word_bits; ++bit) {
			word <<= 1;
			if (color_to_gray[src_pos[bit]] < threshold) {
				word |= uint32_t(1);
			}
		}
		word <<= last_word_unused_bits;
		dst_line[last_word_idx] = word;
		
		dst_line += dst_wpl;
		src_line += src_bpl;
	}
	return dst;
}

static inline uint32_t thresholdRgb32(QRgb const c, int const threshold)
{
	// gray = (R * 11 + G * 16 + B * 5) / 32;
	// return (gray < threshold) ? 1 : 0;
	
	int const sum = qRed(c)*11 + qGreen(c)*16 + qBlue(c)*5;
	return (sum < threshold * 32) ? 1 : 0;
}

BinaryImage
BinaryImage::fromRgb32(
	QImage const& image, QRect const& rect, int const threshold)
{
	int const width = rect.width();
	int const height = rect.height();
	
	assert(image.bytesPerLine() % 4 == 0);
	int const src_wpl = image.bytesPerLine() / 4;
	QRgb const* src_line = (QRgb const*)image.bits();
	src_line += rect.top() * src_wpl + rect.left();
	
	BinaryImage dst(width, height);
	int const dst_wpl = dst.wordsPerLine();
	uint32_t* dst_line = dst.data();
	int const last_word_idx = (width - 1) >> 5;
	int const last_word_bits = width - (last_word_idx << 5);
	int const last_word_unused_bits = 32 - last_word_bits;
	
	for (int i = height; i > 0; --i) {
		for (int j = 0; j < last_word_idx; ++j) {
			QRgb const* const src_pos = &src_line[j << 5];
			uint32_t word = 0;
			for (int bit = 0; bit < 32; ++bit) {
				word <<= 1;
				word |= thresholdRgb32(src_pos[bit], threshold);
			}
			dst_line[j] = word;
		}
		
		// Handle the last word.
		QRgb const* const src_pos = &src_line[last_word_idx << 5];
		uint32_t word = 0;
		for (int bit = 0; bit < last_word_bits; ++bit) {
			word <<= 1;
			word |= thresholdRgb32(src_pos[bit], threshold);
		}
		word <<= last_word_unused_bits;
		dst_line[last_word_idx] = word;
		
		dst_line += dst_wpl;
		src_line += src_wpl;
	}
	return dst;
}

static inline uint32_t thresholdArgbPM(QRgb const pm, int const threshold)
{
	int const alpha = qAlpha(pm);
	if (alpha == 0) {
		return 1; // black
	}
	
	// R = R_PM * 255 / alpha;
	// G = G_PM * 255 / alpha;
	// B = B_PM * 255 / alpha;
	// gray = (R * 11 + G * 16 + B * 5) / 32;
	// return (gray < threshold) ? 1 : 0;
	
	int const sum = qRed(pm)*(255*11) + qGreen(pm)*(255*16) + qBlue(pm)*(255*5);
	return (sum < alpha * threshold * 32) ? 1 : 0;
}

BinaryImage
BinaryImage::fromArgb32Premultiplied(
	QImage const& image, QRect const& rect, int const threshold)
{
	int const width = rect.width();
	int const height = rect.height();
	
	assert(image.bytesPerLine() % 4 == 0);
	int const src_wpl = image.bytesPerLine() / 4;
	QRgb const* src_line = (QRgb const*)image.bits();
	src_line += rect.top() * src_wpl + rect.left();
	
	BinaryImage dst(width, height);
	int const dst_wpl = dst.wordsPerLine();
	uint32_t* dst_line = dst.data();
	int const last_word_idx = (width - 1) >> 5;
	int const last_word_bits = width - (last_word_idx << 5);
	int const last_word_unused_bits = 32 - last_word_bits;
	
	for (int i = height; i > 0; --i) {
		for (int j = 0; j < last_word_idx; ++j) {
			QRgb const* const src_pos = &src_line[j << 5];
			uint32_t word = 0;
			for (int bit = 0; bit < 32; ++bit) {
				word <<= 1;
				word |= thresholdArgbPM(src_pos[bit], threshold);
			}
			dst_line[j] = word;
		}
		
		// Handle the last word.
		QRgb const* const src_pos = &src_line[last_word_idx << 5];
		uint32_t word = 0;
		for (int bit = 0; bit < last_word_bits; ++bit) {
			word <<= 1;
			word |= thresholdArgbPM(src_pos[bit], threshold);
		}
		word <<= last_word_unused_bits;
		dst_line[last_word_idx] = word;
		
		dst_line += dst_wpl;
		src_line += src_wpl;
	}
	return dst;
}

static inline uint32_t thresholdRgb16(uint16_t const c16, int const threshold)
{
	int const c = c16;
	
	// rgb16: RRRRR GGGGGG BBBBB
	//        43210 543210 43210
	
	// r8 = r5 * 8 + r5 / 4 = RRRRR RRR
	//                        43210 432
	int const r8 = ((c >> 8) & 0xF8) | ((c >> 13) & 0x07);
	
	// g8 = g6 * 4 + g6 / 16 = GGGGGG GG
	//                         543210 54
	int const g8 = ((c >> 3) & 0xFC) | ((c >> 9) & 0x03);
	
	// b8 = b5 * 8 + b5 / 4 = BBBBB BBB
	//                        43210 432
	int const b8 = ((c << 3) & 0xF8) | ((c >> 2) & 0x07);
	
	// gray = (R * 11 + G * 16 + B * 5) / 32;
	// return (gray < threshold) ? 1 : 0;
	
	int const sum = r8*11 + g8*16 + b8*5;
	return (sum < threshold * 32) ? 1 : 0;
}

BinaryImage
BinaryImage::fromRgb16(
	QImage const& image, QRect const& rect, int const threshold)
{
	int const width = rect.width();
	int const height = rect.height();
	
	assert(image.bytesPerLine() % 4 == 0);
	int const src_wpl = image.bytesPerLine() / 2;
	uint16_t const* src_line = (uint16_t const*)image.bits();
	
	BinaryImage dst(width, height);
	int const dst_wpl = dst.wordsPerLine();
	uint32_t* dst_line = dst.data();
	int const last_word_idx = (width - 1) >> 5;
	int const last_word_bits = width - (last_word_idx << 5);
	
	for (int i = height; i > 0; --i) {
		for (int j = 0; j < last_word_idx; ++j) {
			uint16_t const* const src_pos = &src_line[j << 5];
			uint32_t word = 0;
			for (int bit = 0; bit < 32; ++bit) {
				word <<= 1;
				word |= thresholdRgb16(src_pos[bit], threshold);
			}
			dst_line[j] = word;
		}
		
		// Handle the last word.
		uint16_t const* const src_pos = &src_line[last_word_idx << 5];
		uint32_t word = 0;
		for (int bit = 0; bit < last_word_bits; ++bit) {
			word <<= 1;
			word |= thresholdRgb16(src_pos[bit], threshold);
		}
		word <<= 32 - last_word_bits;
		dst_line[last_word_idx] = word;
		
		dst_line += dst_wpl;
		src_line += src_wpl;
	}
	return dst;
}

/**
 * \brief Determines if the line is either completely black or completely white.
 *
 * \param line The line.
 * \param last_word_idx Index of the last (possibly incomplete) word.
 * \param last_word_mask The mask to by applied to the last word.
 * \param modifier If 0, this function check if the line is completely black.
 *        If ~uint32_t(0), this function checks if the line is completely white.
 */
bool
BinaryImage::isLineMonotone(
	uint32_t const* const line, int const last_word_idx,
	uint32_t const last_word_mask, uint32_t const modifier)
{
	for (int i = 0; i < last_word_idx; ++i) {
		uint32_t const word = line[i] ^ modifier;
		if (word) {
			return false;
		}
	}
	
	// The last (possibly incomplete) word.
	int const word = (line[last_word_idx] ^ modifier) & last_word_mask;
	if (word) {
		return false;
	}
	
	return true;
}

int
BinaryImage::leftmostBitOffset(
	uint32_t const* const line, int const offset_limit, uint32_t const modifier)
{
	int const num_words = (offset_limit + 31) >> 5;
	
	int bit_offset = offset_limit;
	
	uint32_t const* pword = line;
	for (int i = 0; i < num_words; ++i, ++pword) {
		uint32_t const word = *pword ^ modifier;
		if (word) {
			bit_offset = (i << 5) + countMostSignificantZeroes(word);
			break;
		}
	}
	
	return std::min(bit_offset, offset_limit);
}

int
BinaryImage::rightmostBitOffset(
	uint32_t const* const line, int const offset_limit, uint32_t const modifier)
{
	int const num_words = (offset_limit + 31) >> 5;
	
	int bit_offset = offset_limit;
	
	uint32_t const* pword = line - 1; // line points to last_word_idx, which we skip
	for (int i = 0; i < num_words; ++i, --pword) {
		uint32_t const word = *pword ^ modifier;
		if (word) {
			bit_offset = (i << 5) + countLeastSignificantZeroes(word);
			break;
		}
	}
	
	return std::min(bit_offset, offset_limit);
}

bool operator==(BinaryImage const& lhs, BinaryImage const& rhs)
{
	if (lhs.data() == rhs.data()) {
		// This will also catch the case when both are null.
		return true;
	}
	
	if (lhs.width() != rhs.width() ||
	    lhs.height() != rhs.height()) {
		// This will also catch the case when one is null while the other is not.
		return false;
	}
	
	uint32_t const* lhs_line = lhs.data();
	uint32_t const* rhs_line = rhs.data();
	int const lhs_wpl = lhs.wordsPerLine();
	int const rhs_wpl = rhs.wordsPerLine();
	int const last_bit_idx = lhs.width() - 1;
	int const last_word_idx = last_bit_idx / 32;
	uint32_t const last_word_mask = ~uint32_t(0) << (31 - last_bit_idx % 32);
	
	for (int i = lhs.height(); i > 0; --i) {
		int j = 0;
		for (; j < last_word_idx; ++j) {
			if (lhs_line[j] != rhs_line[j]) {
				return false;
			}
		}
		
		// Handle the last (possibly incomplete) word.
		if ((lhs_line[j] & last_word_mask) != (rhs_line[j] & last_word_mask)) {
			return false;
		}
		
		lhs_line += lhs_wpl;
		rhs_line += rhs_wpl;
	}
	
	return true;
}


/*====================== BinaryIamge::SharedData ========================*/

void
BinaryImage::SharedData::unref() const
{
	if (!m_refCounter.deref()) {
		this->~SharedData();
		free((void*)this);
	}
}

void*
BinaryImage::SharedData::operator new(size_t, NumWords const num_words)
{
	SharedData* sd = 0;
	void* addr = malloc(((char*)&sd->m_data[0] - (char*)sd) + num_words.numWords * 4);
	if (!addr) {
		throw std::bad_alloc();
	}
	return addr;
}

void
BinaryImage::SharedData::operator delete(void* addr, NumWords)
{
	free(addr);
}

} // namespace imageproc
