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

#include "VertLineFinder.h"
#include "ImageTransformation.h"
#include "Dpi.h"
#include "DebugImages.h"
#include "imageproc/Transform.h"
#include "imageproc/GrayImage.h"
#include "imageproc/Grayscale.h"
#include "imageproc/GrayRasterOp.h"
#include "imageproc/Morphology.h"
#include "imageproc/MorphGradientDetect.h"
#include "imageproc/HoughLineDetector.h"
#include "imageproc/Constants.h"
#ifndef Q_MOC_RUN
#include <boost/foreach.hpp>
#endif
#include <QLineF>
#include <QSizeF>
#include <QColor>
#include <QImage>
#include <QPainter>
#include <QPen>
#include <QTransform>
#include <Qt>
#include <QDebug>
#include <list>
#include <algorithm>
#include <math.h>

namespace page_split
{

using namespace imageproc;

std::vector<QLineF>
VertLineFinder::findLines(
	QImage const& image, ImageTransformation const& xform,
	int const max_lines, DebugImages* dbg,
	GrayImage* gray_downscaled, QTransform* out_to_downscaled)
{
	int const dpi = 100;

	ImageTransformation xform_100dpi(xform);
	xform_100dpi.preScaleToDpi(Dpi(dpi, dpi));
	
	QRect target_rect(xform_100dpi.resultingRect().toRect());
	if (target_rect.isEmpty()) {
		target_rect.setWidth(1);
		target_rect.setHeight(1);
	}

	GrayImage const gray100(
		transformToGray(
			image, xform_100dpi.transform(), target_rect,
			OutsidePixels::assumeWeakColor(Qt::black), QSizeF(5.0, 5.0)
		)
	);
	if (dbg) {
		dbg->add(gray100, "gray100");
	}
	
	if (gray_downscaled) {
		*gray_downscaled = gray100;
	}
	if (out_to_downscaled) {
		*out_to_downscaled = xform.transformBack()
				* xform_100dpi.transform();
	}
	
#if 0
	GrayImage preprocessed(removeDarkVertBorders(gray100));
	if (dbg) {
		dbg->add(preprocessed, "preprocessed");
	}
#else
	// It looks like preprocessing causes more problems than it solves.
	// It can reduce the visibility of a folding line to a level where
	// it can't be detected, while it can't always fulfill its purpose of
	// removing vertical edges of a book.  Because of that, other methods
	// of dealing with them were developed, which makes preprocessing
	// obsolete.
	GrayImage preprocessed(gray100);
#endif

#if 0
	GrayImage h_gradient(morphGradientDetectDarkSide(preprocessed, QSize(11, 1)));
	GrayImage v_gradient(morphGradientDetectDarkSide(preprocessed, QSize(1, 11)));
	if (dbg) {
		dbg->add(h_gradient, "h_gradient");
		dbg->add(v_gradient, "v_gradient");
	}
#else
	// These are not gradients, but their difference is the same as for
	// the two gradients above.  This branch is an optimization.
	GrayImage h_gradient(erodeGray(preprocessed, QSize(11, 1), 0x00));
	GrayImage v_gradient(erodeGray(preprocessed, QSize(1, 11), 0x00));
#endif

	if (!dbg) {
		// We'll need it later if debugging is on.
		preprocessed = GrayImage();
	}
	
	grayRasterOp<GRopClippedSubtract<GRopDst, GRopSrc> >(h_gradient, v_gradient);
	v_gradient = GrayImage();
	if (dbg) {
		dbg->add(h_gradient, "vert_raster_lines");
	}

	GrayImage const raster_lines(closeGray(h_gradient, QSize(1, 19), 0x00));
	h_gradient = GrayImage();
	if (dbg) {
		dbg->add(raster_lines, "short_segments_removed");
	}
	
	double const line_thickness = 5.0;
	double const max_angle = 7.0; // degrees
	double const angle_step = 0.25;
	int const angle_steps_to_max = (int)(max_angle / angle_step);
	int const total_angle_steps = angle_steps_to_max * 2 + 1;
	double const min_angle = -angle_steps_to_max * angle_step;
	HoughLineDetector line_detector(
		raster_lines.size(), line_thickness,
		min_angle, angle_step, total_angle_steps
	);
	
	unsigned weight_table[256];
	buildWeightTable(weight_table);

	// We don't want to process areas too close to the vertical edges.
	double const margin_mm = 3.5;
	int const margin = (int)floor(0.5 + margin_mm * constants::MM2INCH * dpi);

	int const x_limit = raster_lines.width() - margin;
	int const height = raster_lines.height();
	uint8_t const* line = raster_lines.data();
	int const stride = raster_lines.stride();
	for (int y = 0; y < height; ++y, line += stride) {
		for (int x = margin; x < x_limit; ++x) {
			unsigned const val = line[x];
			if (val > 1) {
				line_detector.process(x, y, weight_table[val]);
			}
		}
	}
	
	unsigned const min_quality = (unsigned)(height * line_thickness * 1.8) + 1;
	
	if (dbg) {
		dbg->add(line_detector.visualizeHoughSpace(min_quality), "hough_space");
	}
	
	std::vector<HoughLine> const hough_lines(line_detector.findLines(min_quality));
	
	typedef std::list<LineGroup> LineGroups;
	LineGroups line_groups;
	BOOST_FOREACH (HoughLine const& hough_line, hough_lines) {
		QualityLine const new_line(
			hough_line.pointAtY(0.0),
			hough_line.pointAtY(height),
			hough_line.quality()
		);
		LineGroup* home_group = 0;
		
		LineGroups::iterator it(line_groups.begin());
		LineGroups::iterator const end(line_groups.end());
		while (it != end) {
			LineGroup& group = *it;
			if (group.belongsHere(new_line)) {
				if (home_group) {
					home_group->merge(group);
					line_groups.erase(it++);
					continue;
				} else {
					group.add(new_line);
					home_group = &group;
				}
			}
			++it;
		}
		
		if (!home_group) {
			line_groups.push_back(LineGroup(new_line));
		}
	}
	
	std::vector<QLineF> lines;
	BOOST_FOREACH (LineGroup const& group, line_groups) {
		lines.push_back(group.leader().toQLine());
		if ((int)lines.size() == max_lines) {
			break;
		}
	}
	
	if (dbg) {
		QImage visual(
			preprocessed.toQImage().convertToFormat(
				QImage::Format_ARGB32_Premultiplied
			)
		);
	
		{
			QPainter painter(&visual);
			painter.setRenderHint(QPainter::Antialiasing);
			QPen pen(QColor(0xff, 0x00, 0x00, 0x80));
			pen.setWidthF(3.0);
			painter.setPen(pen);
		
			BOOST_FOREACH (QLineF const& line, lines) {
				painter.drawLine(line);
			}
		}
		dbg->add(visual, "vector_lines");
	}
	
	// Transform lines back into original coordinates.
	QTransform const undo_100dpi(
		xform_100dpi.transformBack() * xform.transform()
	);
	BOOST_FOREACH (QLineF& line, lines) {
		line = undo_100dpi.map(line);
	}
	
	return lines;
}

GrayImage
VertLineFinder::removeDarkVertBorders(GrayImage const& src)
{
	GrayImage dst(src);
	
	selectVertBorders(dst);
	grayRasterOp<GRopInvert<GRopClippedSubtract<GRopDst, GRopSrc> > >(dst, src);
	
	return dst;
}

void
VertLineFinder::selectVertBorders(GrayImage& image)
{
	int const w = image.width();
	int const h = image.height();
	
	unsigned char* image_line = image.data();
	int const image_stride = image.stride();
	
	std::vector<unsigned char> tmp_line(w, 0x00);
	
	for (int y = 0; y < h; ++y, image_line += image_stride) {
		// Left to right.
		unsigned char prev_pixel = 0x00; // Black vertical border.
		for (int x = 0; x < w; ++x) {
			prev_pixel = std::max(image_line[x], prev_pixel);
			tmp_line[x] = prev_pixel;
		}
		
		// Right to left
		prev_pixel = 0x00; // Black vertical border.
		for (int x = w - 1; x >= 0; --x) {
			prev_pixel = std::max(
				image_line[x],
				std::min(prev_pixel, tmp_line[x])
			);
			image_line[x] = prev_pixel;
		}
	}
}

void
VertLineFinder::buildWeightTable(unsigned weight_table[])
{
	int gray_level = 0;
	unsigned weight = 2;
	int segment = 2;
	int prev_segment = 1;
	
	while (gray_level < 256) {
		int const limit = std::min(256, gray_level + segment);
		for (; gray_level < limit; ++gray_level) {
			weight_table[gray_level] = weight;
		}
		++weight;
		segment += prev_segment;
		prev_segment = segment;
	}
}


/*======================= VertLineFinder::QualityLine =======================*/

VertLineFinder::QualityLine::QualityLine(
	QPointF const& top, QPointF const& bottom, unsigned const quality)
:	m_quality(quality)
{
	if (top.x() < bottom.x()) {
		m_left = top;
		m_right = bottom;
	} else {
		m_left = bottom;
		m_right = top;
	}
}

QLineF
VertLineFinder::QualityLine::toQLine() const
{
	return QLineF(m_left, m_right);
}


/*======================= VertLineFinder::LineGroup ========================*/

VertLineFinder::LineGroup::LineGroup(QualityLine const& line)
:	m_leader(line),
	m_left(line.left().x()),
	m_right(line.right().x())
{
}

bool
VertLineFinder::LineGroup::belongsHere(QualityLine const& line) const
{
	if (m_left > line.right().x()) {
		return false;
	} else if (m_right < line.left().x()) {
		return false;
	} else {
		return true;
	}
}

void
VertLineFinder::LineGroup::add(QualityLine const& line)
{
	m_left = std::min(qreal(m_left), line.left().x());
	m_right = std::max(qreal(m_right), line.right().x());
	if (line.quality() > m_leader.quality()) {
		m_leader = line;
	}
}

void
VertLineFinder::LineGroup::merge(LineGroup const& other)
{
	m_left = std::min(m_left, other.m_left);
	m_right = std::max(m_right, other.m_right);
	if (other.leader().quality() > m_leader.quality()) {
		m_leader = other.leader();
	}
}

} // namespace page_split
