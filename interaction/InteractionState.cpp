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

#include "InteractionState.h"
#include <limits>

InteractionState::Captor&
InteractionState::Captor::operator=(Captor& other)
{
	swap_nodes(other);
	other.unlink();
	return *this;
}

InteractionState::Captor&
InteractionState::Captor::operator=(CopyHelper other)
{
	return (*this = *other.captor);
}

InteractionState::InteractionState()
:	m_proximityThreshold(Proximity::fromDist(10.0)),
	m_bestProximityPriority(std::numeric_limits<int>::min()),
	m_redrawRequested(false)
{
}

void
InteractionState::capture(Captor& captor)
{
	captor.unlink();
	m_captorList.push_back(captor);
}

bool
InteractionState::capturedBy(Captor const& captor) const
{
	return !m_captorList.empty() && &m_captorList.back() == &captor;
}

void
InteractionState::resetProximity()
{
	m_proximityLeader.clear();
	m_bestProximity = Proximity();
	m_bestProximityPriority = std::numeric_limits<int>::min();
}

void
InteractionState::updateProximity(
	Captor& captor, Proximity const& proximity,
	int priority, Proximity proximity_threshold)
{
	if (captor.is_linked()) {
		return;
	}

	if (proximity_threshold == Proximity()) {
		proximity_threshold = m_proximityThreshold;
	}

	if (proximity <= proximity_threshold) {
		if (betterProximity(proximity, priority)) {
			m_proximityLeader.clear();
			m_proximityLeader.push_back(captor);
			m_bestProximity = proximity;
			m_bestProximityPriority = priority;
		}
	}
}

bool
InteractionState::proximityLeader(Captor const& captor) const
{
	return !m_proximityLeader.empty() && &m_proximityLeader.front() == &captor;
}

bool
InteractionState::betterProximity(Proximity const& proximity, int const priority) const
{
	if (priority != m_bestProximityPriority) {
		return priority > m_bestProximityPriority;
	}
	return proximity < m_bestProximity;
}

QCursor
InteractionState::cursor() const
{
	if (!m_captorList.empty()) {
		return m_captorList.back().interactionCursor();
	} else if (!m_proximityLeader.empty()) {
		return m_proximityLeader.front().proximityCursor();
	} else {
		return QCursor();
	}
}

QString
InteractionState::statusTip() const
{
	if (!m_captorList.empty()) {
		return m_captorList.back().interactionOrProximityStatusTip();
	} else if (!m_proximityLeader.empty()) {
		return m_proximityLeader.front().proximityStatusTip();
	} else {
		return m_defaultStatusTip;
	}
}
