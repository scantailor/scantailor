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
    along with this program.  If not, see <http://www.gnu.org/licenses/>
*/

#include "PolynomialSurface.h"
#include "BinaryImage.h"
#include "Grayscale.h"
#include "BitOps.h"
#include <QImage>
#include <QDebug>
#include <stdexcept>
#include <math.h>
#include <stdint.h>
#include <assert.h>

namespace imageproc
{

PolynomialSurface::PolynomialSurface(
	int const hor_order, int const vert_order, QImage const& src)
:	m_size(src.size()),
	m_horOrder(hor_order),
	m_vertOrder(vert_order)
{
	// Note: m_horOrder and m_vertOrder may still change!
	
	if (hor_order < 0) {
		throw std::invalid_argument("PolynomialSurface: horizontal order is invalid");
	}
	if (vert_order < 0) {
		throw std::invalid_argument("PolynomialSurface: vertical order is invalid");
	}
	
	if (src.format() != QImage::Format_Indexed8
			|| src.numColors() != 256 || !src.allGray()) {
		throw std::invalid_argument("PolynomialSurface: not grayscale");
	}
	
	int const num_data_points = src.width() * src.height();
	maybeReduceOrders(num_data_points);
	
	QSize const mat_size(
		(m_horOrder + 1) * (m_vertOrder + 1), num_data_points
	);
	std::vector<double> M;
	std::vector<double> V;
	M.reserve(mat_size.width() * mat_size.height());
	V.reserve(mat_size.height());
	prepareMatrixAndVector(src, M, V);
	
	m_coeffs.resize(mat_size.width());
	leastSquaresFit(mat_size, M, m_coeffs, V);
}

PolynomialSurface::PolynomialSurface(
	int const hor_order, int const vert_order,
	QImage const& src, BinaryImage const& mask)
:	m_size(src.size()),
	m_horOrder(hor_order),
	m_vertOrder(vert_order)
{
	// Note: m_horOrder and m_vertOrder may still change!
	
	if (hor_order < 0) {
		throw std::invalid_argument("PolynomialSurface: horizontal order is invalid");
	}
	if (vert_order < 0) {
		throw std::invalid_argument("PolynomialSurface: vertical order is invalid");
	}
	
	if (src.format() != QImage::Format_Indexed8
			|| src.numColors() != 256 || !src.allGray()) {
		throw std::invalid_argument("PolynomialSurface: not grayscale");
	}
	
	if (src.size() != mask.size()) {
		throw std::invalid_argument("PolynomialSurface: image and mask have different sizes");
	}
	
	int const num_data_points = mask.countBlackPixels();
	maybeReduceOrders(num_data_points);
	
	QSize const mat_size(
		(m_horOrder + 1) * (m_vertOrder + 1), num_data_points
	);
	
	std::vector<double> M;
	std::vector<double> V;
	M.reserve(mat_size.width() * mat_size.height());
	V.reserve(mat_size.height());
	prepareMatrixAndVector(src, mask, M, V);
	
	m_coeffs.resize(mat_size.width());
	leastSquaresFit(mat_size, M, m_coeffs, V);
}

QImage
PolynomialSurface::render() const
{
	return render(m_size);
}

QImage
PolynomialSurface::render(QSize const& size) const
{
	if (size.isEmpty()) {
		return QImage();
	}
	
	QImage image(size, QImage::Format_Indexed8);
	image.setColorTable(createGrayscalePalette());
	int const width = size.width();
	int const height = size.height();
	unsigned char* line = image.bits();
	int const bpl = image.bytesPerLine();
	
	double xscale = m_size.width() - 1;
	if (width == 1) {
		xscale *= 0.5;
	} else {
		xscale /= width - 1;
	}
	
	double yscale = m_size.height() - 1;
	if (height == 1) {
		yscale *= 0.5;
	} else {
		yscale /= height - 1;
	}
	
	for (int y = 0; y < height; ++y, line += bpl) {
		double const y_adjusted = 1.0 + y * yscale;
		for (int x = 0; x < width; ++x) {
			double const x_adjusted = 1.0 + x * xscale;
			int pos = 0;
			double sum = 0.5; // for rounding purposes
			double pow1 = 1.0;
			for (int i = 0; i <= m_vertOrder; ++i) {
				double pow2 = pow1;
				for (int j = 0; j <= m_horOrder; ++j, ++pos) {
					sum += m_coeffs[pos] * pow2;
					pow2 *= x_adjusted;
				}
				pow1 *= y_adjusted;
			}
			int const isum = (int)sum;
			line[x] = static_cast<unsigned char>(qBound(0, isum, 255));
		}
	}
	
	return image;
}

void
PolynomialSurface::maybeReduceOrders(int const num_data_points)
{
	while (num_data_points  < (m_horOrder + 1) * (m_vertOrder + 1)) {
		// Order 0 is actually legal, the cycle will stop eventually.
		if (m_horOrder > m_vertOrder) {
			--m_horOrder;
		} else {
			--m_vertOrder;
		}
	}
}

void
PolynomialSurface::prepareMatrixAndVector(
	QImage const& image,
	std::vector<double>& M, std::vector<double>& V) const
{
	int const width = m_size.width();
	int const height = m_size.height();
	
	// -1 is added to compensate for x starting at 1 instead of 0.
	uint8_t const* line = image.bits() - 1;
	int const bpl = image.bytesPerLine();
	
	for (int y = 1; y <= height; ++y, line += bpl) {
		for (int x = 1; x <= width; ++x) {
			V.push_back(line[x]);
			
			double pow1 = 1.0;
			for (int i = 0; i <= m_vertOrder; ++i, pow1 *= y) {
				double pow2 = pow1;
				for (int j = 0; j <= m_horOrder; ++j, pow2 *= x) {
					M.push_back(pow2);
				}
			}
		}
	}
}

void
PolynomialSurface::prepareMatrixAndVector(
	QImage const& image, BinaryImage const& mask,
	std::vector<double>& M, std::vector<double>& V) const
{
	int const width = image.width();
	int const height = image.height();
	
	// -1 is added to compensate for x starting at 1 instead of 0.
	uint8_t const* image_line = image.bits() - 1;
	int const image_bpl = image.bytesPerLine();
	
	uint32_t const* mask_line = mask.data();
	int const mask_wpl = mask.wordsPerLine();
	int const last_word_idx = (width - 1) >> 5;
	int const last_word_mask = ~uint32_t(0) << (31 - ((width - 1) & 31));
	
	for (int y = 1; y <= height; ++y) {
		int idx = 0;
		
		// Full words.
		for (; idx < last_word_idx; ++idx) {
			processMaskWord(image_line, mask_line[idx], idx, y, M, V);
		}
		
		// Last word.
		processMaskWord(
			image_line, mask_line[idx] & last_word_mask, idx, y, M, V
		);
		
		image_line += image_bpl;
		mask_line += mask_wpl;
	}
}

void
PolynomialSurface::processMaskWord(
	uint8_t const* image_line,
	uint32_t word, int word_idx, int y,
	std::vector<double>& M, std::vector<double>& V) const
{
	uint32_t const msb = uint32_t(1) << 31;
	int const xbase = 1 + (word_idx << 5);
	
	int x = xbase;
	uint32_t mask = msb;
	
	for (; word; word &= ~mask, mask >>= 1, ++x) {
		if (!(word & mask)) {
			// Skip a group of zero bits.
			int const offset = countMostSignificantZeroes(word);
			x = xbase + offset;
			mask = msb >> offset;
			assert(word & mask);
		}
		
		// Note that image_line is biased to compensate the
		// fact that x goes from 1 instead of 0.
		V.push_back(image_line[x]);
		
		double pow1 = 1.0;
		for (int i = 0; i <= m_vertOrder; ++i, pow1 *= y) {
			double pow2 = pow1;
			for (int j = 0; j <= m_horOrder; ++j, pow2 *= x) {
				M.push_back(pow2);
			}
		}
	}
}

/**
 * \brief Solves C*x = d
 *
 * The result is written into vector x, while C and d are modified.
 */
void
PolynomialSurface::leastSquaresFit(
	QSize const& C_size, std::vector<double>& C,
	std::vector<double>& x, std::vector<double>& d)
{
	int const height = C_size.height();
	int const width = C_size.width();
	
	assert(height >= width);

	// Calculate a QR decomposition of C using Givens rotations.
	// We store R in place of C, while Q is not stored at all.
	// Instead, we rotate the d vector on the fly.
	int jj = 0; // j * width + j
	for (int j = 0; j < width; ++j, jj += width + 1) {
		int ij = jj + width; // i * width + j
		for (int i = j + 1; i < height; ++i, ij += width) {
			double const a = C[jj];
			double const b = C[ij];
			double const radius = sqrt(a*a + b*b);
			double const cos = a / radius;
			double const sin = b / radius;
			
			C[jj] = radius;
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
