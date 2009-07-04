/*
    Scan Tailor - Interactive post-processing tool for scanned pages.
    Copyright (C) 2007-2009  Joseph Artsimovich <joseph_a@mail.ru>

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

#include "SavGolKernel.h"
#include <QSize>
#include <QPoint>
#include <stdexcept>
#include <string.h>
#include <math.h>
#include <assert.h>

#ifdef _MSC_VER
#undef copysign // Just in case.
#define copysign _copysign
#endif

namespace imageproc
{

namespace
{

int calcNumTerms(int const hor_degree, int const vert_degree)
{
	return (hor_degree + 1) * (vert_degree + 1);
}

} // anonymous namespace


SavGolKernel::SavGolKernel(
	QSize const& size, QPoint const& origin,
	int const hor_degree, int const vert_degree)
:	m_horDegree(hor_degree),
	m_vertDegree(vert_degree),
	m_width(size.width()),
	m_height(size.height()),
	m_numTerms(calcNumTerms(hor_degree, vert_degree)),
	m_numDataPoints(size.width() * size.height())
{
	if (size.isEmpty()) {
		throw std::invalid_argument("SavGolKernel: invalid size");
	}
	if (hor_degree < 0) {
		throw std::invalid_argument("SavGolKernel: invalid hor_degree");
	}
	if (vert_degree < 0) {
		throw std::invalid_argument("SavGolKernel: invalid vert_degree");
	}
	if (m_numTerms > m_numDataPoints) {
		throw std::invalid_argument("SavGolKernel: too high degree for this amount of data");
	}
	
	// Allocate memory.
	m_dataPoints.resize(m_numDataPoints, 0.0);
	m_coeffs.resize(m_numTerms);
	AlignedArray<float, 4>(m_numDataPoints).swap(m_kernel);
	
	// Prepare equations.
	m_equations.reserve(m_numTerms * m_numDataPoints);
	for (int y = 1; y <= m_height; ++y) {
		for (int x = 1; x <= m_width; ++x) {
			double pow1 = 1.0;
			for (int i = 0; i <= m_vertDegree; ++i) {
				double pow2 = pow1;
				for (int j = 0; j <= m_horDegree; ++j) {
					m_equations.push_back(pow2);
					pow2 *= x;
				}
				pow1 *= y;
			}
		}
	}
	
	QR();
	recalcForOrigin(origin);
}

/**
 * Perform a QR factorization of m_equations by Givens rotations.
 * We store R in place of m_equations, and we don't store Q anywhere,
 * but we do store the rotations in the order they were performed.
 */
void
SavGolKernel::QR()
{
	m_rotations.clear();
	m_rotations.reserve(
		m_numTerms * (m_numTerms - 1) / 2
		+ (m_numDataPoints - m_numTerms) * m_numTerms
	);
	
	int jj = 0; // j * m_numTerms + j
	for (int j = 0; j < m_numTerms; ++j, jj += m_numTerms + 1) {
		int ij = jj + m_numTerms; // i * m_numTerms + j
		for (int i = j + 1; i < m_numDataPoints; ++i, ij += m_numTerms) {
			double const a = m_equations[jj];
			double const b = m_equations[ij];
			
			if (b == 0.0) {
				m_rotations.push_back(Rotation(1.0, 0.0));
				continue;
			}
			
			double sin, cos;
			
			if (a == 0.0) {
				cos = 0.0;
				sin = copysign(1.0, b);
				m_equations[jj] = fabs(b);
			} else if (fabs(b) > fabs(a)) {
				double const t = a / b;
				double const u = copysign(sqrt(1.0 + t*t), b);
				sin = 1.0 / u;
				cos = sin * t;
				m_equations[jj] = b * u;
			} else {
				double const t = b / a;
				double const u = copysign(sqrt(1.0 + t*t), a);
				cos = 1.0 / u;
				sin = cos * t;
				m_equations[jj] = a * u;
			}
			m_equations[ij] = 0.0;
			
			m_rotations.push_back(Rotation(sin, cos));
			
			int ik = ij + 1; // i * m_numTerms + k
			int jk = jj + 1; // j * m_numTerms + k
			for (int k = j + 1; k < m_numTerms; ++k, ++ik, ++jk) {
				double const temp = cos * m_equations[jk] + sin * m_equations[ik];
				m_equations[ik] = cos * m_equations[ik] - sin * m_equations[jk];
				m_equations[jk] = temp;
			}
		}
	}
}

void
SavGolKernel::recalcForOrigin(QPoint const& origin)
{
	std::fill(m_dataPoints.begin(), m_dataPoints.end(), 0.0);
	m_dataPoints[origin.y() * m_width + origin.x()] = 1.0;
	
	// Rotate data points.
	double* const dp = &m_dataPoints[0];
	std::vector<Rotation>::const_iterator rot(m_rotations.begin());
	for (int j = 0; j < m_numTerms; ++j) {
		for (int i = j + 1; i < m_numDataPoints; ++i, ++rot) {
			double const temp = rot->cos * dp[j] + rot->sin * dp[i];
			dp[i] = rot->cos * dp[i] - rot->sin * dp[j];
			dp[j] = temp;
		}
	}
	
	// Solve R*x = d by back-substitution.
	int ii = m_numTerms * m_numTerms - 1; // i * m_numTerms + i
	for (int i = m_numTerms - 1; i >= 0; --i, ii -= m_numTerms + 1) {
		double sum = dp[i];
		int ik = ii + 1;
		for (int k = i + 1; k < m_numTerms; ++k, ++ik) {
			sum -= m_equations[ik] * m_coeffs[k];
		}
		
		assert(m_equations[ii] != 0.0);
		m_coeffs[i] = sum / m_equations[ii];
	}
	
	int ki = 0;
	for (int y = 1; y <= m_height; ++y) {
		for (int x = 1; x <= m_width; ++x) {
			double sum = 0.0;
			double pow1 = 1.0;
			int ci = 0;
			for (int i = 0; i <= m_vertDegree; ++i) {
				double pow2 = pow1;
				for (int j = 0; j <= m_horDegree; ++j) {
					sum += pow2 * m_coeffs[ci];
					++ci;
					pow2 *= x;
				}
				pow1 *= y;
			}
			m_kernel[ki] = (float)sum;
			++ki;
		}
	}
}

} // namespace imageproc
