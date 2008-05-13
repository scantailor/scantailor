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
#include <WorkerThreadDispatcher.h>

WorkerThread::WorkerThread(QObject* parent)
:	QThread(parent)
{
}

void
WorkerThread::executeCommand(BackgroundTaskPtr const& task)
{
	emit executeCommandSignal(task);
}

void
WorkerThread::run()
{
	WorkerThreadDispatcher dispatcher;
	connect(
		&dispatcher, SIGNAL(finished(BackgroundTaskPtr const&, FilterResultPtr const&)),
		this, SIGNAL(finished(BackgroundTaskPtr const&, FilterResultPtr const&)),
		Qt::QueuedConnection
	);
	connect(
		this, SIGNAL(executeCommandSignal(BackgroundTaskPtr const&)),
		&dispatcher, SLOT(executeCommand(BackgroundTaskPtr const&)),
		Qt::QueuedConnection
	);
	
	emit ready();
	
	exec();
}
