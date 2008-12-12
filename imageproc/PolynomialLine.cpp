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
PolynomialLine::prepareEquations(
	std::vector<double>& equations,
	int const degree, int const num_values)
{
	equations.reserve((degree + 1) * num_values);
	
	// Pretend that data points are positioned in range of [1, 2].
	double const scale = calcScale(num_values);
	for (int i = 0; i < num_values; ++i) {
		double const position = 1.0 + i * scale;
		double pow = 1.0;
		for (int j = 0; j <= degree; ++j, pow *= position) {
			equations.push_back(pow);
		}
	}
}

}
