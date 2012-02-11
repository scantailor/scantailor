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

#include "BackgroundExecutor.h"
#include "OutOfMemoryHandler.h"
#include <QCoreApplication>
#include <QObject>
#include <QThread>
#include <QEvent>
#include <new>
#include <assert.h>

class BackgroundExecutor::Dispatcher : public QObject
{
public:
	Dispatcher(Impl& owner);
protected:
	virtual void customEvent(QEvent* event);
private:
	Impl& m_rOwner;
};


class BackgroundExecutor::Impl : public QThread
{
public:
	Impl(BackgroundExecutor& owner);
	
	~Impl();
	
	void enqueueTask(TaskPtr const& task);
protected:
	virtual void run();
	
	virtual void customEvent(QEvent* event);
private:
	BackgroundExecutor& m_rOwner;
	Dispatcher m_dispatcher;
	bool m_threadStarted;
};


/*============================ BackgroundExecutor ==========================*/

BackgroundExecutor::BackgroundExecutor()
:	m_ptrImpl(new Impl(*this))
{
}

BackgroundExecutor::~BackgroundExecutor()
{
}

void
BackgroundExecutor::shutdown()
{
	m_ptrImpl.reset();
}

void
BackgroundExecutor::enqueueTask(TaskPtr const& task)
{
	if (m_ptrImpl.get()) {
		m_ptrImpl->enqueueTask(task);
	}
}


/*===================== BackgroundExecutor::Dispatcher =====================*/

BackgroundExecutor::Dispatcher::Dispatcher(Impl& owner)
:	m_rOwner(owner)
{
}

void
BackgroundExecutor::Dispatcher::customEvent(QEvent* event)
{
	try {
		TaskEvent* evt = dynamic_cast<TaskEvent*>(event);
		assert(evt);

		TaskPtr const& task = evt->payload();
		assert(task);

		TaskResultPtr const result((*task)());
		if (result) {
			QCoreApplication::postEvent(
				&m_rOwner, new ResultEvent(result)
			);
		}
	} catch (std::bad_alloc const&) {
		OutOfMemoryHandler::instance().handleOutOfMemorySituation();
	}
}


/*======================= BackgroundExecutor::Impl =========================*/

BackgroundExecutor::Impl::Impl(BackgroundExecutor& owner)
:	m_rOwner(owner),
	m_dispatcher(*this),
	m_threadStarted(false)
{
	m_dispatcher.moveToThread(this);
}

BackgroundExecutor::Impl::~Impl()
{
	exit();
	wait();
}

void
BackgroundExecutor::Impl::enqueueTask(TaskPtr const& task)
{
	QCoreApplication::postEvent(&m_dispatcher, new TaskEvent(task));
	if (!m_threadStarted) {
		start();
		m_threadStarted = true;
	}
}

void
BackgroundExecutor::Impl::run()
{
	exec();
}

void
BackgroundExecutor::Impl::customEvent(QEvent* event)
{
	ResultEvent* evt = dynamic_cast<ResultEvent*>(event);
	assert(evt);
	
	TaskResultPtr const& result = evt->payload();
	assert(result);
	
	(*result)();
}
