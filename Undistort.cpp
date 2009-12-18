/*
	Scan Tailor - Interactive post-processing tool for scanned pages.
	Copyright (C) autophile@gmail.com
	Copyright (C) Joseph Artsimovich <joseph_a@mail.ru>

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
#include "imageproc/Morphology.h"
#include <QPoint>
#include <boost/shared_ptr.hpp>
#include <boost/shared_array.hpp>
#include <boost/multi_array.hpp>
#include <boost/array.hpp>
#include <boost/make_shared.hpp>
#include <list>
#include <vector>
#include <math.h>
#include <stdio.h>

using namespace imageproc;

namespace
{

class UndistortLine;

typedef boost::shared_ptr<UndistortLine> RefcountedUndistortLine;

typedef boost::shared_ptr<std::list<RefcountedUndistortLine> > ListOfLine;

typedef std::list<RefcountedUndistortLine>::iterator ListOfLineIterator;

RefcountedUndistortLine findLine(imageproc::BinaryImage& img);

bool gaussjordan(boost::multi_array<double, 2>& a, boost::shared_array<double>& b);

boost::shared_array<double> approximate(
	int k, std::vector<double>& x, std::vector<double>& y);

boost::shared_array<double> approximate(RefcountedUndistortLine& line);

void approximateMargins(ListOfLine& lines,
	boost::shared_array<double>& leftApproximator,
	boost::shared_array<double>& rightApproximator);

ListOfLine findLines(BinaryImage& img);

boost::shared_array<double> findMarginTransform(
	boost::shared_array<double>& leftApproximator,
	boost::shared_array<double>& rightApproximator, int h);

boost::multi_array<double, 2> approximateParameters(
	int k, int approximatorOrder, ListOfLine lines);


class UndistortLine
{
public:
	std::vector<QPoint> points;
	QPoint left;
	QPoint right;
	boost::shared_array<double> approximator;

	UndistortLine() : left(-1, -1), right(-1, -1) {}

	void addPoint(const QPoint& point);

	unsigned int numPixels(void) const { return points.size(); }

	int approximatedY(int x) const;
};

void
UndistortLine::addPoint(QPoint const& point)
{
	points.push_back(point);

	if (left.x() == -1 || point.x() < left.x()) {
		left = point;
	}

	if (right.x() == -1 || point.x() > right.x()) {
		right = point;
	}
}

int
UndistortLine::approximatedY(int x) const
{
	double m = 1;
	double y = 0;

	for (int i=0; i<3; i++)
	{
		y += approximator[i] * m;
		m *= x;
	}

	return (int)floor(y);
}


bool approximateMarginsImpl(
	ListOfLine& leftLines, ListOfLine& rightLines,
	boost::shared_array<double>& leftApproximator,
	boost::shared_array<double>& rightApproximator,
	int highThresh)
{
	bool reapproximateLeft = false;
	bool reapproximateRight = false;

	// Eliminate any line which is inside the current approximated margins
	// by too much. Treat right margin and left margin separately.

	printf("Left pointer to list: %p\n", leftLines.get());
	printf("Right pointer to list: %p\n", rightLines.get());
	printf("Left approximator: %p\n", leftApproximator.get());
	printf("Right approximator: %p\n", rightApproximator.get());

	if (leftApproximator.get() != 0)
	{
		ListOfLineIterator iterator = leftLines->begin();

		while (iterator != leftLines->end())
		{
			RefcountedUndistortLine line(*iterator);

			int leftX = (int)std::floor(0.5 + leftApproximator[0] +
					leftApproximator[1] * line->approximatedY(line->left.x()));

			if (line->left.x() - leftX > highThresh)
			{
				iterator = leftLines->erase(iterator);
				reapproximateLeft = true;
			}
			else
				iterator++;
		}

		iterator = rightLines->begin();

		while (iterator != rightLines->end())
		{
			RefcountedUndistortLine line(*iterator);

			int rightX = (int)std::floor(0.5 + rightApproximator[0] +
					rightApproximator[1] * line->approximatedY(line->right.x()));

			if (rightX - line->right.x() > highThresh)
			{
				iterator = rightLines->erase(iterator);
				reapproximateRight = true;
			}
			else
				iterator++;
		}
	}
	else // note set up yet, so initialize
	{
		reapproximateLeft = true;
		reapproximateRight = true;
	}

	// With the new list of lines, compute approximators to margins

	if (reapproximateLeft)
	{
		std::vector<double> x;
		std::vector<double> y;

		printf("Using %u lines for left margin\n", (unsigned int)(leftLines->size()));

		for (ListOfLineIterator i(leftLines->begin());
				i!=leftLines->end(); i++)
		{
			RefcountedUndistortLine line(*i);
			x.push_back(line->approximatedY(line->left.x()));
			y.push_back(line->left.x());
		}

		leftApproximator = approximate(1, x, y);
	}

	if (reapproximateRight)
	{
		std::vector<double> x;
		std::vector<double> y;

		printf("Using %u lines for right margin\n", (unsigned int)(rightLines->size()));

		for (ListOfLineIterator i(rightLines->begin());
				i!=rightLines->end(); i++)
		{
			RefcountedUndistortLine line(*i);
			x.push_back(line->approximatedY(line->right.x()));
			y.push_back(line->right.x());
		}

		rightApproximator = approximate(1, x, y);
	}

	return reapproximateRight || reapproximateLeft;
}

void approximateMargins(ListOfLine& lines,
	boost::shared_array<double>& leftApproximator,
	boost::shared_array<double>& rightApproximator)
{
	ListOfLine leftLines(boost::make_shared<std::list<RefcountedUndistortLine> >(*lines));
	ListOfLine rightLines(boost::make_shared<std::list<RefcountedUndistortLine> >(*lines));
	bool changed = true;

	leftApproximator.reset();
	rightApproximator.reset();

	printf("Beginning margin approximation\n");
	while (changed)
	{
		changed = approximateMarginsImpl(leftLines, rightLines,
			leftApproximator, rightApproximator, 50);
		printf("LHM x = %g y + %g\n", leftApproximator[1],
			leftApproximator[0]);
		printf("RHM x = %g y + %g\n", rightApproximator[1],
			rightApproximator[0]);
	}
	printf("=======\n");
}

boost::shared_array<double> approximate(RefcountedUndistortLine& line)
{
	std::vector<double> x;
	std::vector<double> y;

	for (int i=0; i<(int)(line->numPixels()); i++)
	{
		x.push_back(line->points[i].x());
		y.push_back(line->points[i].y());
	}

	return approximate(2, x, y); // approximate to 2nd order
}

ListOfLine findLines(BinaryImage& img)
{
	ListOfLine lines(boost::make_shared<std::list<RefcountedUndistortLine> >());

	while(true)
	{
		RefcountedUndistortLine line = findLine(img);

		if (line.get() == 0)
			break;
		if (line->numPixels() > (unsigned int)img.width() / 2)
				lines->push_back(line);
	}

	return lines;
}

RefcountedUndistortLine findLine(BinaryImage& img)
{
	int w = img.width();
	int h = img.height();
	int x;
	int y = 0;
	bool found = false;
	uint32_t* data;
	int wpl = img.wordsPerLine();
	uint32_t mask;

	// Search from left to right, top to bottom to
	// find the beginning of a line

	//printf("Image %dx%d wpl %d\n", w, h, wpl);

	for (x=0; x<w; x++)
	{
		data = img.data() + (x>>5);
		mask = 0x80000000>>(x&31);

		for (y=0; y<h; y++, data+=wpl)
			if ( ( (*data) & mask ) != 0)
			{
				found = true;
				break;
			}
		if (found)
			break;
	}

	if (!found)
		return RefcountedUndistortLine();

	// Now that we have the beginning of a line, attempt to
	// follow the line across the page, deleting the pixels
	// as we go along. We delete the pixels so that the
	// next time we call findLine, the next line will be found.

	RefcountedUndistortLine line(boost::make_shared<UndistortLine>());

	line->addPoint(QPoint(x, y));
	(*data) &= ~mask; // x,y <- 0

	// skipBound is the maximum amount of space we are allowed to skip
	// in the x direction to reach for the next pixel in the line.
	// We also search up/down lineSkipBound pixels.

	int dx;
	int dy = 0;
	int skipBound = w/20;
	int lineSkipBound = 10;

	do
	{
		found = false;

		for (dx=1; dx<skipBound; dx++)
		{
			if (x+dx >= w)
				continue;

			mask = 0x80000000 >> ((x+dx)&31);
			int offset = (y-lineSkipBound)*wpl + ((x+dx)>>5);
			data = img.data() + offset;

			for (dy=-lineSkipBound; dy<=lineSkipBound; dy++, data+=wpl)
			{
				if (y+dy < 0 || y+dy >= h)
					continue;
				if ( ((*data) & mask) != 0)
				{
					//printf("Found pixel at %p, mask %08X, data %08X\n",
					//        data, mask, *data);
					found = true;
					break;
				}
			}
			if (found)
				break;
		}

		if (found)
		{
			x = x+dx;
			y = y+dy;
			line -> addPoint(QPoint(x, y));
			//printf("Add point at %d,%d: at %p, mask %08X, data %08X", x, y,
			//    data, ~mask, *data);
			(*data) &= ~mask; // x,y <- 0
			//printf(" data -> %08X\n", *data);
		}

	} while (found);

	//printf("Found %d pixels in line\n", line->numPixels());
	//for (unsigned int i=0; i<line->numPixels(); i++)
	//    printf("Line pixel is %d,%d\n", line->points[i].x(), line->points[i].y());

	return line;
}



/*
 * Solve Ax = B for x.
 *
 * Inputs: a: an n x n matrix, [row][col]
 *         b: an n x 1 vector
 *
 * Outputs: a: the inverse of the input a
 *          b: the solution x.
 *
 * Returns: true if the solution was found, false if A was a singular matrix
 *   and therefore a solution could not be found.
 *
 * From Numerical Recipes in C, 2ed, code adapted from Leptonica library.
 */

