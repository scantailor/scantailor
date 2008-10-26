/*
    Scan Tailor - Interactive post-processing tool for scanned pages.
    Copyright (C) 2007-2008  Joseph Artsimovich <joseph_a@mail.ru>

    Based on code from the GIMP project,
    Copyright (C) 1995 Spencer Kimball and Peter Mattis

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

#include "GaussBlur.h"
#include "Grayscale.h"
#include "Constants.h"
#include <QImage>
#include <QDebug>
#include <vector>
#include <algorithm>
#include <math.h>
#include <string.h>

namespace imageproc
{

static void transfer_pixels(
	double const* src1, double const* src2,
	unsigned char* dest, int const dest_step, int const count)
{
	for (int i = 0; i < count; ++i) {
		int sum = (int)floor(*src1 + *src2 + 0.5);
		if (sum > 255) {
			sum = 255;
		} else if (sum < 0) {
			sum = 0;
		}
		*dest = (unsigned char)sum;
		
		++src1;
		++src2;
		dest += dest_step;
	}
}

static void find_iir_constants(
	double* n_p, double *n_m, double *d_p,
        double* d_m, double *bd_p, double *bd_m, double std_dev)
{
	/*  The constants used in the implemenation of a casual sequence
	 *  using a 4th order approximation of the gaussian operator
	 */
	
	const double div = sqrt(2.0 * constants::PI) * std_dev;
	const double x0 = -1.783 / std_dev;
	const double x1 = -1.723 / std_dev;
	const double x2 = 0.6318 / std_dev;
	const double x3 = 1.997  / std_dev;
	const double x4 = 1.6803 / div;
	const double x5 = 3.735 / div;
	const double x6 = -0.6803 / div;
	const double x7 = -0.2598 / div;
	
	n_p [0] = x4 + x6;
	n_p [1] = (exp(x1)*(x7*sin(x3)-(x6+2*x4)*cos(x3)) +
		exp(x0)*(x5*sin(x2) - (2*x6+x4)*cos (x2)));
	n_p [2] = (2 * exp(x0+x1) *
		((x4+x6)*cos(x3)*cos(x2) - x5*cos(x3)*sin(x2) -
		x7*cos(x2)*sin(x3)) +
		x6*exp(2*x0) + x4*exp(2*x1));
	n_p [3] = (exp(x1+2*x0) * (x7*sin(x3) - x6*cos(x3)) +
		exp(x0+2*x1) * (x5*sin(x2) - x4*cos(x2)));
	n_p [4] = 0.0;
	
	d_p [0] = 0.0;
	d_p [1] = -2 * exp(x1) * cos(x3) -  2 * exp(x0) * cos (x2);
	d_p [2] = 4 * cos(x3) * cos(x2) * exp(x0 + x1) +  exp(2 * x1) + exp(2 * x0);
	d_p [3] = -2 * cos(x2) * exp(x0 + 2*x1) -  2*cos(x3) * exp(x1 + 2*x0);
	d_p [4] = exp(2*x0 + 2*x1);
	
	for (int i = 0; i <= 4; i++) {
		d_m[i] = d_p[i];
	}
	
	n_m[0] = 0.0;
	
	for (int i = 1; i <= 4; i++) {
		n_m[i] = n_p[i] - d_p[i] * n_p[0];
	}
	
	double sum_n_p = 0.0;
	double sum_n_m = 0.0;
	double sum_d = 0.0;
	
	for (int i = 0; i <= 4; i++) {
		sum_n_p += n_p[i];
		sum_n_m += n_m[i];
		sum_d += d_p[i];
	}
	
	double const a = sum_n_p / (1.0 + sum_d);
	double const b = sum_n_m / (1.0 + sum_d);
	
	for (int i = 0; i <= 4; i++) {
		bd_p[i] = d_p[i] * a;
		bd_m[i] = d_m[i] * b;
	}
}

