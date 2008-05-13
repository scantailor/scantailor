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

#ifndef TIFFREADER_H_
#define TIFFREADER_H_

#include "ImageMetadataLoader.h"
#include "VirtualFunction.h"

class QIODevice;
class QImage;
class ImageMetadata;
class Dpi;

class TiffReader
{
public:
	static bool canRead(QIODevice& device);
	
	static ImageMetadataLoader::Status readMetadata(
		QIODevice& device,
		VirtualFunction1<void, ImageMetadata const&>& out);
	
	/**
	 * \brief Reads the image from io device to QImage.
	 *
	 * \param device The device to read from.  This device must be
	 *        opened for reading and must be seekable.
	 * \param page_num A zero-based page number within a multi-page
	 *        TIFF file.
	 * \return The resulting image, or a null image in case of failure.
	 */
	static QImage readImage(QIODevice& device, int page_num = 0);
private:
	class TiffHeader;
	class TiffHandle;
	struct TiffInfo;
	template<typename T> class TiffBuffer;
	
	static TiffHeader readHeader(QIODevice& device);
	
	static bool checkHeader(TiffHeader const& header);
	
	static ImageMetadata currentPageMetadata(TiffHandle const& tif);
	
	static Dpi getDpi(float xres, float yres, unsigned res_unit);
	
	static QImage extractBinaryOrIndexed8Image(
		TiffHandle const& tif, TiffInfo const& info);
	
	static void readLines(TiffHandle const& tif, QImage& image);
	
	static void readAndUnpackLines(
		TiffHandle const& tif, TiffInfo const& info, QImage& image);
};

#endif
