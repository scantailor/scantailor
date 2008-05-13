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

#ifndef WORKERTHREAD_H_
#define WORKERTHREAD_H_

#include "BackgroundTask.h"
#include "FilterResult.h"
#include <QThread>

class WorkerThread : public QThread
{
	Q_OBJECT
public:
	WorkerThread(QObject* parent);
public slots:
	void executeCommand(BackgroundTaskPtr const& task);
signals:
	void ready();
	
	void executeCommandSignal(BackgroundTaskPtr const& task);
	
	void finished(BackgroundTaskPtr const& task, FilterResultPtr const& result);
protected:
	virtual void run();
};

#endif
