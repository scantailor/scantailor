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

#include "TiffReader.h"
#include "ImageMetadata.h"
#include "NonCopyable.h"
#include "Dpi.h"
#include "Dpm.h"
#include <QtGlobal>
#include <QSysInfo>
#include <QIODevice>
#include <QImage>
#include <QColor>
#include <QSize>
#include <QDebug>
#include <algorithm>
#include <tiff.h>
#include <tiffio.h>
#include <new>
#include <assert.h>

class TiffReader::TiffHeader
{
public:
	enum Signature { INVALID_SIGNATURE, TIFF_BIG_ENDIAN, TIFF_LITTLE_ENDIAN };
	
	TiffHeader() : m_signature(INVALID_SIGNATURE), m_version(0) {}
	
	TiffHeader(Signature signature, int version)
	: m_signature(signature), m_version(version) {}
	
	Signature signature() const { return m_signature; }
	
	int version() const { return m_version; }
private:
	Signature m_signature;
	int m_version;
};


class TiffReader::TiffHandle
{
public:
	TiffHandle(TIFF* handle) : m_pHandle(handle) {}
	
	~TiffHandle() { if (m_pHandle) TIFFClose(m_pHandle); }
	
	TIFF* handle() const { return m_pHandle; }
private:
	TIFF* m_pHandle;
};


template<typename T>
class TiffReader::TiffBuffer
{
	DECLARE_NON_COPYABLE(TiffBuffer)
public:
	TiffBuffer() : m_pData(0) {}

	TiffBuffer(tsize_t num_items) {
		m_pData = (T*)_TIFFmalloc(num_items * sizeof(T));
		if (!m_pData) {
			throw std::bad_alloc();
		}
	}
	
	~TiffBuffer() {
		if (m_pData) {
			_TIFFfree(m_pData);
		}
	}
	
	T* data() { return m_pData; }

	void swap(TiffBuffer& other) {
		std::swap(m_pData, other.m_pData);
	}
private:
	T* m_pData;
};


struct TiffReader::TiffInfo
{
	int width;
	int height;
	uint16 bits_per_sample;
	uint16 samples_per_pixel;
	uint16 sample_format;
	uint16 photometric;
	bool host_big_endian;
	bool file_big_endian;
	
	TiffInfo(TiffHandle const& tif, TiffHeader const& header);
	
	bool mapsToBinaryOrIndexed8() const;
};


TiffReader::TiffInfo::TiffInfo(TiffHandle const& tif, TiffHeader const& header)
:	width(0),
	height(0),
	bits_per_sample(1),
	samples_per_pixel(1),
	sample_format(SAMPLEFORMAT_UINT),
	photometric(PHOTOMETRIC_MINISBLACK),
	host_big_endian(QSysInfo::ByteOrder == QSysInfo::BigEndian),
	file_big_endian(header.signature() == TiffHeader::TIFF_BIG_ENDIAN)
{
	uint16 compression = 1;
	TIFFGetField(tif.handle(), TIFFTAG_COMPRESSION, &compression);
	switch (compression) {
		case COMPRESSION_CCITTFAX3:
		case COMPRESSION_CCITTFAX4:
		case COMPRESSION_CCITTRLE:
		case COMPRESSION_CCITTRLEW:
			photometric = PHOTOMETRIC_MINISWHITE;
	}
	
	TIFFGetField(tif.handle(), TIFFTAG_IMAGEWIDTH, &width);
	TIFFGetField(tif.handle(), TIFFTAG_IMAGELENGTH, &height);
	TIFFGetField(tif.handle(), TIFFTAG_BITSPERSAMPLE, &bits_per_sample);
	TIFFGetField(tif.handle(), TIFFTAG_SAMPLESPERPIXEL, &samples_per_pixel);
	TIFFGetField(tif.handle(), TIFFTAG_SAMPLEFORMAT, &sample_format);
	TIFFGetField(tif.handle(), TIFFTAG_PHOTOMETRIC, &photometric);
}

