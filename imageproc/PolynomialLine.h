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

#ifndef IMAGEPROC_POLYNOMIAL_LINE_H_
#define IMAGEPROC_POLYNOMIAL_LINE_H_

#include "LeastSquaresFit.h"
#include <QSize>
#include <vector>
#include <limits>
#include <math.h>

namespace imageproc
{

/**
 * \brief A polynomial function describing a sequence of numbers.
 */
class PolynomialLine
{
	// Member-wise copying is OK.
public:
	/**
	 * \brief Calculate a polynomial that approximates a sequence of values.
	 *
	 * \param degree The degree of a polynomial to be constructed.
	 *        If there are too few data points, the degree may
	 *        be silently reduced.  The minimum degree is 0.
	 * \param values The data points to be approximated.
	 * \param num_values The number of data points to be approximated.
	 *        There has to be at least one data point.
	 * \param step The distance between adjacent data points.
	 *        The data points will be accessed like this:\n
	 *        values[0], values[step], values[step * 2]
	 */
	template<typename T>
	PolynomialLine(
		int degree, T const* values, int num_values, int step);
	
	/**
	 * \brief Output the polynomial as a sequence of values.
	 *
	 * \param values The data points to be written.  If T is
	 *        an integer, the values will be rounded and clipped
	 *        to the minimum and maximum values for type T,
	 *        which are taken from std::numeric_limits<T>.
	 *        Otherwise, a static cast will be used to covert
	 *        values from double to type T.  If you need
	 *        some other behaviour, use the overloaded version
	 *        of this method and supply your own post-processor.
	 * \param num_values The number of data points to write.  If this
	 *        number is different from the one that was used to
	 *        construct a polynomial, the output will be scaled
	 *        to fit the new size.
	 * \param step The distance between adjacent data points.
	 *        The data points will be accessed like this:\n
	 *        values[0], values[step], values[step * 2]
	 */
	template<typename T>
	void output(T* values, int num_values, int step) const;
	
	/**
	 * \brief Output the polynomial as a sequence of values.
	 *
	 * \param values The data points to be written.
	 * \param num_values The number of data points to write.  If this
	 *        number is different from the one that was used to
	 *        construct a polynomial, the output will be scaled
	 *        to fit the new size.
	 * \param step The distance between adjacent data points.
	 *        The data points will be accessed like this:\n
	 *        values[0], values[step], values[step * 2]
	 * \param pp A functor to convert a double value to type T.
	 *        The functor will be called like this:\n
	 *        T t = pp((double)val);
	 */
	template<typename T, typename PostProcessor>
	void output(T* values, int num_values, int step, PostProcessor pp) const;
private:
	template<typename T>
	class StaticCastPostProcessor
	{
	public:
		T operator()(double val) const;
	};
	
	template<typename T>
	class RoundAndClipPostProcessor
	{
	public:
		RoundAndClipPostProcessor();
		
		T operator()(double val) const;
	private:
		T m_min;
		T m_max;
	};
	
	template<typename T, bool IsInteger>
	struct DefaultPostProcessor : public StaticCastPostProcessor<T> {};
	
	template<typename T>
	struct DefaultPostProcessor<T, true>
	: public RoundAndClipPostProcessor<T> {};
	
	static void validateArguments(int degree, int num_values);
	
	static double calcScale(int num_values);
	
	static void prepareEquations(
		std::vector<double>& equations,
		int degree, int num_values);
	
	std::vector<double> m_coeffs;
};


template<typename T>
inline T
PolynomialLine::StaticCastPostProcessor<T>::operator()(double const val) const
{
	return static_cast<T>(val);
}

template<typename T>
PolynomialLine::RoundAndClipPostProcessor<T>::RoundAndClipPostProcessor()
:	m_min(std::numeric_limits<T>::min()),
	m_max(std::numeric_limits<T>::max())
{
}

template<typename T>
inline T
PolynomialLine::RoundAndClipPostProcessor<T>::operator()(double const val) const
{
	double const rounded = floor(val + 0.5);
	if (rounded < m_min) {
		return m_min;
	} else if (rounded > m_max) {
		return m_max;
	} else {
		return static_cast<T>(rounded);
	}
}

template<typename T>
PolynomialLine::PolynomialLine(
	int degree, T const* values,
	int const num_values, int const step)
{
	validateArguments(degree, num_values);
	
	if (degree + 1 > num_values) {
		degree = num_values - 1;
	}
	
	QSize const dimensions(degree + 1, num_values);
	std::vector<double> equations;
	std::vector<double> data_points;
	data_points.resize(dimensions.height());
	m_coeffs.resize(dimensions.width());
	
	prepareEquations(equations, degree, num_values);
	
	for (int i = 0; i < num_values; ++i, values += step) {
		data_points[i] = *values;
	}
	
	leastSquaresFit(dimensions, &equations[0], &m_coeffs[0], &data_points[0]);
}

template<typename T>
void
PolynomialLine::output(T* values, int num_values, int step) const
{
	typedef DefaultPostProcessor<
		T, std::numeric_limits<T>::is_integer
	> PP;
	output(values, num_values, step, PP());
}

template<typename T, typename PostProcessor>
void
PolynomialLine::output(
	T* values, int num_values, int step, PostProcessor pp) const
{
	if (num_values <= 0) {
		return;
	}
	
	// Pretend that data points are positioned in range of [1, 2].
	double const scale = calcScale(num_values);
	for (int i = 0; i < num_values; ++i, values += step) {
		double const position = 1.0 + i * scale;
		double sum = 0.0;
		double pow = 1.0;
		
		std::vector<double>::const_iterator it(m_coeffs.begin());
		std::vector<double>::const_iterator const end(m_coeffs.end());
		for (; it != end; ++it, pow *= position) {
			sum += *it * pow;
		}
		
		*values = pp(sum);
	}
}

}

#endif
