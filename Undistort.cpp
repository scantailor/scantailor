/*
	Scan Tailor - Interactive post-processing tool for scanned pages.
	Copyright (C) Robert Baruch <autophile@gmail.com>
	Copyright (C) Joseph Artsimovich <joseph.artsimovich@gmail.com>

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

#include "Undistort.h"
#include "TaskStatus.h"
#include "DebugImages.h"
#include "imageproc/BinaryImage.h"
#include "imageproc/ConnectivityMap.h"
#include "imageproc/Connectivity.h"
#include "imageproc/LeastSquaresFit.h"
#include "imageproc/LU.h"
#include "imageproc/LM.h"
#include "imageproc/Constants.h"
#include <QPoint>
#include <QSize>
#include <QRect>
#include <QPointF>
#include <QLineF>
#include <QImage>
#include <QPainter>
#include <QPen>
#include <QColor>
#include <QDebug>
#include <QtGlobal>
#include <boost/foreach.hpp>
#include <vector>
#include <algorithm>
#include <numeric>
#include <utility>
#include <memory>
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <assert.h>
#include <math.h>

using namespace imageproc;

namespace
{

class PowerFunc : public LMfunc
{
private:
	int coeffs;
	int order;
	int w;
	int h;
public:
	PowerFunc(int order, int w, int h) : coeffs(4), order(4), w(w), h(h) {}

	virtual int numParams() const { return coeffs; }

	virtual double val(double const* x, double const* a) const {
		return a[0] + a[1] * x[0] + a[2] * pow(1.1 - x[0], a[3]);
	}

	virtual double grad(double const* x, double const* a, int ak) const {
		switch (ak)
		{
			case 0:
				return 1.0;
			case 1:
				return x[0];
			case 2:
				return pow(1.1 - x[0], a[3]);
			case 3:
				return a[2] * pow(1.1-x[0], a[3]) * log(1.1 - x[0]);
			default:
				return a[2] * pow(0.1+x[0], a[4]) * log(0.1 + x[0]);
		}
	}

	void initialGuess(double* a) const {
		a[0] = 10;
		a[1] = 1;
		a[2] = 1;
		a[3] = 2;
	}
};

class PowerFunc2 : public LMfunc
{
private:
	int coeffs;
	int order;
	int w;
	int h;
public:
	PowerFunc2(int order, int w, int h) : coeffs(6), order(order), w(w), h(h) {}

	virtual int numParams() const { return coeffs; }

	virtual double val(double const* x, double const* a) const {
		return a[0] + a[1] * x[0] + a[2] * (pow(1.1 - x[0], a[3])) +
				a[4] * sin(2.0 * constants::PI * x[0] + a[5]);
	}

	virtual double grad(double const* x, double const* a, int ak) const {
		switch (ak) {
			case 0:
				return 1.0;
			case 1:
				return x[0];
			case 2:
				return pow(1.1 - x[0], a[3]);
			case 3:
				return a[2] * pow(1.1 - x[0], a[3]) * log(1.1 - x[0]);
			case 4:
				return sin(2.0 * constants::PI * x[0] + a[5]);
			default:
				return a[4] * cos(2.0 * constants::PI * x[0] + a[5]);
		}
	}

	void initialGuess(double* a) const {
		a[0] = 10;
		a[1] = 1;
		a[2] = 1;
		a[3] = 2;
		a[4] = 1;
		a[5] = 0;
	}
};

class Line
{
public:
	std::vector<QPoint> points;
	std::vector<double> d_approximator;
	double e;
	QPoint left;
	QPoint right;

	Line() : e(0), left(-1, -1), right(-1, -1)  {}

	bool isNull() const { return points.empty(); }

	int numPixels() const { return points.size(); }

	void addPoint(int x, int y) {
		QPoint point(x, y);
		points.push_back(point);
		if (left == QPoint(-1, -1) || x < left.x()) {
			left = point;
		}
		if (right == QPoint(-1, -1) || x > right.x()) {
			right = point;
		}
	}

	double d_approximatedY(double x, int leftMargin, int rightMargin, LMfunc* f) const {
		double const xpt = ((double)(x - leftMargin)) / (rightMargin - leftMargin);
		return qRound(f->val(&xpt, &d_approximator[0])
					  * d_approximator[d_approximator.size() - 2]
					  + d_approximator[d_approximator.size() - 1]);
		}
};

struct LineLessLeftY
{
	bool operator()(Line const& lhs, Line const& rhs) {
		return lhs.left.y() < rhs.left.y();
	}
};

struct LineLessFirstY
{
	bool operator()(Line const& lhs, Line const& rhs) {
		return lhs.points.front().y() < rhs.points.front().y();
	}
};

Line findLine2(BinaryImage& pixs)
{
	int const w = pixs.width();
	int const h = pixs.height();
	uint32_t* const data = pixs.data();
	int const stride = pixs.wordsPerLine();
	uint32_t const msb = uint32_t(1) << 31;

	int x, y;
	bool found = false;

	// Start from left edge

	for (x = 0; x < w; ++x) {
		uint32_t* p_data = data + (x >> 5);
		for (y = 0; y < h; ++y, p_data += stride) {
			if (*p_data & (msb >> (x & 31))) {
				*p_data &= ~(msb >> (x & 31)); // Clear the pixel.
				found = true;
				break;
			}
		}
		if (found) {
			break;
		}
	}

	if (!found) {
		return Line();
	}

	//qDebug() << "Starting line following at " << x << "," << y;

	Line line;
	line.addPoint(x, y);


	int const skipBound = w / 20;
	int const searchBound = 30;
	std::vector<double> totalY(30, y);

	do {
		found = false;

		double const sum = std::accumulate(totalY.begin(), totalY.end(), 0.0);
		y = qRound(sum / totalY.size());

		int nx, ny;
		int const nx_end = std::min(x + skipBound, w);
		for (nx = x + 1; nx < nx_end; ++nx) {
			int const ny_begin = std::max(y - searchBound, 0);
			int const ny_end = std::min(y + searchBound + 1, h);
			uint32_t* p_data = data + ny_begin * stride + (nx >> 5);
			for (ny = ny_begin; ny < ny_end; ++ny, p_data += stride) {
				if (*p_data & (msb >> (nx & 31))) {
					*p_data &= ~(msb >> (nx & 31)); // Clear the pixel.
					found = true;
					break;
				}
			}
			if (found) {
				break;
			}
		}

		if (found) {
			x = nx;
			y = ny;

			line.addPoint(x, y);

			memmove(&totalY[0], &totalY[0]+1, (totalY.size() - 1) * sizeof(totalY[0]));
			totalY.back() = y;
		}
	} while (found);

	return line;
}

/**
 * Perform a polynomial approximation of order k to the given dataset. The returned
 * array gives the coefficients of the k-order polynomial in x.
 *
 * To compute the estimated y, let the returned array be a[]. Then
 *
 * y = a[0] + a[1] * x^1 + a[2] * x^2 + ... + a[k] * x^k
 *
 * \param k the order of the approximator polynomial
 * \param x the dataset x coordinates
 * \param y the dataset y coordinates
 * \return The array of size k + 1 of polynomial coefficients.
 */
