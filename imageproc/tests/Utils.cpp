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

#include "Utils.h"
#include "BinaryImage.h"
#include <QImage>
#include <QRect>
#include <stdint.h>
#include <stdlib.h>
#include <assert.h>

namespace imageproc
{

namespace tests
{

namespace utils
{

BinaryImage
randomBinaryImage(int const width, int const height)
{
	BinaryImage image(width, height);
	uint32_t* pword = image.data();
	uint32_t* const end = pword + image.height() * image.wordsPerLine();
	for (; pword != end; ++pword) {
		uint32_t const w1 = rand() % (1 << 16);
		uint32_t const w2 = rand() % (1 << 16);
		*pword = (w1 << 16) | w2;
	}
	return image;
}

QImage
randomMonoQImage(int const width, int const height)
{
	QImage image(width, height, QImage::Format_Mono);
	image.setNumColors(2);
	image.setColor(0, 0xffffffff);
	image.setColor(1, 0xff000000);
	uint32_t* pword = (uint32_t*)image.bits();
	assert(image.bytesPerLine() % 4 == 0);
	uint32_t* const end = pword + image.height() * (image.bytesPerLine() / 4);
	for (; pword != end; ++pword) {
		uint32_t const w1 = rand() % (1 << 16);
		uint32_t const w2 = rand() % (1 << 16);
		*pword = (w1 << 16) | w2;
	}
	return image;
}

BinaryImage makeBinaryImage(int const* data, int const width, int const height)
{
	return BinaryImage(makeMonoQImage(data, width, height));
}

QImage makeMonoQImage(int const* data, int const width, int const height)
{
	QImage img(width, height, QImage::Format_Mono);
	img.setNumColors(2);
	img.setColor(0, 0xffffffff);
	img.setColor(1, 0xff000000);
	for (int y = 0; y < height; ++y) {
		for (int x = 0; x < width; ++x) {
			img.setPixel(x, y, data[y * width + x] ? 1 : 0);
		}
	}
	return img;
}

bool surroundingsIntact(QImage const& img1, QImage const& img2, QRect const& rect)
{
	assert(img1.size() == img2.size());
	
	int const w = img1.width();
	int const h = img1.height();
	
	if (rect.left() != 0) {
		QRect const left_of(0, 0, rect.x(), h);
		if (img1.copy(left_of) != img2.copy(left_of)) {
			return false;
		}
	}
	
	if (rect.right() != img1.rect().right()) {
		QRect const right_of(rect.x() + w, 0, w - rect.x() - rect.width(), h);
		if (img1.copy(right_of) != img2.copy(right_of)) {
			return false;
		}
	}
	
	if (rect.top() != 0) {
		QRect const top_of(0, 0, w, rect.y());
		if (img1.copy(top_of) != img2.copy(top_of)) {
			return false;
		}
	}
	
	if (rect.bottom() != img1.rect().bottom()) {
		QRect const bottom_of(
			0, rect.y() + rect.height(), w, h - rect.y() - rect.height()
		);
		if (img1.copy(bottom_of) != img2.copy(bottom_of)) {
			return false;
		}
	}
	
	return true;
}

} // namespace utils

} // namespace tests

} // namespace imageproc
