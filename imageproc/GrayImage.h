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

#ifndef IMAGEPROC_GRAYIMAGE_H_
#define IMAGEPROC_GRAYIMAGE_H_

#include <QImage>
#include <QSize>
#include <QRect>
#include <stdint.h>

namespace imageproc
{

/**
 * \brief A wrapper class around QImage that is always guaranteed to be 8-bit grayscale.
 */
class GrayImage
{
public:
	/**
	 * \brief Creates a 8-bit grayscale image with specified dimensions.
	 *
	 * The image contents won't be initialized.  You can use fill() to initialize them.
	 * If size.isEmpty() is true, creates a null image.
	 *
	 * \throw std::bad_alloc Unlike the underlying QImage, GrayImage reacts to
	 *        out-of-memory situations by throwing an exception rather than
	 *        constructing a null image.
	 */
	explicit GrayImage(QSize size = QSize());

	/**
	 * \brief Constructs a 8-bit grayscale image by converting an arbitrary QImage.
	 *
	 * The QImage may be in any format and may be null.
	 */
	explicit GrayImage(QImage const& image);

	/**
	 * \brief Returns a const reference to the underlying QImage.
	 *
	 * The underlying QImage is either a null image or a 8-bit indexed
	 * image with a grayscale palette.
	 */
	QImage const& toQImage() const { return m_image; }

	operator QImage const&() const { return m_image; }

	bool isNull() const { return m_image.isNull(); }

	void fill(uint8_t color) { m_image.fill(color); }

	uint8_t* data() { return m_image.bits(); }

	uint8_t const* data() const { return m_image.bits(); }

	/**
	 * \brief Number of bytes per line.
	 *
	 * This value may be larger than image width.
	 * An additional guaranee provided by the underlying QImage
	 * is that this value is a multiple of 4.
	 */
	int stride() const { return m_image.bytesPerLine(); }

	QSize size() const { return m_image.size(); }

	QRect rect() const { return m_image.rect(); }

	int width() const { return m_image.width(); }

	int height() const { return m_image.height(); }
private:
	QImage m_image;
};

inline bool operator==(GrayImage const& lhs, GrayImage const& rhs) {
	return lhs.toQImage() == rhs.toQImage();
}

inline bool operator!=(GrayImage const& lhs, GrayImage const& rhs) {
	return lhs.toQImage() != rhs.toQImage();
}

} // namespace imageproc

#endif
