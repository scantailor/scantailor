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

#ifndef BACKGROUNDTASK_H_
#define BACKGROUNDTASK_H_

#include "AbstractCommand.h"
#include "IntrusivePtr.h"
#include "FilterResult.h"
#include "TaskStatus.h"
#include <QAtomicInt>
#include <exception>

class BackgroundTask : public AbstractCommand0<FilterResultPtr>, public TaskStatus
{
public:
	class CancelledException : public std::exception
	{
	public:
		virtual char const* what() const throw();
	};
	
	virtual void cancel() { m_cancelFlag.fetchAndStoreRelaxed(1); }
	
	virtual bool isCancelled() const {
		return m_cancelFlag.fetchAndAddRelaxed(0) != 0;
	}
	
	/**
	 * \brief If cancelled, throws CancelledException.
	 */
	virtual void throwIfCancelled() const;
private:
	mutable QAtomicInt m_cancelFlag;
};

typedef IntrusivePtr<BackgroundTask> BackgroundTaskPtr;

#endif
