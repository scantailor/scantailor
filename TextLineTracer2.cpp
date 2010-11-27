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

#include "TextLineTracer2.h"
#include "Dpi.h"
#include "TaskStatus.h"
#include "DebugImages.h"
#include "NumericTraits.h"
#include "MatrixCalc.h"
#include "VecNT.h"
#include "ToLineProjector.h"
#include "imageproc/BinaryImage.h"
#include "imageproc/BinaryThreshold.h"
#include "imageproc/Grayscale.h"
#include "imageproc/GrayImage.h"
#include "imageproc/Scale.h"
#include "imageproc/Constants.h"
#include "imageproc/GaussBlur.h"
#include "imageproc/Morphology.h"
#include "imageproc/RasterOp.h"
#include "imageproc/GrayRasterOp.h"
#include "imageproc/RasterOpGeneric.h"
#include "imageproc/FindPeaksGeneric.h"
#include "imageproc/ConnectivityMap.h"
#include "imageproc/InfluenceMap.h"
#include <QImage>
#include <QPainter>
#include <QBrush>
#include <QPen>
#include <QColor>
#include <QtGlobal>
#include <boost/scoped_array.hpp>
#include <boost/foreach.hpp>
#include <boost/lambda/lambda.hpp>
#include <boost/lambda/bind.hpp>
#include <boost/lambda/if.hpp>
#include <algorithm>
#include <stdlib.h>
#include <math.h>

using namespace imageproc;

namespace
{

	uint8_t darkest(uint8_t lhs, uint8_t rhs) {
		return lhs < rhs ? lhs : rhs;
	}

	uint8_t lightest(uint8_t lhs, uint8_t rhs) {
		return lhs > rhs ? lhs : rhs;
	}

	uint8_t darker(uint8_t color) {
		return color == 0 ? 0 : color - 1;
	}

}

struct TextLineTracer2::Sample
{
	float y;
	int count;

	Sample() : y(0), count(0) {}
};

struct TextLineTracer2::ApproxLine
{
	enum Flag { LEFT_ENDPOINT = 1, RIGHT_ENDPOINT = 2 };

	int leftX;
	int rightX;
	float leftY;
	float rightY;
	int sampleStorageOffset;
	int flags;

	ApproxLine() : leftX(NumericTraits<int>::max()), rightX(NumericTraits<int>::min()),
		leftY(0), rightY(0), sampleStorageOffset(-1), flags(0) {}
};

struct TextLineTracer2::Comp
{
	enum LineIdx { LEFT_LINE = 0, RIGHT_LINE = 1 };

	ApproxLine approxLines[2];

	ApproxLine& leftApproxLine() { return approxLines[LEFT_LINE]; }

	ApproxLine const& leftApproxLine() const { return approxLines[LEFT_LINE]; }

	ApproxLine& rightApproxLine() { return approxLines[RIGHT_LINE]; }

	ApproxLine const& rightApproxLine() const { return approxLines[RIGHT_LINE]; }

	bool isValid() const { return approxLines[LEFT_LINE].rightX != -1; }
};

struct TextLineTracer2::Node
{
	Vec2f endpoint;
	Vec2f unitVecFromEndpoint;
	uint32_t comp:31; // Index into a vector of Comp's.
	uint32_t right:1; // left: 0, right: 1
	std::vector<uint32_t> connectedNodes; // Indexes of nodes connected to this one.

	Node(uint32_t comp, Comp::LineIdx idx, ApproxLine const& line)
		: comp(comp), right(idx == Comp::RIGHT_LINE) {
		
		if (right) {
			endpoint[0] = line.rightX;
			endpoint[1] = line.rightY;
			unitVecFromEndpoint = Vec2f(line.leftX, line.leftY) - endpoint;
		} else {
			endpoint[0] = line.leftX;
			endpoint[1] = line.leftY;
			unitVecFromEndpoint = Vec2f(line.rightX, line.rightY) - endpoint;
		}

		unitVecFromEndpoint /= sqrt(unitVecFromEndpoint.squaredNorm());
	}
};

