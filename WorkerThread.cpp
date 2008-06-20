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

#include "WorkerThread.h.moc"
#include <QCoreApplication>
#include <QThread>
#include <QEvent>
#include <assert.h>

class WorkerThread::Dispatcher : public QObject
{
public:
	Dispatcher(Impl& owner);
protected:
	virtual void customEvent(QEvent* event);
private:
	Impl& m_rOwner;
};


class WorkerThread::Impl : public QThread
{
public:
	Impl(WorkerThread& owner);
	
	~Impl();
	
	void performTask(BackgroundTaskPtr const& task);
protected:
	virtual void run();
	
	virtual void customEvent(QEvent* event);
private:
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

void
WorkerThread::Dispatcher::customEvent(QEvent* event)
{
	PerformTaskEvent* evt = dynamic_cast<PerformTaskEvent*>(event);
	assert(evt);
	BackgroundTaskPtr const& task = evt->task();
	
	if (task->isCancelled()) {
		return;
	}
	
	FilterResultPtr const result((*task)());
	if (result) {
		QCoreApplication::postEvent(
			&m_rOwner, new TaskResultEvent(task, result)
		);
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
	exit();
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
	exec();
}

void
WorkerThread::Impl::customEvent(QEvent* event)
{
	TaskResultEvent* evt = dynamic_cast<TaskResultEvent*>(event);
	assert(evt);
	m_rOwner.emitTaskResult(evt->task(), evt->result());
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
