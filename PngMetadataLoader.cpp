/*
    Scan Tailor - Interactive post-processing tool for scanned pages.
	Copyright (C) 2007-2009  Joseph Artsimovich <joseph_a@mail.ru>

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

#include "PngMetadataLoader.h"
#include "ImageMetadata.h"
#include "NonCopyable.h"
#include "Dpi.h"
#include "Dpm.h"
#include <QIODevice>
#include <QSize>
#include <new> // for std::bad_alloc
#include <png.h>
#include <setjmp.h>

namespace
{

class PngHandle
{
	DECLARE_NON_COPYABLE(PngHandle)
public:
	PngHandle();
	
	~PngHandle();
	
	png_structp handle() const { return m_pPng; }
	
	png_infop info() const { return m_pInfo; }
private:
	png_structp m_pPng;
	png_infop m_pInfo;
};

PngHandle::PngHandle()
{
	m_pPng = png_create_read_struct(PNG_LIBPNG_VER_STRING, 0, 0, 0);
	if (!m_pPng) {
		throw std::bad_alloc();
	}
	m_pInfo = png_create_info_struct(m_pPng);
	if (!m_pInfo) {
		throw std::bad_alloc();
	}
}

PngHandle::~PngHandle()
{
	png_destroy_read_struct(&m_pPng, &m_pInfo, 0);
}

} // anonymous namespace

static void readFn(png_structp png_ptr, png_bytep data, png_size_t length)
{
	QIODevice* io_device = (QIODevice*)png_get_io_ptr(png_ptr);
	while (length > 0) {
		qint64 const read = io_device->read((char*)data, length);
		if (read <= 0) {
			png_error(png_ptr, "Read Error");
			return;
		}
		length -= read;
	}
}

void
PngMetadataLoader::registerMyself()
{
	static bool registered = false;
	if (!registered) {
		ImageMetadataLoader::registerLoader(
			IntrusivePtr<ImageMetadataLoader>(new PngMetadataLoader)
		);
		registered = true;
	}
}

ImageMetadataLoader::Status
PngMetadataLoader::loadMetadata(
	QIODevice& io_device,
	VirtualFunction1<void, ImageMetadata const&>& out)
{
	if (!io_device.isReadable()) {
		return GENERIC_ERROR;
	}

	png_byte signature[8];
	if (io_device.peek((char*)signature, 8) != 8) {
		return FORMAT_NOT_RECOGNIZED;
	}
	
	if (png_sig_cmp(signature, 0, sizeof(signature)) != 0) {
		return FORMAT_NOT_RECOGNIZED;
	}
	
	PngHandle png;
	
	if (setjmp(png_jmpbuf(png.handle()))) {
		return GENERIC_ERROR;
	}
	
	png_set_read_fn(png.handle(), &io_device, &readFn);
	png_read_info(png.handle(), png.info());
	
	QSize size;
	Dpi dpi;
	size.setWidth(png_get_image_width(png.handle(), png.info()));
	size.setHeight(png_get_image_height(png.handle(), png.info()));
	png_uint_32 res_x, res_y;
	int unit_type;
	if (png_get_pHYs(png.handle(), png.info(), &res_x, &res_y, &unit_type)) {
		if (unit_type == PNG_RESOLUTION_METER) {
			dpi = Dpm(res_x, res_y);
		}
	}
	
	out(ImageMetadata(size, dpi));
	return LOADED;
}