bool gaussjordan(boost::multi_array<double, 2>& a, boost::shared_array<double>& b)
{
	int i, icol = 0, irow = 0, j, k, l, ll;
	double big, dum, pivinv, temp;

	int n = a.shape()[0];

	int indexc[n];
	int indexr[n];
	int ipiv[n];

	for (i=0; i<n; i++)
	{
		indexc[i] = 0;
		indexr[i] = 0;
		ipiv[i] = 0;
	}

	for (i = 0; i < n; i++)
	{
		big = 0.0;
		for (j = 0; j < n; j++)
		{
			if (ipiv[j] != 1)
				for (k = 0; k < n; k++)
				{
					if (ipiv[k] == 0)
					{
						if (std::abs(a[j][k]) >= big)
						{
							big = std::abs(a[j][k]);
							irow = j;
							icol = k;
						}
					}
					else if (ipiv[k] > 1)
						return false;
				}
		}
		++(ipiv[icol]);

		if (irow != icol)
		{
			for (l = 0; l < n; l++)
			{
				// SWAP(a[irow][l], a[icol][l]);
				double tmp = a[irow][l];
				a[irow][l] = a[icol][l];
				a[icol][l] = tmp;
			}
			// SWAP(b[irow], b[icol]);
			double tmp = b[irow];
			b[irow] = b[icol];
			b[icol] = tmp;
		}

		indexr[i] = irow;
		indexc[i] = icol;
		if (a[icol][icol] == 0.0)
			return false;
		pivinv = 1.0 / a[icol][icol];
		a[icol][icol] = 1.0;
		for (l = 0; l < n; l++)
			a[icol][l] *= pivinv;
		b[icol] *= pivinv;

		for (ll = 0; ll < n; ll++)
			if (ll != icol)
			{
				dum = a[ll][icol];
				a[ll][icol] = 0;
				for (l = 0; l < n; l++)
					a[ll][l] -= a[icol][l] * dum;
				b[ll] -= b[icol] * dum;
			}
	}

	for (l = n - 1; l >= 0; l--)
	{
		if (indexr[l] != indexc[l])
			for (k = 0; k < n; k++)
			{
				// SWAP(a[k][indexr[l]], a[k][indexc[l]]);
				double tmp = a[k][indexr[l]];
				a[k][indexr[l]] = a[k][indexc[l]];
				a[k][indexc[l]] = tmp;
			}
	}

	return true;
}

