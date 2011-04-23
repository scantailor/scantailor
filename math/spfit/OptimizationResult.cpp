/*
    Scan Tailor - Interactive post-processing tool for scanned pages.
    Copyright (C)  Joseph Artsimovich <joseph.artsimovich@gmail.com>

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

#include "OptimizationResult.h"
#include <limits>
#include <algorithm>

namespace spfit
{

OptimizationResult::OptimizationResult(
	double force_before, double force_after)
:	m_forceBefore(std::max<double>(force_before, 0)),
	m_forceAfter(std::max<double>(force_after, 0))
{
	// In theory, these distances can't be negative, but in practice they can.
	// We are going to treat negative ones as they are zeros.
}

double
OptimizationResult::improvementPercentage() const
{
	double improvement = m_forceBefore - m_forceAfter;
	improvement /= (m_forceBefore + std::numeric_limits<double>::epsilon());
	return improvement * 100; // Convert to percents.
}

} // namespace spfit
