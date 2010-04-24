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

#ifndef WORKERTHREAD_H_
#define WORKERTHREAD_H_

#include "NonCopyable.h"
#include "BackgroundTask.h"
#include "FilterResult.h"
#include <QObject>
#include <memory>

class WorkerThread : public QObject
{
	Q_OBJECT
	DECLARE_NON_COPYABLE(WorkerThread)
public:	
	WorkerThread(QObject* parent = 0);
	
	~WorkerThread();
	
	/**
	 * \brief Waits for pending jobs to finish and stop the thread.
	 *
	 * The destructor also performs these tasks, so this method is only
	 * useful to prematuraly stop task processing.
	 */
	void shutdown();
public slots:
	void performTask(BackgroundTaskPtr const& task);
signals:
	void taskResult(BackgroundTaskPtr const& task, FilterResultPtr const& result);
private:
	void emitTaskResult(BackgroundTaskPtr const& task, FilterResultPtr const& result);
	
	class Impl;
	class Dispatcher;
	class PerformTaskEvent;
	class TaskResultEvent;
	
	std::auto_ptr<Impl> m_ptrImpl;
};

#endif