static QImage gaussBlurGrayToGray(QImage const& src, double const std_dev)
{
	int const width = src.width();
	int const height = src.height();
	
	std::vector<double> val_p(std::max(width, height), 0);
	std::vector<double> val_m(std::max(width, height), 0);
	
	double n_p[5], n_m[5], d_p[5], d_m[5], bd_p[5], bd_m[5];
	find_iir_constants(n_p, n_m, d_p, d_m, bd_p, bd_m, std_dev);
	
	QImage dst(width, height, QImage::Format_Indexed8);
	dst.setColorTable(createGrayscalePalette());
	
	unsigned char const* const src_data = src.bits();
	unsigned char* const dst_data = dst.bits();
	int const src_bpl = src.bytesPerLine();
	int const dst_bpl = dst.bytesPerLine();
	
	// Vertical pass.
	for (int x = 0; x < width; ++x) {
		memset(&val_p[0], 0, height * sizeof(double));
		memset(&val_m[0], 0, height * sizeof(double));
		
		unsigned char const* sp_p = src_data + x;
		unsigned char const* sp_m = sp_p + (height - 1) * src_bpl;
		double* vp = &val_p[0];
		double* vm = &val_m[0] + height - 1;
		unsigned char const initial_p = sp_p[0];
		unsigned char const initial_m = sp_m[0];
		
		for (int y = 0; y < height; ++y) {
			int const terms = std::min(y, 4);
			int i = 0;
			int sp_off = 0;
			for (; i <= terms; ++i, sp_off += src_bpl) {
				*vp += n_p[i] * sp_p[-sp_off] - d_p[i] * vp[-i];
				*vm += n_m[i] * sp_m[sp_off] - d_m[i] * vm[i];
			}
			for (; i <= 4; ++i) {
				*vp += (n_p[i] - bd_p[i]) * initial_p;
				*vm += (n_m[i] - bd_m[i]) * initial_m;
			}
			sp_p += src_bpl;
			sp_m -= src_bpl;
			++vp;
			--vm;
		}
		
		transfer_pixels(&val_p[0], &val_m[0], dst_data + x, dst_bpl, height);
	}
	
	// Horizontal pass.
	unsigned char* dst_line = dst_data;
	for (int y = 0; y < height; ++y, dst_line += dst_bpl) {
		memset(&val_p[0], 0, width * sizeof(double));
		memset(&val_m[0], 0, width * sizeof(double));
		
		unsigned char const* sp_p = dst_line;
		unsigned char const* sp_m = dst_line + width - 1;
		double* vp = &val_p[0];
		double* vm = &val_m[0] + width - 1;
		unsigned char const initial_p = sp_p[0];
		unsigned char const initial_m = sp_m[0];
		
		for (int x = 0; x < width; ++x) {
			int const terms = std::min(x, 4);
			int i = 0;
			for (; i <= terms; ++i) {
				*vp += n_p[i] * sp_p[-i] - d_p[i] * vp[-i];
				*vm += n_m[i] * sp_m[i] - d_m[i] * vm[i];
			}
			for (; i <= 4; ++i) {
				*vp += (n_p[i] - bd_p[i]) * initial_p;
				*vm += (n_m[i] - bd_m[i]) * initial_m;
			}
			++sp_p;
			--sp_m;
			++vp;
			--vm;
		}
		
		transfer_pixels(&val_p[0], &val_m[0], dst_line, 1, width);
	}
	
	return dst;
}

QImage gaussBlurGray(QImage const& src, double radius)
{
	if (src.isNull()) {
		return QImage();
	}
	
	if (radius < 1.0) {
		// This algorithm doesn't work for radiused less than one.
		radius = 1.0;
	}
	radius += 1.0; // Include the center pixel.
	
	double const std_dev = sqrt((radius * radius) / (-2.0 * log(1.0 / 255.0)));
	
	return gaussBlurGrayToGray(toGrayscale(src), std_dev);
}

} // namespace imageproc
