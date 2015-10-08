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

#include "RelinkingModel.h"
#include "PayloadEvent.h"
#include "OutOfMemoryHandler.h"
#include <QFile>
#include <QDir>
#include <QMutex>
#include <QMutexLocker>
#include <QWaitCondition>
#include <QCoreApplication>
#include <QColor>
#include <Qt>
#ifndef Q_MOC_RUN
#include <boost/foreach.hpp>
#include <boost/multi_index_container.hpp>
#include <boost/multi_index/ordered_index.hpp>
#include <boost/multi_index/sequenced_index.hpp>
#include <boost/multi_index/member.hpp>
#endif
#include <utility>
#include <iterator>
#include <algorithm>
#include <assert.h>

class RelinkingModel::StatusUpdateResponse
{
public:
	StatusUpdateResponse(QString const& path, int row, Status status)
		: m_path(path), m_row(row), m_status(status) {}
	
	QString const& path() const { return m_path; }

	int row() const { return m_row; }

	Status status() const { return m_status; }
private:
	QString m_path;
	int m_row;
	Status m_status;
};


class RelinkingModel::StatusUpdateThread : private QThread
{
public:
	StatusUpdateThread(RelinkingModel* owner);

	/** This will signal the thread to stop and wait for it to happen. */
	virtual ~StatusUpdateThread();

	/**
	 * Requests are served from last to first.
	 * Requesting the same item multiple times will just move the existing
	 * record to the top of the stack.
	 */
	void requestStatusUpdate(QString const& path, int row);
private:
	struct Task
	{
		QString path;
		int row;

		Task(QString const& p, int r) : path(p), row(r) {}
	};

	class OrderedByPathTag;
	class OrderedByPriorityTag;

	typedef boost::multi_index_container<
		Task,
		boost::multi_index::indexed_by<
			boost::multi_index::ordered_unique<
			boost::multi_index::tag<OrderedByPathTag>,
				boost::multi_index::member<Task, QString, &Task::path>
			>,
			boost::multi_index::sequenced<
				boost::multi_index::tag<OrderedByPriorityTag>
			>
		>
	> TaskList;

	typedef TaskList::index<OrderedByPathTag>::type TasksByPath;
	typedef TaskList::index<OrderedByPriorityTag>::type TasksByPriority;

	virtual void run();

	RelinkingModel* m_pOwner;
	TaskList m_tasks;
	TasksByPath& m_rTasksByPath;
	TasksByPriority& m_rTasksByPriority;
	QString m_pathBeingProcessed;
	QMutex m_mutex;
	QWaitCondition m_cond;
	bool m_exiting;
};


/*============================ RelinkingModel =============================*/

RelinkingModel::RelinkingModel()
:	m_fileIcon(":/icons/file-16.png")
,	m_folderIcon(":/icons/folder-16.png")
,	m_ptrRelinker(new Relinker)
,	m_ptrStatusUpdateThread(new StatusUpdateThread(this))
,	m_haveUncommittedChanges(true)
{
}

RelinkingModel::~RelinkingModel()
{
}

int
RelinkingModel::rowCount(QModelIndex const& parent) const
{
	if (!parent.isValid()) {
		return m_items.size();
	} else {
		return 0;
	}
}

QVariant
RelinkingModel::data(QModelIndex const& index, int role) const
{
	Item const& item = m_items[index.row()];
	
	switch (role) {
		case TypeRole:
			return item.type;
		case UncommittedStatusRole:
			return item.uncommittedStatus;
		case UncommittedPathRole:
			return item.uncommittedPath;
		case Qt::DisplayRole:
			if (item.uncommittedPath.startsWith(QChar('/')) &&
				!item.uncommittedPath.startsWith(QLatin1String("//"))) {
				// "//" indicates a network path.
				return item.uncommittedPath;
			} else {
				return QDir::toNativeSeparators(item.uncommittedPath);
			}
		case Qt::DecorationRole:
			return (item.type == RelinkablePath::Dir) ? m_folderIcon : m_fileIcon;
		case Qt::BackgroundColorRole:
			return QColor(Qt::transparent);
	}

	return QVariant();
}

void
RelinkingModel::addPath(RelinkablePath const& path)
{
	QString const normalized_path(path.normalizedPath());

	std::pair<std::set<QString>::iterator, bool> const ins(
		m_origPathSet.insert(path.normalizedPath())
	);
	if (!ins.second) {
		// Not inserted because identical path is already there.
		return;
	}

	beginInsertRows(QModelIndex(), m_items.size(), m_items.size());
	m_items.push_back(Item(path));
	endInsertRows();

	requestStatusUpdate(index(m_items.size() - 1));
}