/*
 * Performs a k-order polynomial approximation to the data
 * using least square fitting. The returned array contains the
 * coefficients in order of their power starting from zero,
 * i.e. 1, x, x^2, ..., x^k
 */

boost::shared_array<double> approximate(int k, std::vector<double>& x, std::vector<double>& y)
{
	boost::multi_array<double, 2> a(boost::extents[k+1][k+1]);
	boost::shared_array<double> b(new double[k+1]);

	double py = -1;
	double powers[2*k+1];

	for (int i=0; i<=k; i++)
		b[i] = 0;
	for (int i=0; i<=2*k; i++)
		powers[i] = 0;

	int n = x.size();

	for (int i=0; i<n; i++)
	{
		if (y[i] == py) // skip any duplicated y
			continue;
		py = y[i];

		double xx = x[i];
		double yy = y[i];
		double m = 1;

		for (int j=0; j<2*k+1; j++)
		{
			powers[j] += m;
			if (j<k+1)
				b[j] += yy * m;
			m *= xx;
		}
	}

	for (int i=0; i<k+1; i++)
		for (int j=0; j<k+1; j++)
			a[i][j] = powers[i+j];

	gaussjordan(a, b);

	return b;
}

/**
 * Find the coefficients of the four-point homographic transform that
 * straightens the margin. The transform is:
 *
 * x = a[0] + a[1]*x' + a[2]*y + a[3]*x'*y
 *
 * Note that this is a REVERSE transform. That is, given x',y' in
 * the transformed image, x,y in the original image will be
 * f(x), y'. Note that the y coordinate does not transform,
 * so y' = y.
 *
 * @param leftApproximator from approximateMargins
 * @param rightApproximator from approximateMargins
 * @param h the height of the image in pixels
 * @return the coefficients a[0]-a[3] of the transform
 */
