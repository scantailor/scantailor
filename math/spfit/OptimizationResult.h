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

#ifndef SPFIT_OPTIMIZATION_RESULT_H_
#define SPFIT_OPTIMIZATION_RESULT_H_

#include <limits>

namespace spfit
{

class OptimizationResult
{
public:
	OptimizationResult(double force_before, double force_after);

	double forceBefore() const { return m_forceBefore; }

	double forceAfter() const { return m_forceAfter; }

	/**
	 * \brief Returns force decrease in percents.
	 *
	 * Force decrease can theoretically be negative.
	 *
	 * \note Improvements from different optimization runs can't be compared,
	 *       as the absolute force values depend on the number of samples,
	 *       which varies from one optimization iteration to another.
	 */
	double improvementPercentage() const;
private:
	double m_forceBefore;
	double m_forceAfter;
};

} // namespace spfit

#endif
