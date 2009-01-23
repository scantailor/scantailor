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

#define _ISOC99SOURCE // For copysign()

#include "LeastSquaresFit.h"
#include <QSize>
#include <stdexcept>
#include <math.h>
#include <assert.h>

#ifdef _MSC_VER
#undef copysign // Just in case.
#define copysign _copysign
#endif

namespace imageproc
{

void leastSquaresFit(QSize const& C_size, double* C, double* x, double* d)
{
	int const width = C_size.width();
	int const height = C_size.height();
	
	if (width < 0 || height < 0 || height < width) {
		throw std::invalid_argument("leastSquaresFit: invalid dimensions");
	}
	
	// Calculate a QR decomposition of C using Givens rotations.
	// We store R in place of C, while Q is not stored at all.
	// Instead, we rotate the d vector on the fly.
	int jj = 0; // j * width + j
	for (int j = 0; j < width; ++j, jj += width + 1) {
		int ij = jj + width; // i * width + j
		for (int i = j + 1; i < height; ++i, ij += width) {
			double const a = C[jj];
			double const b = C[ij];
			
			if (b == 0.0) {
				continue;
			}
			
			double sin, cos;
			
			if (a == 0.0) {
				cos = 0.0;
				sin = copysign(1.0, b);
				C[jj] = fabs(b);
			} else if (fabs(b) > fabs(a)) {
				double const t = a / b;
				double const u = copysign(sqrt(1.0 + t*t), b);
				sin = 1.0 / u;
				cos = sin * t;
				C[jj] = b * u;
			} else {
				double const t = b / a;
				double const u = copysign(sqrt(1.0 + t*t), a);
				cos = 1.0 / u;
				sin = cos * t;
				C[jj] = a * u;
			}
			C[ij] = 0.0;
			
			int ik = ij + 1; // i * width + k
			int jk = jj + 1; // j * width + k
			for (int k = j + 1; k < width; ++k, ++ik, ++jk) {
				double const temp = cos * C[jk] + sin * C[ik];
				C[ik] = cos * C[ik] - sin * C[jk];
				C[jk] = temp;
			}
			
			// Rotate d.
			double const temp = cos * d[j] + sin * d[i];
			d[i] = cos * d[i] - sin * d[j];
			d[j] = temp;
		}
	}
	
	// Solve R*x = d by back-substitution.
	int ii = width * width - 1; // i * width + i
	for (int i = width - 1; i >= 0; --i, ii -= width + 1) {
		double sum = d[i];
		
		int ik = ii + 1;
		for (int k = i + 1; k < width; ++k, ++ik) {
			sum -= C[ik] * x[k];
		}
		
		assert(C[ii] != 0.0);
		x[i] = sum / C[ii];
	}
}

}
