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

#ifndef IMAGEPROC_POLYNOMIAL_SURFACE_H_
#define IMAGEPROC_POLYNOMIAL_SURFACE_H_

#include <QSize>
#include <vector>
#include <stdint.h>

class QImage;

namespace imageproc
{

class BinaryImage;

/**
 * \brief A polynomial function describing a 2D surface.
 */
class PolynomialSurface
{
	// Member-wise copying is OK.
public:
	/**
	 * \brief Calculate a polynomial that approximates the given image.
	 *
	 * \param hor_order The order of the polynomial in horizontal direction.
	 *        Recommended value: 3 or 4.
	 * \param vert_order The order of the polynomial in vertical direction.
	 *        Recommended value: 3 or 4.
	 * \param src The image to approximate.  Must be grayscale and not null.
	 *
	 * \note Building a polynomial surface for full size 300 DPI scans
	 *       takes forever, so pass a downscaled version here. 300x300
	 *       pixels will be fine.  Once built, the polynomial surface
	 *       may then rendered in the original size, if necessary.
	 */
	PolynomialSurface(
		int hor_order, int vert_order, QImage const& src);
	
	/**
	 * \brief Calculate a polynomial that approximates portions of the given image.
	 *
	 * \param hor_order The order of the polynomial in horizontal direction.
	 *        Recommended value: 3 or 4.
	 * \param vert_order The order of the polynomial in vertical direction.
	 *        Recommended value: 3 or 4.
	 * \param src The image to approximate.  Must be grayscale and not null.
	 * \param mask Specifies which areas of \p src to consider.
	 *        A pixel in \p src is considered if the corresponding pixel
	 *        in \p mask is black.
	 *
	 * \note Building a polynomial surface for full size 300 DPI scans
	 *       takes forever, so pass a downscaled version here. 300x300
	 *       pixels will be fine.  Once built, the polynomial surface
	 *       may then rendered in the original size, if necessary.
	 */
	PolynomialSurface(
		int hor_order, int vert_order,
		QImage const& src, BinaryImage const& mask);
	
	/**
	 * \brief Returns the dimentions of the polynomial surface.
	 */
	QSize size() const { return m_size; }
	
	/**
	 * \brief Renders the polynomial surface as a grayscale image.
	 */
	QImage render() const;
	
	/**
	 * \brief Renders the polynomial surface as a grayscale image.
	 *
	 * The surface will be stretched / shrinked to fit the new size.
	 */
	QImage render(QSize const& size) const;
private:
	void maybeReduceOrders(int num_data_points);
	
	void prepareMatrixAndVector(
		QImage const& image,
		std::vector<double>& M, std::vector<double>& V) const;
	
	void prepareMatrixAndVector(
		QImage const& image, BinaryImage const& mask,
		std::vector<double>& M, std::vector<double>& V) const;
	
	void processMaskWord(
		uint8_t const* image_line, uint32_t word,
		int y, int mask_word_idx,
		std::vector<double>& M, std::vector<double>& V) const;
	
	static void leastSquaresFit(
		QSize const& C_size, std::vector<double>& C,
		std::vector<double>& x, std::vector<double>& d);
	
	std::vector<double> m_coeffs;
	QSize m_size;
	int m_horOrder;
	int m_vertOrder;
};

}

#endif