boost::shared_array<double> findMarginTransform(
	boost::shared_array<double>& leftApproximator,
	boost::shared_array<double>& rightApproximator, int h)
{
	QPoint TL((int)std::floor(0.5 + leftApproximator[0]), 0);
	QPoint TR((int)std::floor(0.5 + rightApproximator[0]), 0);
	QPoint BL((int)std::floor(0.5 + leftApproximator[0] +
			leftApproximator[1]*(h-1)), h-1);
	QPoint BR((int)std::floor(0.5 + rightApproximator[0] +
			rightApproximator[1]*(h-1)), h-1);
	QPoint points[4] = { TL, TR, BL, BR };

	boost::multi_array<double, 2> a(boost::extents[4][4]); // [row][col]
	boost::shared_array<double> b(new double[4]);

	printf("Untransformed margins:\n");
	printf("  TL: %dx%d\n", TL.x(), TL.y());
	printf("  TR: %dx%d\n", TR.x(), TR.y());
	printf("  BL: %dx%d\n", BL.x(), BL.y());
	printf("  BR: %dx%d\n", BR.x(), BR.y());

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

	for (int i=0; i<4; i++)
	{
		b[i] = points[i].x();

		a[i][0] = 1;
		// a[1] are the x'
		a[i][2] = points[i].y();
		// a[3] are the x'*y
	}

	// Fill in the x' in the matrix

	if (TR.x()-TL.x() > BR.x()-BL.x())
	{
		// Keep TR and TL the same, because the margins are wider
		// at the top. Move bottom R and L to top R and L.

		a[0][1] = TL.x();
		a[1][1] = TR.x();
		a[2][1] = TL.x();
		a[3][1] = TR.x();
	}
	else
	{
		// Margins are wider at the bottom.

		a[0][1] = BL.x();
		a[1][1] = BR.x();
		a[2][1] = BL.x();
		a[3][1] = BR.x();
	}

	// Fill in the x'*y in the matrix

	for (int i=0; i<4; i++)
		a[i][3] = a[i][1]*points[i].y();

	printf("Matrix A =\n");
	for (int i=0; i<4; i++)
		printf("  %g %g %g %g\n", a[i][0], a[i][1], a[i][2], a[i][3]);
	printf("Vector B =\n");
	for (int i=0; i<4; i++)
		printf("  %g\n", b[i]);

	gaussjordan(a, b);

	printf("Solution X =\n");
	for (int i=0; i<4; i++)
		printf("  %g\n", b[i]);

	return b;
}

