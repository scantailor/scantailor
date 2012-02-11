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
#include "WorkerThread.h"
#include "WorkerThread.h.moc"
#include "ThreadPriority.h"
#include "OutOfMemoryHandler.h"
#include <QCoreApplication>
#include <QThread>
#include <QEvent>
#include <QSettings>
#include <new>
#include <assert.h>

#if defined(Q_OS_LINUX) // For Linux updatePriority()
#include <unistd.h>
#include <errno.h>
#include <sys/resource.h>
#endif

class WorkerThread::Dispatcher : public QObject
{
public:
	enum UpdatePriorityResult {
		PriorityUpdated,
		PriorityUpdateFailed,
		ThreadRestartRequired
	};

	Dispatcher(Impl& owner);

	UpdatePriorityResult updateThreadPriority(BackgroundTask const& task);

	void maybeProcessQueuedTask();
protected:
	virtual void customEvent(QEvent* event);
private:
	void processTask(BackgroundTaskPtr const& task);

	Impl& m_rOwner;

	/**
	 * This one will be set if we decide we need to restart
	 * the background thread before processing a given task.
	 */
	BackgroundTaskPtr m_ptrQueuedTask;
};


class WorkerThread::Impl : public QThread
{
public:
	enum { NormalExit = 0, ExitForRestart };

	Impl(WorkerThread& owner);
	
	~Impl();
	
	void performTask(BackgroundTaskPtr const& task);
protected:
	virtual void run();
	
	virtual void customEvent(QEvent* event);
protected:
	void resetPriority();
private:
	static QEvent::Type const ThreadRestartEvent = (QEvent::Type)(QEvent::User + 1);

	WorkerThread& m_rOwner;
	Dispatcher m_dispatcher;
	bool m_threadStarted;
};


class WorkerThread::PerformTaskEvent : public QEvent
{
public:
	PerformTaskEvent(BackgroundTaskPtr const& task);
	
	BackgroundTaskPtr const& task() const { return m_ptrTask; }
private:
	BackgroundTaskPtr m_ptrTask;
};


class WorkerThread::TaskResultEvent : public QEvent
{
public:
	TaskResultEvent(BackgroundTaskPtr const& task, FilterResultPtr const& result);
	
	BackgroundTaskPtr const& task() const { return m_ptrTask; }
	
	FilterResultPtr const& result() const { return m_ptrResult; }
private:
	BackgroundTaskPtr m_ptrTask;
	FilterResultPtr m_ptrResult;
};


/*=============================== WorkerThread ==============================*/

WorkerThread::WorkerThread(QObject* parent)
:	QObject(parent),
	m_ptrImpl(new Impl(*this))
{
}

WorkerThread::~WorkerThread()
{
}

void
WorkerThread::shutdown()
{
	m_ptrImpl.reset();
}

void
WorkerThread::performTask(BackgroundTaskPtr const& task)
{
	if (m_ptrImpl.get()) {
		m_ptrImpl->performTask(task);
	}
}

void
WorkerThread::emitTaskResult(
	BackgroundTaskPtr const& task, FilterResultPtr const& result)
{
	emit taskResult(task, result);
}


/*======================== WorkerThread::Dispatcher ========================*/

WorkerThread::Dispatcher::Dispatcher(Impl& owner)
:	m_rOwner(owner)
{
}