void
RelinkingModel::replacePrefix(
	QString const& prefix, QString const& replacement, RelinkablePath::Type type)
{
	QString slash_terminated_prefix(prefix);
	ensureEndsWithSlash(slash_terminated_prefix);

	int modified_rowspan_begin = -1;

	int row = -1;
	BOOST_FOREACH(Item& item, m_items) {
		++row;
		bool modified = false;
		
		if (type == RelinkablePath::File) {
			if (item.type == RelinkablePath::File && item.uncommittedPath == prefix) {
				item.uncommittedPath = replacement;
				modified = true;
			}
		} else {
			assert(type == RelinkablePath::Dir);
			if (item.uncommittedPath.startsWith(slash_terminated_prefix)) {
				int const suffix_len = item.uncommittedPath.length() - slash_terminated_prefix.length() + 1;
				item.uncommittedPath = replacement + item.uncommittedPath.right(suffix_len);
				modified = true;
			} else if (item.uncommittedPath == prefix) {
				item.uncommittedPath = replacement;
				modified = true;
			}
		}
		
		if (modified) {
			m_haveUncommittedChanges = true;
			if (modified_rowspan_begin == -1) {
				modified_rowspan_begin = row;
			}
			emit dataChanged(index(modified_rowspan_begin), index(row));
			requestStatusUpdate(index(row)); // This sets item.changedStatus to StatusUpdatePending.
		} else {
			if (modified_rowspan_begin != -1) {
				emit dataChanged(index(modified_rowspan_begin), index(row));
				modified_rowspan_begin = -1;
			}
		}
	}

	if (modified_rowspan_begin != -1) {
		emit dataChanged(index(modified_rowspan_begin), index(row));
	}
}

bool
RelinkingModel::checkForMerges() const
{
	std::vector<QString> new_paths;
	new_paths.reserve(m_items.size());

	BOOST_FOREACH(Item const& item, m_items) {
		new_paths.push_back(item.uncommittedPath);
	}

	std::sort(new_paths.begin(), new_paths.end());
	return std::adjacent_find(new_paths.begin(), new_paths.end()) != new_paths.end();
}

void
RelinkingModel::commitChanges()
{
	if (!m_haveUncommittedChanges) {
		return;
	}

	Relinker new_relinker(*m_ptrRelinker);
	int modified_rowspan_begin = -1;

	int row = -1;
	BOOST_FOREACH(Item& item, m_items) {
		++row;
		
		if (item.committedPath != item.uncommittedPath) {
			item.committedPath = item.uncommittedPath;
			item.committedStatus = item.uncommittedStatus;
			new_relinker.addMapping(item.origPath, item.committedPath);
			if (modified_rowspan_begin == -1) {
				modified_rowspan_begin = row;
			}
		} else {
			if (modified_rowspan_begin != -1) {
				emit dataChanged(index(modified_rowspan_begin), index(row));
				modified_rowspan_begin = -1;
			}
		}
	}

	if (modified_rowspan_begin != -1) {
		emit dataChanged(index(modified_rowspan_begin), index(row));
	}

	m_ptrRelinker->swap(new_relinker);
	m_haveUncommittedChanges = false;
}

void
RelinkingModel::rollbackChanges()
{
	if (!m_haveUncommittedChanges) {
		return;
	}

	int modified_rowspan_begin = -1;

	int row = -1;
	BOOST_FOREACH(Item& item, m_items) {
		++row;
		
		if (item.uncommittedPath != item.committedPath) {
			item.uncommittedPath = item.committedPath;
			item.uncommittedStatus = item.committedStatus;
			if (modified_rowspan_begin == -1) {
				modified_rowspan_begin = row;
			}
		} else {
			if (modified_rowspan_begin != -1) {
				emit dataChanged(index(modified_rowspan_begin), index(row));
				modified_rowspan_begin = -1;
			}
		}
	}

	if (modified_rowspan_begin != -1) {
		emit dataChanged(index(modified_rowspan_begin), index(row));
	}

	m_haveUncommittedChanges = false;
}

void
RelinkingModel::ensureEndsWithSlash(QString& str)
{
	if (!str.endsWith(QChar('/'))) {
		str += QChar('/');
	}
}