std::vector<float> approximate(int k, float const* x, float const* y, int samples)
{
	std::vector<double> matrix;
	std::vector<double> y_dbl(y, y + samples);
	std::vector<double> res(k + 1);

	for (int i = 0; i < samples; ++i) {
		double const base = x[i];
		double pow = 1.0;
		for (int j = 0; j <= k; ++j, pow *= base) {
			matrix.push_back(pow);
		}
	}

	leastSquaresFit(QSize(k + 1, samples), &matrix[0], &res[0], &y_dbl[0]);

	return std::vector<float>(res.begin(), res.end());
}

std::vector<Line> findLines(BinaryImage& pixs)
{
	std::vector<Line> lines;
	int const w = pixs.width();
	int const h = pixs.height();
	Line line;

	while (!(line = findLine2(pixs)).isNull()) {
		if (line.numPixels() < w / 3) {
			continue;
		}

		assert(!line.points.empty());

		int topY = line.points.front().y();
		int botY = topY;

		for (unsigned i = 1; i < line.points.size(); ++i) {
			topY = std::min(line.points[i].y(), topY);
			botY = std::max(line.points[i].y(), botY);
		}

		if (((double)(botY - topY)) / h > 0.05) {
			continue;
		}

		lines.push_back(line);
	}

	// Order lines by y of lefthand endpoint
	std::sort(lines.begin(), lines.end(), LineLessLeftY());

	return lines;
}

