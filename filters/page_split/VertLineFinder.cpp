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

#include "VertLineFinder.h"
#include "ImageTransformation.h"
#include "Dpi.h"
#include "DebugImages.h"
#include "imageproc/Transform.h"
#include "imageproc/Grayscale.h"
#include "imageproc/GrayRasterOp.h"
#include "imageproc/Morphology.h"
#include "imageproc/MorphGradientDetect.h"
#include "imageproc/HoughLineDetector.h"
#include <boost/foreach.hpp>
#include <QLineF>
#include <QSizeF>
#include <QColor>
#include <QImage>
#include <QPainter>
#include <QPen>
#include <QTransform>
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
	int const max_lines, DebugImages* dbg, QImage* hor_shadows)
{
	ImageTransformation xform_100dpi(xform);
	xform_100dpi.preScaleToDpi(Dpi(100, 100));
	
	QColor const black(0x00, 0x00, 0x00);
	QImage const gray100(
		transformToGray(
			image, xform_100dpi.transform(),
			xform_100dpi.resultingRect().toRect(), black,
			QSizeF(5.0, 5.0)
		)
	);
	
	if (hor_shadows) {
		*hor_shadows = detectHorShadows(gray100);
	}
	
	QImage preprocessed(removeDarkVertBorders(gray100));
	if (dbg) {
		dbg->add(preprocessed, "preprocessed");
	}
	
	QImage h_gradient(morphGradientDetectDarkSide(preprocessed, QSize(11, 1)));
	QImage v_gradient(morphGradientDetectDarkSide(preprocessed, QSize(1, 11)));
	if (dbg) {
		dbg->add(h_gradient, "h_gradient");
		dbg->add(v_gradient, "v_gradient");
	} else {
		// We'll need it later if debugging is on.
		preprocessed = QImage();
	}
	
	grayRasterOp<GRopClippedSubtract<GRopDst, GRopSrc> >(h_gradient, v_gradient);
	v_gradient = QImage();
	if (dbg) {
		dbg->add(h_gradient, "vert_raster_lines");
	}
	
	QImage const raster_lines(closeGray(h_gradient, QSize(1, 19), 0x00));
	h_gradient = QImage();
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
	
	int const width = raster_lines.width();
	int const height = raster_lines.height();
	uint8_t const* line = raster_lines.bits();
	int const bpl = raster_lines.bytesPerLine();
	for (int y = 0; y < height; ++y, line += bpl) {
		for (int x = 0; x < width; ++x) {
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
			preprocessed.convertToFormat(
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

QImage
VertLineFinder::detectHorShadows(QImage const& src)
{
	QImage long_hor_lines(openGray(src, QSize(100, 1), 0xff));
	long_hor_lines = removeDarkHorBorders(long_hor_lines);
	return openGray(long_hor_lines, QSize(100, 1), 0xff);
}

QImage
VertLineFinder::removeDarkVertBorders(QImage const& src)
{
	QImage dst(src);
	
	selectVertBorders(dst);
	grayRasterOp<GRopInvert<GRopClippedSubtract<GRopDst, GRopSrc> > >(dst, src);
	
	return dst;
}

QImage
VertLineFinder::removeDarkHorBorders(QImage const& src)
{
	QImage dst(src);
	
	selectHorBorders(dst);
	grayRasterOp<GRopInvert<GRopClippedSubtract<GRopDst, GRopSrc> > >(dst, src);
	
	return dst;
}

void
VertLineFinder::selectVertBorders(QImage& image)
{
	int const w = image.width();
	int const h = image.height();
	
	unsigned char* image_line = image.bits();
	int const image_bpl = image.bytesPerLine();
	
	std::vector<unsigned char> tmp_line(w, 0x00);
	
	for (int y = 0; y < h; ++y, image_line += image_bpl) {
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
VertLineFinder::selectHorBorders(QImage& image)
{
	int const w = image.width();
	int const h = image.height();
	
	unsigned char* const image_data = image.bits();
	int const image_bpl = image.bytesPerLine();
	
	std::vector<unsigned char> tmp_line(h, 0x00);
	
	for (int x = 0; x < w; ++x) {
		// Left to right.
		unsigned char* p_image = image_data + x;
		unsigned char prev_pixel = 0x00; // Black vertical border.
		for (int y = 0; y < h; ++y, p_image += image_bpl) {
			prev_pixel = std::max(*p_image, prev_pixel);
			tmp_line[y] = prev_pixel;
		}
		
		// Right to left
		p_image = image_data + x + (h - 1) * image_bpl;
		prev_pixel = 0x00; // Black vertical border.
		for (int y = h - 1; y >= 0; --y, p_image -= image_bpl) {
			prev_pixel = std::max(
				*p_image,
				std::min(prev_pixel, tmp_line[y])
			);
			*p_image = prev_pixel;
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
	m_left = std::min(m_left, line.left().x());
	m_right = std::max(m_right, line.right().x());
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
