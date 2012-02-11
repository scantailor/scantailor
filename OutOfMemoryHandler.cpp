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

#include "OutOfMemoryHandler.h"
#include "OutOfMemoryHandler.h.moc"
#include <QMutexLocker>
#include <QMetaObject>
#include <Qt>

OutOfMemoryHandler::OutOfMemoryHandler()
:	m_hadOOM(false)
{
}

OutOfMemoryHandler&
OutOfMemoryHandler::instance()
{
	// Depending on the compiler, this may not be thread-safe.
	// However, because we insist an instance of this object to be created early on,
	// the only case that might get us into trouble is an out-of-memory situation
	// after main() has returned and this instance got destroyed.  This scenario
	// sounds rather fantastic, and is not a big deal, as the project would have
	// already been saved.
	static OutOfMemoryHandler object;

	return object;
}

void
OutOfMemoryHandler::allocateEmergencyMemory(size_t bytes)
{
	QMutexLocker const locker(&m_mutex);

	boost::scoped_array<char>(new char[bytes]).swap(m_emergencyBuffer);
}

void
OutOfMemoryHandler::handleOutOfMemorySituation()
{
	QMutexLocker const locker(&m_mutex);

	if (m_hadOOM) {
		return;
	}

	m_hadOOM = true;
	boost::scoped_array<char>().swap(m_emergencyBuffer);
	QMetaObject::invokeMethod (this, "outOfMemory", Qt::QueuedConnection);
}

bool
OutOfMemoryHandler::hadOutOfMemorySituation() const
{
	QMutexLocker const locker(&m_mutex);
	return m_hadOOM;
}
