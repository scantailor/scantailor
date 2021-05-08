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

#include "JP2Reader.h"
#include "ImageMetadata.h"
#include <QIODevice>
#include <QImage>
#include <openjpeg.h>

bool JP2Reader::peekMagic(QIODevice& dev)
{
	char buf[12];
	const char magic4[] = "\x0d\x0a\x87\x0a";
	const char magic12[] = "\x00\x00\x00\x0c\x6a\x50\x20\x20\x0d\x0a\x87\x0a";

	qint64 seen = dev.peek(buf, 12);
	return	(seen >=  4 && memcmp(buf, magic4,   4) == 0) ||
		(seen == 12 && memcmp(buf, magic12, 12) == 0);
}

static OPJ_SIZE_T readFn(void *buf, OPJ_SIZE_T size, void *ctx)
{
	QIODevice* dev = (QIODevice *) ctx;
	qint64 n = dev->read((char *) buf, size);
	if (n < 0)
		/* error... */
		return (OPJ_SIZE_T)-1;
	if (n == 0)
		/* end of stream */
		return (OPJ_SIZE_T)-1;
	return n;
}

static OPJ_OFF_T skipFn(OPJ_OFF_T off, void *ctx)
{
	(void) ctx; (void) off;
	fprintf(stderr, "jp2: skip not supported\n");
	return 0;
}

static OPJ_BOOL seekFn(OPJ_OFF_T off, void *ctx)
{
	(void) ctx; (void) off;
	fprintf(stderr, "jp2: seek not supported\n");
	return false;
}

static void warner(const char *msg, void *ctx)
{
	(void) ctx;
	fprintf(stderr, "jp2: %s", msg);
}

static opj_image_t *jp2decode(QIODevice& dev, bool full)
{
	opj_dparameters_t jp2_param;
	opj_set_default_decoder_parameters(&jp2_param);

	opj_codec_t *jp2_codec = opj_create_decompress(OPJ_CODEC_JP2);
	opj_setup_decoder(jp2_codec, &jp2_param);

	opj_set_warning_handler(jp2_codec, warner, NULL);
	opj_set_error_handler(jp2_codec, warner, NULL);

	opj_stream_t *jp2_stream = opj_stream_create(OPJ_J2K_STREAM_CHUNK_SIZE, OPJ_TRUE);
	opj_stream_set_read_function(jp2_stream, readFn);
	opj_stream_set_skip_function(jp2_stream, skipFn);
	opj_stream_set_seek_function(jp2_stream, seekFn);

	opj_stream_set_user_data(jp2_stream, &dev, NULL);
	opj_stream_set_user_data_length(jp2_stream, dev.size());

	opj_image_t *jp2 = NULL;
	bool ok = opj_read_header(jp2_stream, jp2_codec, &jp2);

	if (ok && full)
		ok = opj_decode(jp2_codec, jp2_stream, jp2) &&
		     opj_end_decompress(jp2_codec, jp2_stream);

	opj_stream_destroy(jp2_stream);
	opj_destroy_codec(jp2_codec);

	if (!ok) {
		opj_image_destroy(jp2);
		return NULL;
	}

	return jp2;
}

bool JP2Reader::readMetadata(QIODevice& dev,
	VirtualFunction1<void, ImageMetadata const&>& out)
{
	opj_image_t *jp2 = jp2decode(dev, false);
	if (!jp2)
		return false;

	QSize size(jp2->x1 - jp2->x0,
		   jp2->y1 - jp2->y0);
	Dpi dpi;

	out(ImageMetadata(size, dpi));

	opj_image_destroy(jp2);
	return true;
}

QImage
JP2Reader::readImage(QIODevice& dev)
{
	opj_image_t *jp2 = jp2decode(dev, true);
	if (!jp2)
		return QImage();

	int w = jp2->comps[0].w;
	int h = jp2->comps[0].h;

	if (jp2->numcomps == 3) {
		QImage img(w, h, QImage::Format_RGB32);
		for (int y = 0; y < h; y++) {
			int *r = jp2->comps[0].data + y * w;
			int *g = jp2->comps[1].data + y * w;
			int *b = jp2->comps[2].data + y * w;
			QRgb *q = (QRgb *) img.scanLine(y);
			for (int x = 0; x < w; x++)
				q[x] = qRgb(r[x], g[x], b[x]);
		}
		opj_image_destroy(jp2);
		return img;
	}

	opj_image_destroy(jp2);
	return QImage();
}
