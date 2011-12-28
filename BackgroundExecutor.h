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

#ifndef BACKGROUNDEXECUTOR_H_
#define BACKGROUNDEXECUTOR_H_

#include "NonCopyable.h"
#include "IntrusivePtr.h"
#include "AbstractCommand.h"
#include "PayloadEvent.h"
#include <memory>

class BackgroundExecutor
{
	DECLARE_NON_COPYABLE(BackgroundExecutor)
public:
	typedef IntrusivePtr<AbstractCommand0<void> > TaskResultPtr;
	typedef IntrusivePtr<AbstractCommand0<TaskResultPtr> > TaskPtr;
	
	BackgroundExecutor();
	
	/**
	 * \brief Waits for background tasks to finish, then destroys the object.
	 */
	~BackgroundExecutor();
	
	/**
	 * \brief Waits for pending jobs to finish and stop the background thread.
	 *
	 * The destructor also performs these tasks, so this method is only
	 * useful to prematuraly stop task processing.  After shutdown, any
	 * attempts to enqueue a task will be silently ignored.
	 */
	void shutdown();
	
	/**
	 * \brief Enqueue a task for execution in a background thread.
	 *
	 * A task is a functor to be executed in a background thread.
	 * That functor may optionally return another one, that is
	 * to be executed in the thread where this BackgroundExecutor
	 * object was constructed.
	 */
	void enqueueTask(TaskPtr const& task);
private:
	class Impl;
	class Dispatcher;
	typedef PayloadEvent<TaskPtr> TaskEvent;
	typedef PayloadEvent<TaskResultPtr> ResultEvent;
	
	std::auto_ptr<Impl> m_ptrImpl;
};

#endif
