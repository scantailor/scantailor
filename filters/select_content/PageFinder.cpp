/*
    Scan Tailor - Interactive post-processing tool for scanned pages.
    Copyright (C)  Joseph Artsimovich <joseph.artsimovich@gmail.com>
    Copyright (C) 2012  Petr Kovar <pejuko@gmail.com>

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

#include "PageFinder.h"

#include "CommandLine.h"
#include "Dpi.h"
#include "DebugImages.h"
#include "FilterData.h"
#include "ImageTransformation.h"
#include "imageproc/BinaryImage.h"
#include "imageproc/Binarize.h"
#include "imageproc/Transform.h"
#include "imageproc/GrayRasterOp.h"
#include "imageproc/Grayscale.h"
#include "TaskStatus.h"

#include <Qt>
#include <QColor>
#include <QDebug>
#include <QtGlobal>
#include <QImage>
#include <QRect>
#include <QRectF>
#include <QTransform>

namespace select_content
{

using namespace imageproc;

QRectF
PageFinder::findPageBox(
	TaskStatus const& status, FilterData const& data, bool fine_tune, DebugImages* dbg)
{
	ImageTransformation xform_150dpi(data.xform());
	xform_150dpi.preScaleToDpi(Dpi(150, 150));

	if (xform_150dpi.resultingRect().toRect().isEmpty()) {
		return QRectF();
	}
	
	uint8_t const darkest_gray_level = darkestGrayLevel(data.grayImage());
	QColor const outside_color(darkest_gray_level, darkest_gray_level, darkest_gray_level);

	QImage gray150(
		transformToGray(
			data.grayImage(), xform_150dpi.transform(),
			xform_150dpi.resultingRect().toRect(),
			OutsidePixels::assumeColor(outside_color)
		)
	);
	// Note that we fill new areas that appear as a result of
	// rotation with black, not white.  Filling them with white
	// may be bad for detecting the shadow around the page.
	if (dbg) {
		dbg->add(gray150, "gray150");
	}

	BinaryImage bw150(peakThreshold(gray150));
	//BinaryImage bw150(binarizeOtsu(gray150));
	if (dbg) {
	    dbg->add(bw150, "peakThreshold");
	}

	QImage bwimg(bw150.toQImage());
	QRect content_rect(detectBorders(bwimg));
	if (fine_tune)
		fineTuneCorners(bwimg, content_rect);
	

	// Transform back from 150dpi.
	QTransform combined_xform(xform_150dpi.transform().inverted());
	combined_xform *= data.xform().transform();
	return combined_xform.map(QRectF(content_rect)).boundingRect();
}

QRect
PageFinder::detectBorders(QImage const& img)
{
	int l=0, t=0, r=img.width()-1, b=img.height()-1;
	int xmid = int(double(r) * 0.382);
	int ymid = int(double(b) * 0.382);

	l = detectEdge(img, l, r, 1, ymid, Qt::Horizontal);
	t = detectEdge(img, t, b, 1, xmid, Qt::Vertical);
	r = detectEdge(img, r, 0, -1, ymid, Qt::Horizontal);
	b = detectEdge(img, b, t, -1, xmid, Qt::Vertical);

	return QRect(l, t, r-l+1, b-t+1);
}

/**
 * shift edge while points around mid are black
 */
int
PageFinder::detectEdge(QImage const& img, int start, int end, int inc, int mid, Qt::Orientation orient)
{
	int min_size=10;
	int gap=0;
	int i=start, edge=start;
	int ms = mid - int(double(mid) / 4.0);
	int me = mid + int(double(mid) / 4.0);
	int min_bp = int(double(me-ms) * 0.8);
	Qt::GlobalColor black = Qt::color1;

	while (i != end) {
		int black_pixels=0;
	    int old_gap=gap;

		// count black pixels on the edge around given point
	    for (int j=ms; j!=me; j++) {
	        int x=i, y=j;
	        if (orient == Qt::Vertical) { x=j; y=i; }
	        int pixel = img.pixelIndex(x, y);
	        if (pixel == black)
				++black_pixels;
	    }

		if (black_pixels < min_bp) {
			++gap;
		} else {
			gap = 0;
			edge = i;
		}

	    if (gap > min_size)
	        break;

	    i += inc;
	}

	return edge;
}

void
PageFinder::fineTuneCorners(QImage const& img, QRect &rect)
{
	int l=rect.left(), t=rect.top(), r=rect.right(), b=rect.bottom();

	fineTuneCorner(img, l, t, 1, 1);
	fineTuneCorner(img, r, t, -1, 1);
	fineTuneCorner(img, l, b, 1, -1);
	fineTuneCorner(img, r, b, -1, -1);

	rect.setLeft(l);
	rect.setTop(t);
	rect.setRight(r);
	rect.setBottom(b);
}

/**
 * shift edges until given corner is out of black
 */
void
PageFinder::fineTuneCorner(QImage const& img, int &x, int &y, int inc_x, int inc_y)
{
	Qt::GlobalColor black = Qt::color1;
	while (1) {
	    int pixel = img.pixelIndex(x, y);
	    int tx = x + inc_x;
	    int ty = y + inc_y;
	    if (pixel!=black || tx<0 || tx>(img.width()-1) || ty<0 || ty>(img.height()-1))
	        break;
	    x = tx;
	    y = ty;
	}
}

} // namespace
