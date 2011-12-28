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

#ifndef RELINKING_MODEL_H_
#define RELINKING_MODEL_H_

#include "NonCopyable.h"
#include "VirtualFunction.h"
#include "RelinkablePath.h"
#include "AbstractRelinker.h"
#include "IntrusivePtr.h"
#include <QString>
#include <QModelIndex>
#include <QAbstractListModel>
#include <QPixmap>
#include <QThread>
#include <Qt>
#include <memory>
#include <vector>
#include <set>
#include <map>

class RelinkingModel :
	public QAbstractListModel,
	public VirtualFunction1<void, RelinkablePath const&>
{
	DECLARE_NON_COPYABLE(RelinkingModel)
public:
	enum Status { Exists, Missing, StatusUpdatePending };

	enum {
		TypeRole = Qt::UserRole,
		UncommittedPathRole,
		UncommittedStatusRole
	};

	RelinkingModel();

	virtual ~RelinkingModel();
	
	virtual int rowCount(QModelIndex const& parent = QModelIndex()) const;
	
	virtual QVariant data(QModelIndex const& index, int role = Qt::DisplayRole) const;

	/**
	 * This method guarantees that
	 * \code
	 * model.relinker().get() == model.relinker().get()
	 * \endcode
	 * will hold true for the lifetime of RelinkingModel object.
	 * This allows you to take the relinker right after construction
	 * and then use it when accepted() signal is emitted.
	 */
	IntrusivePtr<AbstractRelinker> relinker() const { return m_ptrRelinker; }

	virtual void operator()(RelinkablePath const& path) { addPath(path); }

	void addPath(RelinkablePath const& path);

	void replacePrefix(QString const& prefix, QString const& replacement, RelinkablePath::Type type);

	/**
	 * Returns true if we have different original paths remapped to the same one.
	 * Checking is done on uncommitted paths.
	 */
	bool checkForMerges() const;

	void commitChanges();

	void rollbackChanges();

	void requestStatusUpdate(QModelIndex const& index);
protected:
	virtual void customEvent(QEvent* event);
private:
	class StatusUpdateThread;
	class StatusUpdateResponse;

	/** Stands for File System Object (file or directory). */
	struct Item
	{
		QString origPath; /**< That's the path passed through addPath(). It never changes. */
		QString committedPath;
		QString uncommittedPath; /**< Same as committedPath when m_haveUncommittedChanges == false. */
		RelinkablePath::Type type;
		Status committedStatus;
		Status uncommittedStatus; /**< Same as committedStatus when m_haveUncommittedChanges == false. */

		Item(RelinkablePath const& path);
	};

	class Relinker : public AbstractRelinker
	{
	public:
		void addMapping(QString const& from, QString const& to);

		/** Returns path.normalizedPath() if there is no mapping for it. */
		virtual QString substitutionPathFor(RelinkablePath const& path) const;

		void swap(Relinker& other);
	private:
		std::map<QString, QString> m_mappings;
	};

	static void ensureEndsWithSlash(QString& str);

	QPixmap m_fileIcon;
	QPixmap m_folderIcon;
	std::vector<Item> m_items;
	std::set<QString> m_origPathSet;
	IntrusivePtr<Relinker> const m_ptrRelinker;
	std::auto_ptr<StatusUpdateThread> m_ptrStatusUpdateThread;
	bool m_haveUncommittedChanges;
};

#endif
