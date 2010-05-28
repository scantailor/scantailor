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

#include "ThreadPriority.h"
#include <QSettings>
#include <assert.h>

QThread::Priority
ThreadPriority::toQThreadPriority() const
{
	switch (m_prio) {
		case Normal:
			return QThread::NormalPriority;
		case Low:
			return QThread::LowPriority;
		case Lowest:
			return QThread::LowestPriority;
		case Idle:
			return QThread::IdlePriority;
	}

	assert(!"Unreachable");
	return QThread::NormalPriority;
}

int
ThreadPriority::toPosixNiceLevel() const
{
	switch (m_prio) {
		case Normal:
			return 0;
		case Low:
			return 6;
		case Lowest:
			return 12;
		case Idle:
			return 19;
	}

	assert(!"Unreachable");
	return 0;
}

ThreadPriority
ThreadPriority::load(
	QSettings const& settings, QString const& key, Priority dflt)
{
	QString const str(settings.value(key).toString());
	if (str == "normal") {
		return Normal;
	} else if (str == "low") {
		return Low;
	} else if (str == "lowest") {
		return Lowest;
	} else if (str == "idle") {
		return Idle;
	} else {
		return dflt;
	}
}

ThreadPriority
ThreadPriority::load(QString const& key, Priority dflt)
{
	QSettings settings;
	return load(settings, key, dflt);
}

void
ThreadPriority::save(QSettings& settings, QString const& key)
{
	char const* str = "";
	switch (m_prio) {
		case Normal:
			str = "normal";
			break;
		case Low:
			str = "low";
			break;
		case Lowest:
			str = "lowest";
			break;
		case Idle:
			str = "idle";
			break;
	}

	settings.setValue(key, QString::fromAscii(str));
}

void
ThreadPriority::save(QString const& key)
{
	QSettings settings;
	save(settings, key);
}
