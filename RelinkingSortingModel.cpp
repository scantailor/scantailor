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

#include "RelinkingSortingModel.h"
#include "RelinkingModel.h"
#include "RelinkablePath.h"
#include <QString>
#include <assert.h>

RelinkingSortingModel::RelinkingSortingModel(QObject* parent)
:	QSortFilterProxyModel(parent)
{
	setDynamicSortFilter(true);
	sort(0);
}

bool
RelinkingSortingModel::lessThan(QModelIndex const& left, QModelIndex const& right) const
{
	int const left_status = left.data(RelinkingModel::UncommittedStatusRole).toInt();
	int const right_status = right.data(RelinkingModel::UncommittedStatusRole).toInt();
	if (left_status != right_status) {
		if (left_status == RelinkingModel::Missing) {
			return true; // Missing items go before other ones.
		} else if (right_status == RelinkingModel::Missing) {
			return false;
		} else if (left_status == RelinkingModel::StatusUpdatePending) {
			return true; // These go after missing ones.
		} else if (right_status == RelinkingModel::StatusUpdatePending) {
			return false;
		}
		assert(!"Unreachable");
	}

	int const left_type = left.data(RelinkingModel::TypeRole).toInt();
	int const right_type = right.data(RelinkingModel::TypeRole).toInt();
	QString const left_path(left.data(RelinkingModel::UncommittedPathRole).toString());
	QString const right_path(right.data(RelinkingModel::UncommittedPathRole).toString());

	if (left_type != right_type) {
		// Directories go above their siblings.
		QString const left_dir(left_path.left(left_path.lastIndexOf(QChar('/'))));
		QString const right_dir(right_path.left(right_path.lastIndexOf(QChar('/'))));
		if (left_dir == right_dir) {
			return left_type == RelinkablePath::Dir;
		}
	}

	// The last resort is lexicographical ordering.
	return left_path < right_path;
}