void
TextLineTracer2::trace(
	GrayImage const& input, Dpi const& dpi,
	TaskStatus const& status, DebugImages* dbg)
{
	GrayImage downscaled(downscale(input, dpi));
	
#if 0
	GrayImage gradient(verticalSobel(downscaled));
	if (dbg) {
		dbg->add(gradient, "gradient");
	} else {
		downscaled = GrayImage(); // Save memory.
	}
#endif

	int const downscaled_width = downscaled.width();
	int const downscaled_height = downscaled.height();

	GrayImage blurred(gaussBlur(stretchGrayRange(downscaled), 17, 7));
	if (dbg) {
		dbg->add(blurred.toQImage(), "blurred");
	}

	ConnectivityMap cmap(findVerticalPeaks(blurred), CONN8);
	if (dbg) {
		dbg->add(cmap.visualized(), "ridges");
	}

	std::vector<Comp> comps(cmap.maxLabel() + 1);
	killShortComponents(cmap, comps);
	if (dbg) {
		dbg->add(overlay(blurred, cmap.visualized(Qt::transparent)), "short_comps_killed");
	}
	
	approximateRidgeEndings(cmap, comps);
	if (dbg) {
		dbg->add(visualizeApproxLines(blurred, comps), "approx_endings");
	}

	markLeftRightEndpoints(cmap, comps);
	if (dbg) {
		dbg->add(visualizeEndpoints(blurred, comps), "endpoint_segments");
	}

	std::vector<Node> nodes;
	nodes.reserve(comps.size() * 2);
	createNodes(nodes, comps);

	connectNodes(nodes, comps);
	if (dbg) {
		dbg->add(visualizeGraph(blurred, cmap.visualized(Qt::transparent), nodes), "graph");
	}

	//InfluenceMap imap(cmap);
	//cmap = ConnectivityMap();
	//if (dbg) {
	//	dbg->add(imap.visualized(), "imap");
	//}
}

GrayImage
TextLineTracer2::downscale(GrayImage const& input, Dpi const& dpi)
{
	// Downscale to 200 DPI.
	QSize downscaled_size(input.size());
	if (dpi.horizontal() < 180 || dpi.horizontal() > 220 || dpi.vertical() < 180 || dpi.vertical() > 220) {
		downscaled_size.setWidth(std::max<int>(1, input.width() * 200 / dpi.horizontal()));
		downscaled_size.setHeight(std::max<int>(1, input.height() * 200 / dpi.vertical()));
	}

	return scaleToGray(input, downscaled_size);
}

GrayImage
TextLineTracer2::verticalSobel(GrayImage const& src)
{
	int const width = src.width();
	int const height = src.height();

	int const src_stride = src.stride();
	int const tmp_stride = width + 2;

	// tmp will initially contain a copy of src with a reflective 1px border from each side.
	boost::scoped_array<int16_t> tmp(new int16_t[(width + 2) * (height + 2)]);

	uint8_t const* src_line = src.data();
	int16_t* tmp_line = tmp.get() + tmp_stride + 1;

	// Copy src to tmp.
	for (int y = 0; y < height; ++y) {
		for (int x = 0; x < width; ++x) {
			tmp_line[x] = src_line[x];
		}
		src_line += src_stride;
		tmp_line += tmp_stride;
	}

	// Write border corners.
	tmp_line = tmp.get();
	tmp_line[0] = tmp_line[tmp_stride + 1];
	tmp_line[tmp_stride - 1] = tmp_line[tmp_stride * 2 - 2];
	tmp_line += tmp_stride * (height + 1);
	tmp_line[0] = tmp_line[1 - tmp_stride];
	tmp_line[tmp_stride - 1] = tmp_line[-2];

	// Top border line.
	tmp_line = tmp.get() + 1;
	for (int x = 0; x < width; ++x) {
		tmp_line[0] = tmp_line[tmp_stride];
		++tmp_line;
	}

	// Bottom border line.
	tmp_line = tmp.get() + tmp_stride * (height + 1) + 1;
	for (int x = 0; x < width; ++x) {
		tmp_line[0] = tmp_line[-tmp_stride];
		++tmp_line;
	}

	// Left and right border lines.
	tmp_line = tmp.get() + tmp_stride;
	for (int y = 0; y < height; ++y) {
		tmp_line[0] = tmp_line[1];
		tmp_line[tmp_stride - 1] = tmp_line[tmp_stride - 2];
		tmp_line += tmp_stride;
	}

	// Do a horizontal pass.
	tmp_line = tmp.get() + 1;
	for (int y = 0; y < height + 2; ++y) {
		int16_t prev = tmp_line[-1];
		for (int x = 0; x < width; ++x) {
			uint16_t cur = tmp_line[x];
			tmp_line[x] = prev + (cur << 1) + tmp_line[x + 1];
			prev = cur;
		}
		tmp_line += tmp_stride;
	}

	GrayImage dst(src.size());
	int const dst_stride = dst.stride();
	uint8_t* dst_line = dst.data();

	// Do a vertical pass and write resuts.
	tmp_line = tmp.get() + tmp_stride + 1;
	for (int y = 0; y < height; ++y) {
		for (int x = 0; x < width; ++x) {
			int16_t const val = tmp_line[x + tmp_stride] - tmp_line[x - tmp_stride];
			dst_line[x] = static_cast<uint8_t>(qBound<int16_t>(0, val >> 2, 255));
		}
		tmp_line += tmp_stride;
		dst_line += dst_stride;
	}

	return dst;
}