void
RelinkingModel::requestStatusUpdate(QModelIndex const& index)
{
	assert(index.isValid());

	Item& item = m_items[index.row()];
	item.uncommittedStatus = StatusUpdatePending;

	m_ptrStatusUpdateThread->requestStatusUpdate(item.uncommittedPath, index.row());
}

void
RelinkingModel::customEvent(QEvent* event)
{
	typedef PayloadEvent<StatusUpdateResponse> ResponseEvent;
	ResponseEvent* evt = dynamic_cast<ResponseEvent*>(event);
	assert(evt);

	StatusUpdateResponse const& response = evt->payload();
	if (response.row() < 0 || response.row() >= int(m_items.size())) {
		return;
	}

	Item& item = m_items[response.row()];
	if (item.uncommittedPath == response.path()) {
		item.uncommittedStatus = response.status();
	}
	if (item.committedPath == response.path()) {
		item.committedStatus = response.status();
	}

	emit dataChanged(index(response.row()), index(response.row()));
}


/*========================== StatusUpdateThread =========================*/

RelinkingModel::StatusUpdateThread::StatusUpdateThread(RelinkingModel* owner)
:	QThread(owner)
,	m_pOwner(owner)
,	m_tasks()
,	m_rTasksByPath(m_tasks.get<OrderedByPathTag>())
,	m_rTasksByPriority(m_tasks.get<OrderedByPriorityTag>())
,	m_exiting(false)
{
}

RelinkingModel::StatusUpdateThread::~StatusUpdateThread()
{
	{
		QMutexLocker locker(&m_mutex);
		m_exiting = true;
	}

	m_cond.wakeAll();
	wait();
}

void
RelinkingModel::StatusUpdateThread::requestStatusUpdate(QString const& path, int row)
{
	QMutexLocker const locker(&m_mutex);
	if (m_exiting) {
		return;
	}

	if (path == m_pathBeingProcessed) {
		// This task is currently in progress.
		return;
	}

	std::pair<TasksByPath::iterator, bool> const ins(
		m_rTasksByPath.insert(Task(path, row))
	);

	// Whether inserted or being already there, move it to the front of priority queue.
	m_rTasksByPriority.relocate(
		m_rTasksByPriority.end(), m_tasks.project<OrderedByPriorityTag>(ins.first)
	);

	if (!isRunning()) {
		start();
	}

	m_cond.wakeOne();
}

void
RelinkingModel::StatusUpdateThread::run()
try {
	QMutexLocker const locker(&m_mutex);

	class MutexUnlocker
	{
	public:
		MutexUnlocker(QMutex* mutex) : m_pMutex(mutex) { mutex->unlock(); }

		~MutexUnlocker() { m_pMutex->lock(); }
	private:
		QMutex* const m_pMutex;
	};

	for (;;) {
		if (m_exiting) {
			break;
		}
		
		if (m_tasks.empty()) {
			m_cond.wait(&m_mutex);
		}

		if (m_tasks.empty()) {
			continue;
		}
		
		Task const task(m_rTasksByPriority.front());
		m_pathBeingProcessed = task.path;
		m_rTasksByPriority.pop_front();

		{
			MutexUnlocker const unlocker(&m_mutex);

			bool const exists = QFile::exists(task.path);
			StatusUpdateResponse const response(task.path, task.row, exists ? Exists : Missing);
			QCoreApplication::postEvent(m_pOwner, new PayloadEvent<StatusUpdateResponse>(response));
		}

		m_pathBeingProcessed.clear();
	}
} catch (std::bad_alloc const&) {
	OutOfMemoryHandler::instance().handleOutOfMemorySituation();
}


/*================================ Item =================================*/

RelinkingModel::Item::Item(RelinkablePath const& path)
:	origPath(path.normalizedPath())
,	committedPath(path.normalizedPath())
,	uncommittedPath(path.normalizedPath())
,	type(path.type())
,	committedStatus(StatusUpdatePending)
,	uncommittedStatus(StatusUpdatePending)
{
}


/*============================== Relinker ================================*/

void
RelinkingModel::Relinker::addMapping(QString const& from, QString const& to)
{
	m_mappings[from] = to;
}

QString
RelinkingModel::Relinker::substitutionPathFor(RelinkablePath const& path) const
{
	std::map<QString, QString>::const_iterator const it(m_mappings.find(path.normalizedPath()));
	if (it != m_mappings.end()) {
		return it->second;
	} else {
		return path.normalizedPath();
	}
}

void
RelinkingModel::Relinker::swap(Relinker& other)
{
	m_mappings.swap(other.m_mappings);
}