/**
 * Approximate three k-order polynomials in y0, one for each of
 * the n parameters in the lines' n-order polynomial approximations.
 * Thus, we end up with a family of curves y(x) = a_0(y0) + a_1(y0)*x +
 * ... + a_n-1(y0)*x^(n-1), where each a_k is a k-order polynomial in y0.
 * So what is y0? It is defined as y(0) -- and therefore a_0(y0) = y0.
 *
 * @param k the order of polynomial in y0 to compute
 * @param lines the list of lines. Each line has an approximator.
 * @return a 2-dimensional array. The first index is the coefficient
 *   number and the second index is the coefficient of
 *   the power (1, y0, y0^2, ..., y0^k).
 */
boost::multi_array<double, 2> approximateParameters(
	int k, int approximatorOrder, ListOfLine lines)
{
	int numLines = lines->size();
	boost::multi_array<double, 2> result(boost::extents[approximatorOrder+1][k+1]);

	for (int n=0; n<approximatorOrder+1; n++)
	{
		std::vector<double> x;
		std::vector<double> y;

		for (ListOfLineIterator iterator = lines->begin();
				iterator!=lines->end(); iterator++)
		{
			// Use the y coordinate of the line at x=0 to create
			// a[n] = f(y).

			x.push_back((*iterator)->approximator[0]); // y(x=0) = a[0]
			y.push_back((*iterator)->approximator[n]); // the coefficient
		}

		boost::shared_array<double> a(approximate(k, x, y));

		for (int i=0; i<k+1; i++)
			result[n][i] = a[i];
	}

	return result;
}

} // anonymous namespace