BinaryImage
TextLineTracer2::findVerticalPeaks(GrayImage const& input)
{
	using namespace boost::lambda;

	GrayImage mask(input.size());
	rasterOpGeneric(
		mask.data(), mask.stride(), mask.size(), input.data(),
		input.stride(), _1 = bind(&darker, _2)
	);
	
	GrayImage seed(input);
	seedFillVerticalOnly(seed, mask);
	rasterOpGeneric(
		seed.data(), seed.stride(), seed.size(), input.data(),
		input.stride(), if_then_else(_1 == _2, _1 = constant(uint8_t(0)), _1 = constant(uint8_t(255)))
	);
	mask = GrayImage();

	BinaryImage peaks(seed);
	seed = GrayImage();

	GrayImage eroded(erodeGray(input, QSize(1, 23)));
	rasterOpGeneric(
		eroded.data(), eroded.stride(), eroded.size(), input.data(),
		input.stride(), if_then_else(_1 - _2 >= 12, _1 = 0, _1 = 255)
	);
	BinaryImage peak_mask(eroded);
	eroded = GrayImage();

	rasterOp<RopAnd<RopSrc, RopDst> >(peaks, peak_mask);

	return peaks;
}

void
TextLineTracer2::seedFillVerticalOnly(GrayImage& seed, GrayImage const& mask)
{
	int const width = seed.width();
	int const height = seed.height();
	
	uint8_t* p_seed = seed.data();
	uint8_t const* p_mask = mask.data();
	int const seed_stride = seed.stride();
	int const mask_stride = mask.stride();

	for (int x = 0; x < width; ++x, ++p_seed, ++p_mask) {
		uint8_t prev = 255;
		for (int y = 0; y < height; ++y) {
			*p_seed = prev = lightest(*p_mask, darkest(prev, *p_seed));
			p_seed += seed_stride;
			p_mask += mask_stride;
		}
		
		prev = 255;
		for (int y = height - 1; y >= 0; --y) {
			p_seed -= seed_stride;
			p_mask -= mask_stride;
			*p_seed = prev = lightest(*p_mask, darkest(prev, *p_seed));
		}
	}
}

