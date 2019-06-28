/*
    Scan Tailor - Interactive post-processing tool for scanned pages.
    Copyright (C)  Joseph Artsimovich <joseph.artsimovich@gmail.com>

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
#include "GrayImage.h"
#include "Constants.h"
#include <boost/lambda/lambda.hpp>
#include <boost/lambda/bind.hpp>
#include <stdint.h>
#include <math.h>

namespace imageproc
{

namespace gauss_blur_impl
{

void find_iir_constants(
	float* n_p, float *n_m, float *d_p,
	float* d_m, float *bd_p, float *bd_m, float std_dev)
{
	/*  The constants used in the implementation of a casual sequence
	 *  using a 4th order approximation of the gaussian operator
	 */
	
	const float div = sqrt(2.0 * constants::PI) * std_dev;
	const float x0 = -1.783 / std_dev;
	const float x1 = -1.723 / std_dev;
	const float x2 = 0.6318 / std_dev;
	const float x3 = 1.997  / std_dev;
	const float x4 = 1.6803 / div;
	const float x5 = 3.735 / div;
	const float x6 = -0.6803 / div;
	const float x7 = -0.2598 / div;
	
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
	
	float sum_n_p = 0.0;
	float sum_n_m = 0.0;
	float sum_d = 0.0;
	
	for (int i = 0; i <= 4; i++) {
		sum_n_p += n_p[i];
		sum_n_m += n_m[i];
		sum_d += d_p[i];
	}
	
	float const a = sum_n_p / (1.0 + sum_d);
	float const b = sum_n_m / (1.0 + sum_d);
	
	for (int i = 0; i <= 4; i++) {
		bd_p[i] = d_p[i] * a;
		bd_m[i] = d_m[i] * b;
	}
}

} // namespace gauss_blur_impl

GrayImage gaussBlur(GrayImage const& src, float h_sigma, float v_sigma)
{	
	using namespace boost::lambda;

	if (src.isNull()) {
		return src;
	}

	GrayImage dst(src.size());
	gaussBlurGeneric(
		src.size(), h_sigma, v_sigma,
		src.data(), src.stride(), StaticCastValueConv<float>(),
		dst.data(), dst.stride(), _1 = bind<uint8_t>(RoundAndClipValueConv<uint8_t>(), _2)
	);

	return dst;
}

} // namespace imageproc