bool
TiffReader::TiffInfo::mapsToBinaryOrIndexed8() const
{
	if (samples_per_pixel != 1 || sample_format != SAMPLEFORMAT_UINT || bits_per_sample > 8) {
		return false;
	}
	
	switch (photometric) {
		case PHOTOMETRIC_PALETTE:
		case PHOTOMETRIC_MINISBLACK:
		case PHOTOMETRIC_MINISWHITE:
			return true;
	}
	
	return false;
}


static tsize_t deviceRead(thandle_t context, tdata_t data, tsize_t size)
{
	QIODevice* dev = (QIODevice*)context;
	return (tsize_t)dev->read(static_cast<char*>(data), size);
}

static tsize_t deviceWrite(thandle_t context, tdata_t data, tsize_t size)
{
	// Not implemented.
	return 0;
}

static toff_t deviceSeek(thandle_t context, toff_t offset, int whence)
{
	QIODevice* dev = (QIODevice*)context;
	
	switch (whence) {
		case SEEK_SET:
			dev->seek(offset);
			break;
		case SEEK_CUR:
			dev->seek(dev->pos() + offset);
			break;
		case SEEK_END:
			dev->seek(dev->size() + offset);
			break;
	}
	
	return dev->pos();
}

static int deviceClose(thandle_t context)
{
	QIODevice* dev = (QIODevice*)context;
	dev->close();
	return 0;
}

static toff_t deviceSize(thandle_t context)
{
	QIODevice* dev = (QIODevice*)context;
	return dev->size();
}

static int deviceMap(thandle_t, tdata_t*, toff_t*)
{
	// Not implemented.
	return 0;
}

static void deviceUnmap(thandle_t, tdata_t, toff_t)
{
	// Not implemented.
}


bool
TiffReader::canRead(QIODevice& device)
{
	if (!device.isReadable()) {
		return false;
	}
	if (device.isSequential()) {
		// libtiff needs to be able to seek.
		return false;
	}
	
	TiffHeader header(readHeader(device));
	return checkHeader(header);
}

ImageMetadataLoader::Status
TiffReader::readMetadata(
	QIODevice& device,
	VirtualFunction1<void, ImageMetadata const&>& out)
{
	if (!device.isReadable()) {
		return ImageMetadataLoader::GENERIC_ERROR;
	}
	if (device.isSequential()) {
		// libtiff needs to be able to seek.
		return ImageMetadataLoader::GENERIC_ERROR;
	}
	
	if (!checkHeader(TiffHeader(readHeader(device)))) {
		return ImageMetadataLoader::FORMAT_NOT_RECOGNIZED;
	}
	
	TiffHandle tif(
		TIFFClientOpen(
			"file", "rBm", &device, &deviceRead, &deviceWrite,
			&deviceSeek, &deviceClose, &deviceSize,
			&deviceMap, &deviceUnmap
		)
	);
	if (!tif.handle()) {
		return ImageMetadataLoader::GENERIC_ERROR;
	}
	
	do {
		out(currentPageMetadata(tif));
	} while (TIFFReadDirectory(tif.handle()));
	
	return ImageMetadataLoader::LOADED;
}

static void convertAbgrToArgb(uint32 const* src, uint32* dst, int count)
{
	for (int i = 0; i < count; ++i) {
		uint32 const src_word = src[i];
		uint32 dst_word = src_word & 0xFF000000;   // A
		dst_word |= (src_word & 0x00FF0000) >> 16; // B
		dst_word |= src_word & 0x0000FF00;         // G
		dst_word |= (src_word & 0x000000FF) << 16; // R
		dst[i] = dst_word;
	}
}

