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

#include "PolynomialLine.h"
#include "MatT.h"
#include "MatrixCalc.h"
#include <stdexcept>

namespace imageproc
{

void
PolynomialLine::validateArguments(
	int const degree, int const num_values)
{
	if (degree < 0) {
		throw std::invalid_argument("PolynomialLine: degree is invalid");
	}
	if (num_values <= 0) {
		throw std::invalid_argument("PolynomialLine: no data points");
	}
}

double
PolynomialLine::calcScale(int const num_values)
{
	if (num_values <= 1) {
		return 0.0;
	} else {
		return 1.0 / (num_values - 1);
	}
}

void
PolynomialLine::doLeastSquares(VecT<double> const& data_points, VecT<double>& coeffs)
{	
	int const num_terms = coeffs.size();
	int const num_values = data_points.size();

	// The least squares equation is A^T*A*x = A^T*b
	// We will be building A^T*A and A^T*b incrementally.
	// This allows us not to build matrix A at all.
	MatT<double> AtA(num_terms, num_terms);
	VecT<double> Atb(num_terms);

	// 1, x, x^2, x^3, ...
	VecT<double> powers(num_terms);

	// Pretend that data points are positioned in range of [0, 1].
	double const scale = calcScale(num_values);
	for (int x = 0; x < num_values; ++x) {
		double const position = x * scale;
		double const data_point = data_points[x];
		
		// Fill the powers vector.
		double pow = 1.0;
		for (int j = 0; j < num_terms; ++j) {
			powers[j] = pow;
			pow *= position;
		}

		// Update AtA and Atb.
		for (int i = 0; i < num_terms; ++i) {
			double const i_val = powers[i];
			Atb[i] += i_val * data_point;
			for (int j = 0; j < num_terms; ++j) {
				double const j_val = powers[j];
				AtA(i, j) += i_val * j_val;
			}
		}
	}

	// In case AtA is rank-deficient, we can usually fix it like this:
	for (int i = 0; i < num_terms; ++i) {
		AtA(i, i) += 1e-5; // Add a small value to the diagonal.
	}

	try {
		DynamicMatrixCalc<double> mc;
		mc(AtA).solve(mc(Atb)).write(coeffs.data());
	} catch (std::runtime_error const&) {}
}

} // namespace imageproc
