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

#include "JpegMetadataLoader.h"
#include "ImageMetadata.h"
#include "NonCopyable.h"
#include "Dpi.h"
#include "Dpm.h"
#include <QIODevice>
#include <QSize>
#include <QDebug>
#include <setjmp.h>
#include <string.h>
#include <assert.h>

extern "C" {
#include <jpeglib.h>
}

namespace
{

/*======================== JpegDecompressionHandle =======================*/

class JpegDecompressHandle
{
	DECLARE_NON_COPYABLE(JpegDecompressHandle)
public:
	JpegDecompressHandle(jpeg_error_mgr* err_mgr, jpeg_source_mgr* src_mgr);
	
	~JpegDecompressHandle();
	
	jpeg_decompress_struct* ptr() { return &m_info; }
	
	jpeg_decompress_struct* operator->() { return &m_info; }
private:
	jpeg_decompress_struct m_info;
};

JpegDecompressHandle::JpegDecompressHandle(
	jpeg_error_mgr* err_mgr, jpeg_source_mgr* src_mgr)
{
	m_info.err = err_mgr;
	jpeg_create_decompress(&m_info);
	m_info.src = src_mgr;
}

JpegDecompressHandle::~JpegDecompressHandle()
{
	jpeg_destroy_decompress(&m_info);
}


/*============================ JpegSourceManager =========================*/

class JpegSourceManager : public jpeg_source_mgr
{
	DECLARE_NON_COPYABLE(JpegSourceManager)
public:
	JpegSourceManager(QIODevice& io_device);
private:
	static void initSource(j_decompress_ptr cinfo);
	
	static boolean fillInputBuffer(j_decompress_ptr cinfo);
	
	boolean fillInputBufferImpl();
	
	static void skipInputData(j_decompress_ptr cinfo, long num_bytes);
	
	void skipInputDataImpl(long num_bytes);
	
	static void termSource(j_decompress_ptr cinfo);
	
	static JpegSourceManager* object(j_decompress_ptr cinfo);
	
	QIODevice& m_rDevice;
	JOCTET m_buf[4096];
};

JpegSourceManager::JpegSourceManager(QIODevice& io_device)
:	m_rDevice(io_device)
{
	init_source = &JpegSourceManager::initSource;
	fill_input_buffer = &JpegSourceManager::fillInputBuffer;
	skip_input_data = &JpegSourceManager::skipInputData;
	resync_to_restart = &jpeg_resync_to_restart;
	term_source = &JpegSourceManager::termSource;
	bytes_in_buffer = 0;
	next_input_byte = m_buf;
}

void
JpegSourceManager::initSource(j_decompress_ptr cinfo)
{
	// No-op.
}

boolean
JpegSourceManager::fillInputBuffer(j_decompress_ptr cinfo)
{
	return object(cinfo)->fillInputBufferImpl();
}

boolean
JpegSourceManager::fillInputBufferImpl()
{
	qint64 const bytes_read = m_rDevice.read((char*)m_buf, sizeof(m_buf));
	if (bytes_read > 0) {
		bytes_in_buffer = bytes_read;
	} else {
		// Insert a fake EOI marker.
		m_buf[0] = 0xFF;
		m_buf[1] = JPEG_EOI;
		bytes_in_buffer = 2;
	}
	next_input_byte = m_buf;
	return 1;
}

void
JpegSourceManager::skipInputData(j_decompress_ptr cinfo, long num_bytes)
{
	object(cinfo)->skipInputDataImpl(num_bytes);
}

void
JpegSourceManager::skipInputDataImpl(long num_bytes)
{
	if (num_bytes <= 0) {
		return;
	}
	
	while (num_bytes > (long)bytes_in_buffer) {
		num_bytes -= (long)bytes_in_buffer;
		fillInputBufferImpl();
	}
	next_input_byte += num_bytes;
	bytes_in_buffer -= num_bytes;
}

void
JpegSourceManager::termSource(j_decompress_ptr cinfo)
{
	// No-op.
}

JpegSourceManager*
JpegSourceManager::object(j_decompress_ptr cinfo)
{
	return static_cast<JpegSourceManager*>(cinfo->src);
}


/*============================= JpegErrorManager ===========================*/

class JpegErrorManager : public jpeg_error_mgr
{
	DECLARE_NON_COPYABLE(JpegErrorManager)
public:
	JpegErrorManager();
	
	jmp_buf& jmpBuf() { return m_jmpBuf; }
private:
	static void errorExit(j_common_ptr cinfo);
	
	static JpegErrorManager* object(j_common_ptr cinfo);
	
	jmp_buf m_jmpBuf;
};

JpegErrorManager::JpegErrorManager()
{
	jpeg_std_error(this);
	error_exit = &JpegErrorManager::errorExit;
}

void
JpegErrorManager::errorExit(j_common_ptr cinfo)
{
	longjmp(object(cinfo)->jmpBuf(), 1);
}

JpegErrorManager*
JpegErrorManager::object(j_common_ptr cinfo)
{
	return static_cast<JpegErrorManager*>(cinfo->err);
}

} // anonymous namespace


/*============================= JpegMetadataLoader ==========================*/

void
JpegMetadataLoader::registerMyself()
{
	static bool registered = false;
	if (!registered) {
		ImageMetadataLoader::registerLoader(
			IntrusivePtr<ImageMetadataLoader>(new JpegMetadataLoader)
		);
		registered = true;
	}
}

ImageMetadataLoader::Status
JpegMetadataLoader::loadMetadata(
	QIODevice& io_device,
	VirtualFunction1<void, ImageMetadata const&>& out)
{
	if (!io_device.isReadable()) {
		return GENERIC_ERROR;
	}

	static unsigned char const jpeg_signature[] = { 0xff, 0xd8, 0xff };
	static int const sig_size = sizeof(jpeg_signature);

	unsigned char signature[sig_size];
	if (io_device.peek((char*)signature, sig_size) != sig_size) {
		return FORMAT_NOT_RECOGNIZED;
	}
	if (memcmp(jpeg_signature, signature, sig_size) != 0) {
		return FORMAT_NOT_RECOGNIZED;
	}
	
	JpegErrorManager err_mgr;
	if (setjmp(err_mgr.jmpBuf())) {
		// Returning from longjmp().
		return GENERIC_ERROR;
	}
	
	JpegSourceManager src_mgr(io_device);
	JpegDecompressHandle cinfo(&err_mgr, &src_mgr);
	
	int const header_status = jpeg_read_header(cinfo.ptr(), 0);
	if (header_status == JPEG_HEADER_TABLES_ONLY) {
		return NO_IMAGES;
	}
	
	// The other possible value is JPEG_SUSPENDED, but we never suspend it.
	assert(header_status == JPEG_HEADER_OK);
	
	if (!jpeg_start_decompress(cinfo.ptr())) {
		// libjpeg doesn't support all compression types.
		return GENERIC_ERROR;
	}
	
	QSize const size(cinfo->image_width, cinfo->image_height);
	Dpi dpi;
	if (cinfo->density_unit == 1) {
		// Dots per inch.
		dpi = Dpi(cinfo->X_density, cinfo->Y_density);
	} else if (cinfo->density_unit == 2) {
		// Dots per centimeter.
		dpi = Dpm(cinfo->X_density * 100, cinfo->Y_density * 100);
	}
	
	out(ImageMetadata(size, dpi));
	return LOADED;
}