std::vector<std::vector<float> > approximateMargins2(
	int w, std::list<Line>& leftLines, std::list<Line>& rightLines,
	std::vector<std::vector<float> > const& currentApproximators,
	int leftHighThresh, int leftLowThresh, int rightHighThresh, int rightLowThresh)
{
	std::vector<std::vector<float> > result(2);
	bool reapproximateLeft = false;
	bool reapproximateRight = false;
	std::list<Line>::iterator leftEliminate(leftLines.end());
	std::list<Line>::iterator rightEliminate(rightLines.end());

	// If there is at least one line inside the margin by too much,
	// eliminate the line inside the margin the most.

	if (currentApproximators.empty()) {
		reapproximateLeft = true;
		reapproximateRight = true;
	} else {
		for (std::list<Line>::iterator it(leftLines.begin()); it != leftLines.end(); ++it) {
			int leftX = qRound(
				currentApproximators[0][0] + currentApproximators[0][1] * it->left.y()
			);

			if (it->left.x() - leftX > leftHighThresh) {
				reapproximateLeft = true;
				if (leftEliminate == leftLines.end() || it->left.x() > leftEliminate->left.x()) {
					leftEliminate = it;
				}
			}
		}

		for (std::list<Line>::iterator it(rightLines.begin()); it != rightLines.end(); ++it) {
			int rightX = qRound(
				currentApproximators[1][0] + currentApproximators[1][1] * it->right.y()
			);

			if (rightX - it->right.x() > rightHighThresh) {
				reapproximateRight = true;
				if (rightEliminate == rightLines.end() || it->right.x() < rightEliminate->right.x()) {
					rightEliminate = it;
				}
			}
		}
	}

	if (leftEliminate != leftLines.end()) {
		leftLines.erase(leftEliminate);
	}
	if (rightEliminate != rightLines.end()) {
		rightLines.erase(rightEliminate);
	}

	// With the new list of lines, compute approximators to margins

	if (reapproximateLeft) {
		std::vector<float> x;
		std::vector<float> y;
		x.reserve(leftLines.size());
		y.reserve(leftLines.size());

		qDebug() << "Using " << x.size() + " lines for left margin";

		BOOST_FOREACH(Line const& line, leftLines) {
			x.push_back(line.left.y());
			y.push_back(line.left.x());
		}

		result[0] = approximate(1, &x[0], &y[0], leftLines.size());
	} else {
		result[0] = currentApproximators[0];
	}

	if (reapproximateRight) {
		std::vector<float> x;
		std::vector<float> y;
		x.reserve(rightLines.size());
		y.reserve(rightLines.size());

		qDebug() << "Using " << x.size() << " lines for right margin";
		BOOST_FOREACH(Line const& line, rightLines) {
			x.push_back(line.right.y());
			y.push_back(line.right.x());
		}

		result[1] = approximate(1, &x[0], &y[0], rightLines.size());
	} else {
		result[1] = currentApproximators[1];
	}

	return result;
}

/**
 * Approximate left and right margins of lines by a first-order
 * polynomial, x = round(b + m*y) = floor(b + m*y + 0.5).
 * Neither x nor y are normalized to the page size. Why x as a function
 * of y? Because the ideal margin is perfectly vertical, which means
 * m = infinity, which isn't very nice for approximating with a polynomial!
 *
 * \param w Image width.
 * \param lines Lines to approximate.
 * \return { left, right }, where left and right are { b, m }.
 */
std::vector<std::vector<float> >
approximateMargins(int w, std::vector<Line> const& lines)
{
	std::vector<std::vector<float> > current;
	std::list<Line> leftLines(lines.begin(), lines.end());
	std::list<Line> rightLines(lines.begin(), lines.end());
	std::vector<std::vector<float> > result;

	//qDebug() << "Beginning margin approximation";
	while (current.empty() || result != current) {
		current = result;
		result = approximateMargins2(w, leftLines, rightLines, current,
				50, 10, 50, 10);
		//qDebug() << "LHM x = " << result[0][1] << "y + " << result[0][0];
		//qDebug() << "RHM x = " << result[1][1] << "y + " << result[1][0];
	}
	//qDebug() << "==============================";
	return result;
}

/**
 * Find the coefficients of the four-point homographic transform that
 * straightens the margin. The transform is:
 *
 * x = a[0] + a[1]*x' + a[2]*y + a[3]*x'*y
 *
 * Note that this is a REVERSE transform. That is, given x',y' in
 * the transformed image, x,y in the original image will be
 * f(x), y'. Also note that the y coordinate does not transform.
 */