QImage
TiffReader::readImage(QIODevice& device, int const page_num)
{
	if (!device.isReadable()) {
		return QImage();
	}
	if (device.isSequential()) {
		// libtiff needs to be able to seek.
		return QImage();
	}
	
	TiffHeader header(readHeader(device));
	if (!checkHeader(header)) {
		return QImage();
	}
	
	TiffHandle tif(
		TIFFClientOpen(
			"file", "rBm", &device, &deviceRead, &deviceWrite,
			&deviceSeek, &deviceClose, &deviceSize,
			&deviceMap, &deviceUnmap
		)
	);
	if (!tif.handle()) {
		return QImage();
	}
	
	if (!TIFFSetDirectory(tif.handle(), page_num)) {
		return QImage();
	}
	
	TiffInfo const info(tif, header);
	
	ImageMetadata const metadata(currentPageMetadata(tif));
	
	QImage image;
	
	if (info.mapsToBinaryOrIndexed8()) {
		// Common case optimization.
		image = extractBinaryOrIndexed8Image(tif, info);
	} else {
		// General case.
		image = QImage(
			info.width, info.height,
			info.samples_per_pixel == 3
			? QImage::Format_RGB32 : QImage::Format_ARGB32
		);
		if (image.isNull()) {
			throw std::bad_alloc();
		}

		// For ABGR -> ARGB conversion.
		TiffBuffer<uint32> tmp_buffer;
		uint32 const* src_line = 0;

		if (image.bytesPerLine() == 4 * info.width) {
			// We can avoid creating a temporary buffer in this case.
			if (!TIFFReadRGBAImageOriented(tif.handle(), info.width, info.height,
			                               (uint32*)image.bits(), ORIENTATION_TOPLEFT, 0)) {
				return QImage();
			}
			src_line = (uint32 const*)image.bits();
		} else {
			TiffBuffer<uint32>(info.width * info.height).swap(tmp_buffer);
			if (!TIFFReadRGBAImageOriented(tif.handle(), info.width, info.height,
				                           tmp_buffer.data(), ORIENTATION_TOPLEFT, 0)) {
				return QImage();
			}
			src_line = tmp_buffer.data();
		}
		
		uint32* dst_line = (uint32*)image.bits();
		assert(image.bytesPerLine() % 4 == 0);
		int const dst_stride = image.bytesPerLine() / 4;
		for (int y = 0; y < info.height; ++y) {
			convertAbgrToArgb(src_line, dst_line, info.width);
			src_line += info.width;
			dst_line += dst_stride;
		}
	}
	
	if (!metadata.dpi().isNull()) {
		Dpm const dpm(metadata.dpi());
		image.setDotsPerMeterX(dpm.horizontal());
		image.setDotsPerMeterY(dpm.vertical());
	}
	
	return image;
}

TiffReader::TiffHeader
TiffReader::readHeader(QIODevice& device)
{
	unsigned char data[4];
	if (device.peek((char*)data, sizeof(data)) != sizeof(data)) {
		return TiffHeader();
	}
	
	uint16 const version_byte0 = data[2];
	uint16 const version_byte1 = data[3];
	
	if (data[0] == 0x4d && data[1] == 0x4d) {
		uint16 const version = (version_byte0 << 8) + version_byte1;
		return TiffHeader(TiffHeader::TIFF_BIG_ENDIAN, version);
	} else if (data[0] == 0x49 && data[1] == 0x49) {
		uint16 const version = (version_byte1 << 8) + version_byte0;
		return TiffHeader(TiffHeader::TIFF_LITTLE_ENDIAN, version);
	} else {
		return TiffHeader();
	}
}

bool
TiffReader::checkHeader(TiffHeader const& header)
{
	if (header.signature() == TiffHeader::INVALID_SIGNATURE) {
		return false;
	}
	if (header.version() != 42 && header.version() != 43) {
		return false;
	}
	return true;
}

ImageMetadata
TiffReader::currentPageMetadata(TiffHandle const& tif)
{
	uint32 width = 0, height = 0;
	float xres = 0, yres = 0;
	uint16 res_unit = 0;
	TIFFGetField(tif.handle(), TIFFTAG_IMAGEWIDTH, &width);
	TIFFGetField(tif.handle(), TIFFTAG_IMAGELENGTH, &height);
	TIFFGetField(tif.handle(), TIFFTAG_XRESOLUTION, &xres);
	TIFFGetField(tif.handle(), TIFFTAG_YRESOLUTION, &yres);
	TIFFGetFieldDefaulted(tif.handle(), TIFFTAG_RESOLUTIONUNIT, &res_unit);
	return ImageMetadata(QSize(width, height), getDpi(xres, yres, res_unit));
}