void
TextLineTracer2::killShortComponents(imageproc::ConnectivityMap& cmap, std::vector<Comp>& comps)
{
	int const width = cmap.size().width();
	int const height = cmap.size().height();
	int const stride = cmap.stride();
	uint32_t* line = cmap.data();

	// Find the first and the last x coordinate for each component.
	// Note that leftX and rightX of Comp::leftApproxLine are already
	// initialized to INT_MAX and INT_MIN respectively.
	for (int y = 0; y < height; ++y, line += stride) {
		for (int x = 0; x < width; ++x) {
			Comp& comp = comps[line[x]];
			if (x < comp.leftApproxLine().leftX) {
				comp.leftApproxLine().leftX = x;
			}
			if (x > comp.leftApproxLine().rightX) {
				comp.leftApproxLine().rightX = x;
			}
		}
	}

	// Invalidate short components.
	int const threshold = 30;
	BOOST_FOREACH(Comp& comp, comps) {
		if (comp.leftApproxLine().rightX - comp.leftApproxLine().leftX < threshold) {
			comp.leftApproxLine().leftX = 0;
			comp.leftApproxLine().rightX = -1;
		}
		comp.rightApproxLine().leftX = comp.leftApproxLine().leftX;
		comp.rightApproxLine().rightX = comp.leftApproxLine().rightX;
	}
	// Invalidate the background component as well.
	comps[0].leftApproxLine().leftX = 0;
	comps[0].leftApproxLine().rightX = -1;
	comps[0].rightApproxLine().leftX = 0;
	comps[0].rightApproxLine().rightX = -1;

	// Remove short components from the map.
	line = cmap.data();
	for (int y = 0; y < height; ++y, line += stride) {
		for (int x = 0; x < width; ++x) {
			Comp& comp = comps[line[x]];
			if (!comp.isValid()) {
				line[x] = 0;
			}
		}
	}
}

void
TextLineTracer2::approximateRidgeEndings(
	imageproc::ConnectivityMap const& cmap, std::vector<Comp>& comps)
{
	// First let's calculate the total number of samples required
	// and assign offset into the sample storage to comps.
	// Note that leftX and rightX members of ApproxLine are aleady initialized.
	int total_samples = 0;
	BOOST_FOREACH(Comp& comp, comps) {
		if (!comp.isValid()) {
			continue;
		}

		int const hor_span = comp.leftApproxLine().rightX - comp.leftApproxLine().leftX + 1;

		comp.leftApproxLine().sampleStorageOffset = total_samples;
		total_samples += hor_span;

		int const long_hor_span = 50;
		if (hor_span < long_hor_span) {
			// Shared samples for left and right side.
			comp.rightApproxLine().sampleStorageOffset = comp.leftApproxLine().sampleStorageOffset;
		} else {
			// Separate samples for left and right side.
			comp.rightApproxLine().sampleStorageOffset = total_samples;
			total_samples += hor_span;
			comp.leftApproxLine().rightX = comp.leftApproxLine().leftX + (long_hor_span - 1);
			comp.rightApproxLine().leftX = comp.rightApproxLine().rightX - (long_hor_span - 1); 
		}
	}

	std::vector<Sample> samples(total_samples);

	int const width = cmap.size().width();
	int const height = cmap.size().height();
	int const stride = cmap.stride();
	uint32_t const* line = cmap.data();

	for (int y = 0; y < height; ++y, line += stride) {
		for (int x = 0; x < width; ++x) {
			Comp& comp = comps[line[x]];
			if (!comp.isValid()) {
				continue;
			}

			if (/*x >= comp.leftApproxLine().leftX &&*/ x <= comp.leftApproxLine().rightX) {
				int const offset = comp.leftApproxLine().sampleStorageOffset;
				Sample& sample = samples[offset + x - comp.leftApproxLine().leftX];
				sample.y += y;
				sample.count += 1;
			}

			if (x >= comp.rightApproxLine().leftX /*&& x <= comp.rightApproxLine().rightX*/) {
				// If our samples are shared, it's still fine to do this.
				int const offset = comp.rightApproxLine().sampleStorageOffset;
				Sample& sample = samples[offset + x - comp.rightApproxLine().leftX];
				sample.y += y;
				sample.count += 1;
			}
		}
	}

	BOOST_FOREACH(Comp& comp, comps) {
		if (!comp.isValid()) {
			continue;
		}

		approxLine(comp.leftApproxLine(), samples);
		if (comp.rightApproxLine().sampleStorageOffset == comp.leftApproxLine().sampleStorageOffset) {
			comp.rightApproxLine() = comp.leftApproxLine();
		} else {
			approxLine(comp.rightApproxLine(), samples);
		}
	}
}

