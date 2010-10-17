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

#ifndef IMAGEPROC_GRAYSCALE_H_
#define IMAGEPROC_GRAYSCALE_H_

#include "GrayImage.h"
#include <QVector>
#include <QColor>

class QImage;
class QSize;

namespace imageproc
{

class BinaryImage;

class GrayscaleHistogram
{
public:
	explicit GrayscaleHistogram(QImage const& img);
	
	GrayscaleHistogram(QImage const& img, BinaryImage const& mask);
	
	int& operator[](int idx) { return m_pixels[idx]; }
	
	int operator[](int idx) const { return m_pixels[idx]; }
private:
	void fromMonoImage(QImage const& img);
	
	void fromMonoMSBImage(QImage const& img, BinaryImage const& mask);
	
	void fromGrayscaleImage(QImage const& img);
	
	void fromGrayscaleImage(QImage const& img, BinaryImage const& mask);
	
	void fromAnyImage(QImage const& img);
	
	void fromAnyImage(QImage const& img, BinaryImage const& mask);
	
	int m_pixels[256];
};

/**
 * \brief Create a 256-element grayscale palette.
 */
QVector<QRgb> createGrayscalePalette();

/**
 * \brief Convert an image from any format to grayscale.
 *
 * \param src The source image in any format.
 * \return A grayscale image with proper palette.  Null will be returned
 *         if \p src was null.
 */
QImage toGrayscale(QImage const& src);

/**
 * \brief Stetch the distribution of gray levels to cover the whole range.
 *
 * \param src The source image.  It doesn't have to be grayscale.
 * \param black_clip_fraction The fraction of pixels (fractions of 1) that are
 *        allowed to go negative.  Such pixels will be clipped to 0 (black).
 * \param white_clip_fraction The fraction of pixels (fractions of 1) that are
 *        allowed to exceed the maximum brightness level.  Such pixels will be
 *        clipped to 255 (white).
 * \return A grayscale image, or a null image, if \p src was null.
 */
GrayImage stretchGrayRange(GrayImage const& src, double black_clip_fraction = 0.0,
	double white_clip_fraction = 0.0);

/**
 * \brief Create a grayscale image consisting of a 1 pixel frame and an inner area.
 *
 * \param size The size of the image including the frame.
 * \param inner_color The gray level of the inner area.  Defaults to white.
 * \param frame_color The gray level of the frame area.  Defaults to black.
 * \return The resulting image.
 */
GrayImage createFramedImage(
	QSize const& size, unsigned char inner_color = 0xff,
	unsigned char frame_color = 0x00);

/**
 * \brief Find the darkest gray level of an image.
 *
 * \param image The image to process.  If it's null, 0xff will
 *        be returned as the darkest image.  If it's not grayscale,
 *        a grayscale copy will be created.
 */
unsigned char darkestGrayLevel(QImage const& image);

} // namespace imageproc

#endif
