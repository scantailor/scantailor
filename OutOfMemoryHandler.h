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

#ifndef OUT_OF_MEMORY_HANDLER_H_
#define OUT_OF_MEMORY_HANDLER_H_

#include "NonCopyable.h"
#include <QObject>
#include <QMutex>
#ifndef Q_MOC_RUN
#include <boost/scoped_array.hpp>
#endif
#include <stddef.h>

class OutOfMemoryHandler : public QObject
{
	Q_OBJECT
	DECLARE_NON_COPYABLE(OutOfMemoryHandler)
public:
	static OutOfMemoryHandler& instance();

	/**
	 * To be called once, before any OOM situations can occur.
	 */
	void allocateEmergencyMemory(size_t bytes);

	/** May be called from any thread. */
	void handleOutOfMemorySituation();

	bool hadOutOfMemorySituation() const;
signals:
	/** Will be dispatched from the main thread. */
	void outOfMemory();
private:
	OutOfMemoryHandler();

	mutable QMutex m_mutex;
	boost::scoped_array<char> m_emergencyBuffer;
	bool m_hadOOM;
};

#endif