std::vector<float> findMarginTransform(
	std::vector<std::vector<float> > const& marginApproximators, int h)
{
	QPointF TL(marginApproximators[0][0], 0);
	QPointF TR(marginApproximators[1][0], 0);
	QPointF BL(marginApproximators[0][0] + marginApproximators[0][1]*(h-1), h-1);
	QPointF BR(marginApproximators[1][0] + marginApproximators[1][1]*(h-1), h-1);

	std::vector<QPointF> points;
	points.reserve(4);

	points.push_back(TL);
	points.push_back(TR);
	points.push_back(BL);
	points.push_back(BR);

	// Find transform for x:
	//
	// b = a * v where
	//
	//
	//     1 x' y x'y <- for transformed Top Left (x',y)
	// a = 1 x' y x'y <- for transformed Top Right
	//     1 x' y x'y <- for transformed BL
	//     1 x' y x'y <- for transformed BR
	//
	//     a0
	// v = a1
	//     a2
	//     a3
	//
	//     x (TL)
	// b = x (TR)
	//     x (BL)
	//     x (BR)
	//
	// v is the solution (a inverse times b) found by Gauss-Jordan
	// elimination (4 equations in 4 unknowns).

	double a[4*4]; // [row][col]
	double b[4];

	for (int i = 0; i < 4; ++i) {
		b[i] = points[i].x();
		a[i * 4 + 0] = 1;
		// a[1] are the x'
		a[i * 4 + 2] = points[i].y();
		// a[3] are the x'*y
	}

	// Fill in the x' in the matrix

	if (TR.x() - TL.x() > BR.x() - BL.x()) {
		// Keep TR and TL the same, because the margins are wider
		// at the top. Move bottom R and L to top R and L.

		a[0 * 4 + 1] = TL.x();
		a[1 * 4 + 1] = TR.x();
		a[2 * 4 + 1] = TL.x();
		a[3 * 4 + 1] = TR.x();
	} else {
		// Margins are wider at the bottom.

		a[0 * 4 + 1] = BL.x();
		a[1 * 4 + 1] = BR.x();
		a[2 * 4 + 1] = BL.x();
		a[3 * 4 + 1] = BR.x();
	}

	// Fill in the x'*y in the matrix

	for (int i = 0; i < 4; ++i) {
		a[i * 4 + 3] = a[i * 4 + 1] * points[i].y();
	}

	double res[4];
	leastSquaresFit(QSize(4, 4), a, res, b);

	return std::vector<float>(res, res + 4);
}

float forwardTransformX(float x, float y, std::vector<float> const& marginTransform)
{
	return (x - marginTransform[0] - marginTransform[2] * y) /
			(marginTransform[1] + marginTransform[3] * y);
}

} // anonymous namespace

