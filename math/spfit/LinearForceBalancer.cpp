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

#include "LinearForceBalancer.h"
#include <math.h>

namespace spfit
{

LinearForceBalancer::LinearForceBalancer(double internal_external_ratio)
: m_currentRatio(internal_external_ratio)
, m_targetRatio(internal_external_ratio)
, m_rateOfChange(0)
, m_iterationsToTarget(0)
{
}

void
LinearForceBalancer::setCurrentRatio(double internal_external_ratio)
{
	m_currentRatio = internal_external_ratio;
	recalcRateOfChange();
}

void
LinearForceBalancer::setTargetRatio(double internal_external_ratio)
{
	m_targetRatio = internal_external_ratio;
	recalcRateOfChange();
}

void
LinearForceBalancer::setIterationsToTarget(int iterations)
{
	m_iterationsToTarget = iterations;
	recalcRateOfChange();
}

double
LinearForceBalancer::calcInternalForceWeight(double internal_force, double external_force) const
{
	// (internal * lambda) / external = ratio
	// internal * lambda = external * ratio
	double lambda = 0;
	if (fabs(internal_force) > 1e-6) {
		lambda = m_currentRatio * external_force / internal_force;
	}
	return lambda;
}

void
LinearForceBalancer::nextIteration()
{
	if (m_iterationsToTarget > 0) {
		--m_iterationsToTarget;
		m_currentRatio += m_rateOfChange;
	}
}

void
LinearForceBalancer::recalcRateOfChange()
{
	if (m_iterationsToTarget <= 0) {
		// Already there.
		m_rateOfChange = 0;
	} else {
		m_rateOfChange = (m_targetRatio - m_currentRatio) / m_iterationsToTarget;
	}
}

} // namespace spfit