Dpi
TiffReader::getDpi(float xres, float yres, unsigned res_unit)
{
	switch (res_unit) {
		case RESUNIT_INCH: // inch
			return Dpi(qRound(xres), qRound(yres));
		case RESUNIT_CENTIMETER: // cm
			return Dpm(qRound(xres * 100), qRound(yres * 100));
	}
	return Dpi();
}

QImage
TiffReader::extractBinaryOrIndexed8Image(
	TiffHandle const& tif, TiffInfo const& info)
{
	QImage::Format format = QImage::Format_Indexed8;
	if (info.bits_per_sample == 1) {
		// Because we specify B option when opening, we can
		// always use Format_Mono, and not Format_MonoLSB.
		format = QImage::Format_Mono;
	}
	
	QImage image(info.width, info.height, format);
	if (image.isNull()) {
		throw std::bad_alloc();
	}
	
	int const num_colors = 1 << info.bits_per_sample;
	image.setNumColors(num_colors);
	
	if (info.photometric == PHOTOMETRIC_PALETTE) {
		uint16* pr = 0;
		uint16* pg = 0;
		uint16* pb = 0;
		TIFFGetField(tif.handle(), TIFFTAG_COLORMAP, &pr, &pg, &pb);
		if (!pr || !pg || !pb) {
			return QImage();
		}
		if (info.host_big_endian != info.file_big_endian) {
			TIFFSwabArrayOfShort(pr, num_colors);
			TIFFSwabArrayOfShort(pg, num_colors);
			TIFFSwabArrayOfShort(pb, num_colors);
		}
		double const f = 255.0/65535.0;
		for (int i = 0; i < num_colors; ++i) {
			uint32 const r = (uint32)(pr[i] * f + 0.5);
			uint32 const g = (uint32)(pg[i] * f + 0.5);
			uint32 const b = (uint32)(pb[i] * f + 0.5);
			uint32 const a = 0xFF000000;
			image.setColor(i, a | (r << 16) | (g << 8) | b);
		}
	} else if (info.photometric == PHOTOMETRIC_MINISBLACK) {
		double const f = 255.0 / (num_colors - 1);
		for (int i = 0; i < num_colors; ++i) {
			int const gray = (int)(i * f + 0.5);
			image.setColor(i, qRgb(gray, gray, gray));
		}
	} else if (info.photometric == PHOTOMETRIC_MINISWHITE) {
		double const f = 255.0 / (num_colors - 1);
		int c = num_colors - 1;
		for (int i = 0; i < num_colors; ++i, --c) {
			int const gray = (int)(c * f + 0.5);
			image.setColor(i, qRgb(gray, gray, gray));
		}
	} else {
		return QImage();
	}
	
	if (info.bits_per_sample == 1 || info.bits_per_sample == 8) {
		readLines(tif, image);
	} else {
		readAndUnpackLines(tif, info, image);
	}
	
	return image;
}

void
TiffReader::readLines(TiffHandle const& tif, QImage& image)
{
	int const height = image.height();
	for (int y = 0; y < height; ++y) {
		TIFFReadScanline(tif.handle(), image.scanLine(y), y);
	}
}

void
TiffReader::readAndUnpackLines(
	TiffHandle const& tif, TiffInfo const& info, QImage& image)
{
	TiffBuffer<uint8> buf(TIFFScanlineSize(tif.handle()));
	
	int const width = image.width();
	int const height = image.height();
	int const bits_per_sample = info.bits_per_sample;
	unsigned const dst_mask = (1 << bits_per_sample) - 1;
	
	for (int y = 0; y < height; ++y) {
		TIFFReadScanline(tif.handle(), buf.data(), y);
		
		unsigned accum = 0;
		int bits_in_accum = 0;
		
		uint8 const* src = buf.data();
		uint8* dst = (uint8*)image.scanLine(y);
		
		for (int i = width; i > 0; --i, ++dst) {
			while (bits_in_accum < bits_per_sample) {
				accum <<= 8;
				accum |= *src;
				bits_in_accum += 8;
				++src;
			}
			bits_in_accum -= bits_per_sample;
			*dst = static_cast<uint8>((accum >> bits_in_accum) & dst_mask);
		}
	}
}