BinaryImage undistort(BinaryImage const& input, TaskStatus const& status, DebugImages* dbg)
{
	if (input.isNull()) {
		return input;
	}

	int const width = input.width();
	int const height = input.height();

	ConnectivityMap cmap(input, CONN8);
	if (dbg) {
		dbg->add(cmap.visualized(), "cmap");
	}

	// Collect some info on each connected component.
	std::vector<QPoint> topLeft(cmap.maxLabel(), QPoint(-1, -1));
	std::vector<QRect> cc_bbox(cmap.maxLabel());
	uint32_t const* cmap_line = cmap.data();
	int const cmap_stride = cmap.stride();
	for (int y = 0; y < height; ++y) {
		for (int x = 0; x < width; ++x) {
			uint32_t label = cmap_line[x];
			if (label == 0) {
				continue;
			}

			--label;

			if (topLeft[label] == QPoint(-1, -1)) {
				topLeft[label] = QPoint(x, y);
			}
			cc_bbox[label] |= QRect(x, y, 1, 1);
		}
		cmap_line += cmap_stride;
	}

	if (dbg) {
		BinaryImage cc_bbox_img(input.size(), WHITE);
		BOOST_FOREACH(QRect r, cc_bbox) {
			cc_bbox_img.fill(r, BLACK);
		}
		dbg->add(cc_bbox_img, "cc_bbox");
	}

	// For each connected component, for each x, find the mean y.
	BinaryImage cc_mean_y(input.size(), WHITE);
	uint32_t* cc_mean_y_data = cc_mean_y.data();
	int const cc_mean_y_stride = cc_mean_y.wordsPerLine();
	uint32_t const msb = uint32_t(1) << 31;
	for (size_t i = 0; i < cc_bbox.size(); ++i) {
		uint32_t const label = i + 1;
		int left, top, right, bottom;
		cc_bbox[i].getCoords(&left, &top, &right, &bottom);
		for (int x = left; x <= right; ++x) {
			unsigned sum = 0;
			unsigned points = 0;
			uint32_t const* p_cmap = cmap.data() + top * cmap_stride + x;
			for (int y = top; y <= bottom; ++y, p_cmap += cmap_stride) {
				if (*p_cmap == label) {
					sum += y;
					++points;
				}
			}
			if (points != 0) {
				sum = (sum + points / 2) / points; // Division with rounding.
				cc_mean_y_data[sum * cc_mean_y_stride + (x >> 5)] |= msb >> (x & 31);
			}
		}
	}
	if (dbg) {
		dbg->add(cc_mean_y, "cc_mean_y");
	}

	std::vector<Line> lines(findLines(cc_mean_y));

	if (dbg) {
		// We can reuse cc_mean_y here, as we don't need it anymore.
		cc_mean_y.fill(WHITE);
		uint32_t* dbg_line = cc_mean_y.data();
		int const dbg_stride = cc_mean_y.wordsPerLine();
		BOOST_FOREACH(Line const& line, lines) {
			BOOST_FOREACH(QPoint pt, line.points) {
				dbg_line[pt.y() * dbg_stride + (pt.x() >> 5)] |= msb >> (pt.x() & 31);
			}
		}
		dbg->add(cc_mean_y, "lined-candidates");
	}

	cc_mean_y.release();

	if (lines.size() < 2) {
		return input;
	}

	std::vector<std::vector<float> > marginApproximators(approximateMargins(width, lines));

	if (dbg) {
		QPointF left_vec(marginApproximators[0][0] + marginApproximators[0][1], 1);
		QPointF right_vec(marginApproximators[1][0] + marginApproximators[1][1], 1);
		QLineF left_line(QPointF(marginApproximators[0][0], 0), left_vec);
		QLineF right_line(QPointF(marginApproximators[1][0], 0), right_vec);
		QLineF top_line(0, 0, 1, 0);
		QLineF bottom_line(0, height, 1, height);
		QPointF top_left, bottom_left, top_right, bottom_right;
		left_line.intersect(top_line, &top_left);
		left_line.intersect(bottom_line, &bottom_left);
		right_line.intersect(top_line, &top_right);
		right_line.intersect(bottom_line, &bottom_right);
		QImage img(input.toQImage().convertToFormat(QImage::Format_ARGB32_Premultiplied));
		{
			QPainter painter(&img);
			painter.setRenderHint(QPainter::Antialiasing);
			QPen pen(QColor(0xff, 0x00, 0x00, 180));
			pen.setWidthF(width / 500.0);
			painter.setPen(pen);
			painter.drawLine(top_left, bottom_left);
			painter.drawLine(top_right, bottom_right);
		}
		dbg->add(img, "lined-margins");
	}

	std::vector<float> marginTransform(findMarginTransform(marginApproximators, height));

	qDebug() << "Margin reverse transformation coefficients: ";
	qDebug() << " x =  ";
	qDebug() << "      " << marginTransform[0];
	qDebug() << "    + " << marginTransform[1] << " x";
	qDebug() << "    + " << marginTransform[2] << " y";
	qDebug() << "    + " << marginTransform[3] << " xy";

	// Forward transform the points in the lines so that the
	// margins are straight

	int const leftMargin = qRound(forwardTransformX(marginApproximators[0][0], 0, marginTransform));
	int const rightMargin = qRound(forwardTransformX(marginApproximators[1][0], 0, marginTransform));

	BOOST_FOREACH(Line& line, lines) {
		BOOST_FOREACH(QPoint& p, line.points) {
			p.rx() = qRound(forwardTransformX(p.x(), p.y(), marginTransform));
		}
		line.left.rx() = qRound(forwardTransformX(line.left.x(), line.left.y(), marginTransform));
		line.right.rx() = qRound(forwardTransformX(line.right.x(), line.right.y(), marginTransform));
	}

	QImage margin_transformed_img;

	if (dbg) {
		margin_transformed_img = QImage((input.size() / 10).expandedTo(QSize(1, 1)), QImage::Format_RGB32);
		margin_transformed_img.fill(0xffffffff);
		uint32_t* dbg_data = (uint32_t*)margin_transformed_img.bits();
		int const dbg_stride = margin_transformed_img.bytesPerLine() / 4;

		int h = 65;
		int s = 200;
		int v = 200;

		QColor hsv;

		BOOST_FOREACH(Line const& line, lines) {
			hsv.setHsv(h, s, v);
			h = (h + 85) % 360;
			uint32_t const rgb = hsv.rgba();

			BOOST_FOREACH(QPoint const& p, line.points) {
				QPoint sp((QPointF(p) / 10).toPoint());
				if (sp.x() > 0 && sp.x() < width) {
					dbg_data[sp.y() * dbg_stride + sp.x()] = rgb;
				}
			}
		}

		dbg->add(margin_transformed_img, "corrected-points");
	}

	// Order lines by first element y
	std::sort(lines.begin(), lines.end(), LineLessFirstY());

	// XXX
	bool const double_pass = true;
	bool const sinusoidal_component = true;

	int const MAXROUNDS = double_pass ? 2 : 1;
	bool const BACKWARD_PASS = double_pass;

	std::vector<double> aguess;

	std::auto_ptr<LMfunc> func;
	if (sinusoidal_component) {
		PowerFunc2* f = new PowerFunc2(2, width, height);
		func.reset(f);
		aguess.resize(f->numParams());
		f->initialGuess(&aguess[0]);
	} else {
		PowerFunc* f = new PowerFunc(2, width, height);
		func.reset(f);
		aguess.resize(f->numParams());
		f->initialGuess(&aguess[0]);
	}

	double totalError = 0;
	for (int round = 0; round < MAXROUNDS; ++round) {

		totalError = 0;
		qDebug() << "Beginning round " << (round+1) << " for nonlinear regression...";

		// Perform a forward pass and then a backward pass.
		for (unsigned i = 0; i < lines.size() * (BACKWARD_PASS ? 2 : 1); ++i) {
			int lineIndex = i < lines.size() ? i : 2 * lines.size() - i - 1;
			Line& line = lines[lineIndex];
			int ty = line.points.front().y();
			int by = ty;
			int ay = ty;

			if (BACKWARD_PASS && i == lines.size()) {
				totalError = 0;
			}

			BOOST_FOREACH(QPoint p, line.points) {
				if (p.y() < ty) {
					ty = p.y();
				}
				if (p.y() > by) {
					by = p.y();
				}
				ay += p.y();
			}

			int topY = ty;
			int avgY = (ay + line.points.size() / 2) / line.points.size();
			int botY = by;

			std::vector<double> x(line.points.size());
			std::vector<double> y(line.points.size());
			std::vector<double> s(line.points.size());

			for (size_t ii = 0; ii < line.points.size(); ++ii) {
				x[ii] = (((double)line.points[ii].x()) - leftMargin) / (rightMargin - leftMargin);
				y[ii] = ((double)line.points[ii].y() - topY) / (botY - topY);
				s[ii] = 1;
			}

			std::vector<bool> vary(aguess.size(), true);
			LM::solve(
				line.points.size(), &x[0], 1, &aguess[0], &y[0], &s[0],
				vary, func.get(), 0.001, 0.001, 100
			);

			double const e = LM::chiSquared(
				line.points.size(), &x[0], 1, &aguess[0], &y[0], &s[0], func.get()
			);
			totalError += e;

			qDebug() << "Parameters for line " << lineIndex << ":";
			{
				QDebug d(qDebug());
				BOOST_FOREACH(double param, aguess) {
					d << " " << param;
				}
				d << "  " << (botY-topY) << "  " << topY << "e=" << e;
			}

			if (round != MAXROUNDS-1) {
				continue;
			}

			line.d_approximator = aguess;
			line.d_approximator.push_back(botY - topY);
			line.d_approximator.push_back(topY);
			line.e = LM::chiSquared(line.points.size(), &x[0], 1, &aguess[0], &y[0], &s[0], func.get());
		}
	}

	qDebug() << "Total error: " << totalError;

	if (dbg) {
		uint32_t* const dbg_data = (uint32_t*)margin_transformed_img.bits();
		int const dbg_stride = margin_transformed_img.bytesPerLine() / 4;
		QRect const rect(margin_transformed_img.rect());

		BOOST_FOREACH(Line const& line, lines) {
			for (int xx=leftMargin; xx <= rightMargin; ++xx) {
				double const xpt = ((double)xx - leftMargin)/(rightMargin - leftMargin);
				double val = func->val(&xpt, &line.d_approximator[0]);
				double yval = line.d_approximator[line.d_approximator.size() - 2] * val
							  + line.d_approximator[line.d_approximator.size() - 1];
				int sy = (int)qRound(yval / 10);
				int sx = (xx + 5) / 10;

				if (rect.contains(QPoint(sx, sy))) {
					dbg_data[sy * dbg_stride + sx] = 0xff000000;
				}
			}
		}

		dbg->add(margin_transformed_img, "approx_lines");
	}

	// Determine which end of the line should be used.

	margin_transformed_img = QImage();
	int const leftHeight = lines.back().points.front().y() - lines.front().points.front().y();
	int const rightHeight = lines.back().points.back().y() - lines.front().points.back().y();
	bool const useLeft = leftHeight > rightHeight;

	qDebug() << "Used " << (useLeft ? "left" : "right") << " side for y";

	std::vector<Line> spacedLines;
	spacedLines.reserve(lines.size());

	spacedLines.push_back(lines.front());
	for (std::vector<Line>::const_iterator it(lines.begin()); ++it != lines.end();) {
		double y0 = spacedLines.back().d_approximatedY(
			useLeft ? leftMargin : rightMargin, leftMargin, rightMargin, func.get()
		);
		double y1 = it->d_approximatedY(
			useLeft ? leftMargin : rightMargin, leftMargin, rightMargin, func.get()
		);
		if (y1 - y0 >= height / 100) {
			spacedLines.push_back(*it);
		}
	}

	qDebug() << "Removed " << (lines.size() - spacedLines.size())
			<< " lines for being too close to other lines";

	std::vector<int> spacedY;
	spacedY.reserve(spacedLines.size());
	BOOST_FOREACH(Line const& line, spacedLines) {
		double const y = line.d_approximatedY(
			useLeft ? leftMargin : rightMargin, leftMargin, rightMargin, func.get()
		);
		spacedY.push_back(qRound(y));
	}

	BinaryImage output(input.size(), WHITE);
	uint32_t* out_line = output.data();
	int const out_stride = output.wordsPerLine();
	uint32_t const* const inp_data = input.data();
	int const inp_stride = input.wordsPerLine();

	QRectF const rect(input.rect());
	for (int y = 0; y < height; ++y, out_line += out_stride) {
		// Determine before which line (leftMargin, y) lies

		int k = 0;
		for (; k < (int)spacedY.size(); ++k) {
			if (spacedY[k] >= y) {
				break;
			}
		}

		// Linearly interpolate the parameters between line k and line k-1.
		// Note that if k=0, then use the k=0 parameters, and if k=spacedY.length
		// then use the spacedY.length-1 parameters.

		int lower = k - 1;
		int upper = k;

		if (k==0) {
			lower = 0;
			k = 1;
		} else if (k == (int)spacedY.size()) {
			k = upper = spacedY.size() - 1;
		}

		double y0 = spacedY[lower];
		double y1 = spacedY[upper];
		double f = 0;

		if (upper != lower) {
			f = (y-y0) / (y1-y0);
		}

		for (int x = 0; x < width; ++x) {
			double ypt0 = spacedLines[lower].d_approximatedY(x, leftMargin, rightMargin, func.get());
			double ypt1 = spacedLines[upper].d_approximatedY(x, leftMargin, rightMargin, func.get());
			double dypt0 = ypt0 - spacedLines[lower].d_approximator.back();
			double dypt1 = ypt1 - spacedLines[upper].d_approximator.back();

			double yy;

			if (upper == lower) {
				yy = ypt0 - (y0 - y);
			} else {
				yy = ypt0 + (ypt1 - ypt0) * f;
			}

			double xx = (marginTransform[0] + marginTransform[1]*x + marginTransform[2]*yy + marginTransform[3]*x*yy);

			QPoint pt(QPointF(xx, yy).toPoint());
			if (rect.contains(pt)) {
				uint32_t word = inp_data[pt.y() * inp_stride + (pt.x() >> 5)];
				word >>= 31 - (pt.x() & 31);
				word &= 1;
				word <<= 31 - (x & 31);
				out_line[x >> 5] |= word;
			}
		}
	}

	return output;
}