BinaryImage undistort(BinaryImage const& bin_img, TaskStatus const& status, DebugImages* dbg)
{
	BinaryImage step1(
			imageproc::closeBrick(
				bin_img,
				QSize(100,2),
				WHITE));

	BinaryImage step2(
			imageproc::openBrick(
				step1,
				QSize(100,16),
				WHITE));
	step1.release();

	if (dbg) {
		dbg->add(step2, "undistort_lines");
	}

	char const bottomPattern[] =
		"XXXXX"
		"XXXXX"
		"XXXXX"
		"XXXXX"
		"     "
		"     "
		"     ";

	BinaryImage step3(
			imageproc::hitMissMatch(
				step2,
				WHITE,
				bottomPattern,
				5, 7,
				QPoint(2, 3)));
	step2.release();

	if (dbg) {
		dbg->add(dilateBrick(step3, QSize(5, 5)), "bottom_points");
	}

	// Find lines that go most of the way across the page

	ListOfLine lines = findLines(step3);

	printf("Found %u lines\n", (unsigned int)(lines->size()));

	// Produce second-order polynomial approximations in x for
	// each such line

	for (ListOfLineIterator n = lines->begin();
			n != lines->end(); n++) {
		printf("Estimating line: ");
		(*n)->approximator = approximate(*n);
		printf("%g + %g x + %g x^2\n",
				(*n)->approximator[0],
				(*n)->approximator[1],
				(*n)->approximator[2]);
	}

	// Produce first-order polynomial approximations in y (yes, in *y*)
	// for left and right text margins

	boost::shared_array<double> leftApproximator;
	boost::shared_array<double> rightApproximator;
	approximateMargins(lines,
		leftApproximator, rightApproximator);

	// Create a four-point homographic reverse transform based on
	// the text margins. This transform will only transform the x
	// coordinate, leaving the y coordinate the same.

	boost::shared_array<double> marginTransform =
		findMarginTransform(leftApproximator, rightApproximator, step3.height());

	printf("Margin transform: x = %g + %g x' + %g y' + %g x'y'\n",
		marginTransform[0], marginTransform[1], marginTransform[2],
		marginTransform[3]);

	// Note: Don't change the order of the approximator. 2nd order is
	// good enough, but also the transformation of the final image
	// is hardcoded to assume 2nd order approximators.

	boost::multi_array<double, 2> p(approximateParameters(2, 2, lines));

	printf("====== Parameter approximators for y = a + bx + cx^2 ======\n");
	printf("a = %g + %g y0 + %g y0^2\n",
		p[0][0], p[0][1], p[0][2]);
	printf("b = %g + %g y0 + %g y0^2\n",
		p[1][0], p[1][1], p[1][2]);
	printf("c = %g + %g y0 + %g y0^2\n",
		p[2][0], p[2][1], p[2][2]);

	// Now we can transform the destination image from the source image.
	// For each x,y coordinate, we transform the x coordinate
	// (which straightens the margins), and transform the y coordinate
	// (which uncurves curved lines). The order of transformation doesn't
	// seem to matter much.

	int w = step3.width();
	int h = step3.height();
	int wpl = step3.wordsPerLine();

	printf("Original size   : %dx%d\n", bin_img.width(), bin_img.height());
	printf("Transformed size: %dx%d\n", w, h);

	uint32_t* ddata = step3.data();
	for (int y=0; y<h; y++, ddata+=wpl)
	{

		// Here's where we hardcoded 2nd order approximators

		double A = p[0][0] +
				p[0][1] * y +
				p[0][2] * y * y;
		double B = p[1][0] +
				p[1][1] * y +
				p[1][2] * y * y;
		double C = p[2][0] +
				p[2][1] * y +
				p[2][2] * y * y;

		for (int x=0; x<w; x++)
		{
			// transform y

			int yp = (int)std::floor(0.5 + A + B*x + C*x*x);
			//int yp = y;

			if (yp < 0 || yp >= h)
			{
				printf("Error: transformed y is out of bounds: %d,%d\n", x, y);
				continue;
			}

			// transform x (use transformed y!)

			int xp = (int)std::floor(0.5 +
					marginTransform[0] +
					marginTransform[1]*x +
					marginTransform[2]*yp +
					marginTransform[3]*x*yp);

			if (xp < 0 || xp >= w)
			{
				printf("Error: transformed x is out of bounds: %d,%d\n", x, y);
				continue;
			}

			// if bin_img[xp][yp] is black, then set step3[x][y].

			if ((bin_img.data()[yp*wpl + (xp>>5)] & (0x80000000>>(xp&31))) != 0)
				ddata[x>>5] |= (0x80000000>>(x&31));

		}
	}

	return step3;
}