WorkerThread::Dispatcher::UpdatePriorityResult
WorkerThread::Dispatcher::updateThreadPriority(BackgroundTask const& task)
{
	assert(QCoreApplication::instance()->thread() != QThread::currentThread());

	ThreadPriority prio(
		ThreadPriority::load(
			"settings/batch_processing_priority", ThreadPriority::Normal
		)
	);
	if (task.type() == task.INTERACTIVE) {
		prio.setValue(ThreadPriority::Normal);
	}
	
#if defined(Q_OS_LINUX)
	/*
	On Linux, there is no good way of adjusting priority of an individual
	thread of a process.  In particular, pthread_setschedparam() only works
	for realtime threads.
	Fortunately, the following quote from pthread(7) provides a workaround
	that's ugly but better than nothing:
	----------------------------------------------------
	NPTL still has a few non-conformances with POSIX.1:
	...
	Threads do not share a common nice value.
	----------------------------------------------------
	*/
	if (setpriority(PRIO_PROCESS, 0, prio.toPosixNiceLevel()) != 0) {
		if (errno == EACCES) {
			// Unless the executable has special permissions, lowering
			// the process nice value (even if it doesn't go below zero)
			// will fail with this error code.  In this case, we restart
			// the thread.
			return ThreadRestartRequired;
		} else {
			return PriorityUpdateFailed;
		}
	}
#else
	// QThread::setPriority() doesn't do anything on Linux BTW.
	m_rOwner.setPriority(prio.toQThreadPriority());
#endif

	return PriorityUpdated;
}

void
WorkerThread::Dispatcher::maybeProcessQueuedTask()
{
	if (m_ptrQueuedTask.get()) {
		BackgroundTaskPtr task;
		m_ptrQueuedTask.swap(task);

		// In this case we must not restart the thread if updateThreadPriority()
		// tells us we should.  After all, we've just did that, and doing it again
		// would probably lead to infinite loop.
		updateThreadPriority(*task);

		processTask(task);
	}
}

void
WorkerThread::Dispatcher::customEvent(QEvent* event)
{
	PerformTaskEvent* evt = dynamic_cast<PerformTaskEvent*>(event);
	assert(evt);
	BackgroundTaskPtr const& task = evt->task();
	
	if (updateThreadPriority(*task) == ThreadRestartRequired) {
		m_ptrQueuedTask = task;
		m_rOwner.exit(Impl::ExitForRestart);
		return;
	}

	processTask(task);
}

void
WorkerThread::Dispatcher::processTask(BackgroundTaskPtr const& task)
{
	if (task->isCancelled()) {
		return;
	}

	try {
		FilterResultPtr const result((*task)());
		if (result) {
			QCoreApplication::postEvent(
				&m_rOwner, new TaskResultEvent(task, result)
			);
		}
	} catch (std::bad_alloc const&) {
		OutOfMemoryHandler::instance().handleOutOfMemorySituation();
	}
}


/*========================== WorkerThread::Impl ============================*/

WorkerThread::Impl::Impl(WorkerThread& owner)
:	m_rOwner(owner),
	m_dispatcher(*this),
	m_threadStarted(false)
{
	m_dispatcher.moveToThread(this);
}

WorkerThread::Impl::~Impl()
{
	exit(NormalExit);
	wait();
}

void
WorkerThread::Impl::performTask(BackgroundTaskPtr const& task)
{
	QCoreApplication::postEvent(&m_dispatcher, new PerformTaskEvent(task));
	if (!m_threadStarted) {
		start();
		m_threadStarted = true;
	}
}

void
WorkerThread::Impl::run()
{
	m_dispatcher.maybeProcessQueuedTask();

	if (exec() == ExitForRestart) {
		QCoreApplication::postEvent(this, new QEvent(ThreadRestartEvent));
	}
}

void
WorkerThread::Impl::customEvent(QEvent* event)
{
	if (event->type() == ThreadRestartEvent) {
		start();
		return;
	}

	if (TaskResultEvent* evt = dynamic_cast<TaskResultEvent*>(event)) {
		m_rOwner.emitTaskResult(evt->task(), evt->result());
	}
}


/*====================== WorkerThread::PerformTaskEvent ====================*/

WorkerThread::PerformTaskEvent::PerformTaskEvent(
	BackgroundTaskPtr const& task)
:	QEvent(User),
	m_ptrTask(task)
{
}


/*====================== WorkerThread::TaskResultEvent =====================*/

WorkerThread::TaskResultEvent::TaskResultEvent(
	BackgroundTaskPtr const& task, FilterResultPtr const& result)
:	QEvent(User),
	m_ptrTask(task),
	m_ptrResult(result)
{
}