void
TextLineTracer2::approxLine(ApproxLine& approx_line, std::vector<Sample> const& samples)
{
	int const num_samples = approx_line.rightX - approx_line.leftX + 1;
	boost::scoped_array<double> At(new double[num_samples * 2]);
	boost::scoped_array<double> B(new double[num_samples]);

	int const offset = approx_line.sampleStorageOffset - approx_line.leftX;
	double* pAt = At.get();
	double* pB = B.get();
	for (int x = approx_line.leftX; x <= approx_line.rightX; ++x) {
		Sample const& sample = samples[offset + x];
		pAt[0] = x;
		pAt[1] = 1;
		pAt += 2;
		*pB = sample.y / double(sample.count);
		++pB;
	}

	double x[2];

	try {
		DynamicMatrixCalc<double> mc;
		((mc(At.get(), 2, num_samples)*mc(At.get(), 2, num_samples).trans())
			.inv()*mc(At.get(), 2, num_samples)*mc(B.get(), num_samples, 1)).write(x);

		approx_line.leftY = x[0] * approx_line.leftX + x[1];
		approx_line.rightY = x[0] * approx_line.rightX + x[1];
	} catch (...) {
		approx_line.leftY = 0;
		approx_line.rightY = 0; // XXX
	}
}

void
TextLineTracer2::markLeftRightEndpoints(
	imageproc::ConnectivityMap const& cmap, std::vector<Comp>& comps)
{
	InfluenceMap imap(cmap);

	int const height = imap.size().height();
	int const rightmost_x = imap.size().width() - 1;
	InfluenceMap::Cell const* line = imap.data();
	int const stride = imap.stride();

	for (int y = 0; y < height; ++y, line += stride) {
		comps[line[0].label].leftApproxLine().flags |= ApproxLine::LEFT_ENDPOINT;
		comps[line[rightmost_x].label].rightApproxLine().flags |= ApproxLine::RIGHT_ENDPOINT;
	}
}

void
TextLineTracer2::createNodes(std::vector<Node>& nodes, std::vector<Comp> const& comps)
{
	uint32_t const num_comps = comps.size();
	for (uint32_t label = 1; label < num_comps; ++label) {
		Comp const& comp = comps[label];
		if (!comp.isValid()) {
			continue;
		}

		nodes.push_back(Node(label, Comp::LEFT_LINE, comp.leftApproxLine()));
		nodes.push_back(Node(label, Comp::RIGHT_LINE, comp.rightApproxLine()));
	}
}

void
TextLineTracer2::connectNodes(std::vector<Node>& nodes, std::vector<Comp> const& comps)
{
	// Warning: quadratic complexity with regards to the number of nodes.
	// Use kd-trees if this becomes a performance bottleneck.

	uint32_t const num_nodes = nodes.size();

	for (uint32_t node1_idx = 0; node1_idx < num_nodes; ++node1_idx) {
		Node& node1 = nodes[node1_idx];

		for (uint32_t node2_idx = node1_idx + 1; node2_idx < num_nodes; ++node2_idx) {
			Node& node2 = nodes[node2_idx];

			if (node1.right == node2.right) {
				// Don't connect two left sides or two right sides together.
				continue;
			}

			if (node1.comp == node2.comp) {
				// Connect nodes that are part of the same ridge.
				node1.connectedNodes.push_back(node2_idx);
				node2.connectedNodes.push_back(node1_idx);
				continue;
			}

			if ((node1.endpoint - node2.endpoint).squaredNorm() > 100*100) {
				// Endpoints are too far apart.
				continue;
			}

			if (node1.unitVecFromEndpoint.dot(node2.unitVecFromEndpoint) > -0.95) {
				// Angle is too high.
				continue;
			}

			Vec2f const normal1(node1.unitVecFromEndpoint[1], -node1.unitVecFromEndpoint[0]);
			Vec2f const normal2(node2.unitVecFromEndpoint[1], -node2.unitVecFromEndpoint[0]);
			Vec2f const vec(node2.endpoint - node1.endpoint);
			if (fabs(vec.dot(normal1)) > 10.0 || fabs(vec.dot(normal2)) > 10.0) {
				// One endpoint shouldn't be too far from the other line (not line segment).
				continue;
			}

			node1.connectedNodes.push_back(node2_idx);
			node2.connectedNodes.push_back(node1_idx);
		}
	}
}

QImage
TextLineTracer2::overlay(QImage const& background, QImage const& overlay)
{
	QImage canvas(background.convertToFormat(QImage::Format_ARGB32_Premultiplied));
	QPainter painter(&canvas);
	painter.drawImage(canvas.rect(), overlay);
	return canvas;
}

QImage
TextLineTracer2::colorizedOverlay(QImage const& background, BinaryImage const& overlay, QColor const& color)
{
	QImage canvas(background.convertToFormat(QImage::Format_ARGB32_Premultiplied));
	QImage fg(overlay.size(), QImage::Format_ARGB32);
	fg.fill(color.rgba());
	fg.setAlphaChannel(overlay.inverted().toQImage());
	QPainter painter(&canvas);
	painter.drawImage(fg.rect(), fg);
	return canvas;
}

QImage
TextLineTracer2::visualizeApproxLines(QImage const& background, std::vector<Comp> const& comps)
{
	QImage canvas(background.convertToFormat(QImage::Format_ARGB32_Premultiplied));
	QPainter painter(&canvas);
	painter.setRenderHint(QPainter::Antialiasing);
	QPen pen(Qt::blue);
	pen.setWidthF(2.5);
	painter.setPen(pen);

	BOOST_FOREACH(Comp const& comp, comps) {
		if (!comp.isValid()) {
			continue;
		}
		
		ApproxLine const& left_line = comp.leftApproxLine();
		ApproxLine const& right_line = comp.rightApproxLine();

		painter.drawLine(left_line.leftX, left_line.leftY, left_line.rightX, left_line.rightY);
		if (left_line.sampleStorageOffset != right_line.sampleStorageOffset) {
			painter.drawLine(right_line.leftX, right_line.leftY, right_line.rightX, right_line.rightY);
		}
	}
	
	return canvas;
}

QImage
TextLineTracer2::visualizeEndpoints(QImage const& background, std::vector<Comp> const& comps)
{
	QImage canvas(background.convertToFormat(QImage::Format_ARGB32_Premultiplied));
	QPainter painter(&canvas);
	painter.setRenderHint(QPainter::Antialiasing);
	QPen pen(Qt::blue);
	pen.setWidthF(2.5);
	painter.setPen(pen);

	BOOST_FOREACH(Comp const& comp, comps) {
		if (!comp.isValid()) {
			continue;
		}

		ApproxLine const& left_line = comp.leftApproxLine();
		ApproxLine const& right_line = comp.rightApproxLine();
		
		if (left_line.flags & ApproxLine::LEFT_ENDPOINT) {
			painter.drawLine(left_line.leftX, left_line.leftY, left_line.rightX, left_line.rightY);
		}
		if (right_line.flags & ApproxLine::RIGHT_ENDPOINT) {
			painter.drawLine(right_line.leftX, right_line.leftY, right_line.rightX, right_line.rightY);
		}
	}
	
	return canvas;
}

QImage
TextLineTracer2::visualizeGraph(
	QImage const& background, QImage const& overlay, std::vector<Node> const& nodes)
{
	QImage canvas(background.convertToFormat(QImage::Format_ARGB32_Premultiplied));
	QPainter painter(&canvas);
	painter.drawImage(QPointF(0, 0), overlay);
	painter.setRenderHint(QPainter::Antialiasing);
	QPen pen(Qt::black);
	pen.setWidthF(1.0);
	painter.setPen(pen);

	uint32_t const num_nodes = nodes.size();
	for (uint32_t node1_idx = 0; node1_idx < num_nodes; ++node1_idx) {
		Node const& node1 = nodes[node1_idx];
		BOOST_FOREACH(uint32_t node2_idx, node1.connectedNodes) {
			Node const& node2 = nodes[node2_idx];
			if (node1.comp == node2.comp) {
				continue; // Already connected by a ridge.  No need to draw another line.
			}
			if (node2_idx > node1_idx) { // To prevent drawing the same line twice.
				painter.drawLine(node1.endpoint, node2.endpoint);
			}
		}
	}
	
	return canvas;
}
