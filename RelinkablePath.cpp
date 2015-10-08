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

#include "RelinkablePath.h"
#include <QStringList>
#ifndef Q_MOC_RUN
#include <boost/foreach.hpp>
#endif

RelinkablePath::RelinkablePath(QString const& path, Type type)
:	m_normalizedPath(normalize(path))
,	m_type(type)
{
}

QString
RelinkablePath::normalize(QString const& path)
{
	QString front_slashes(path);
	front_slashes.replace(QChar('\\'), QLatin1String("/"));
	
	QStringList new_components;
	BOOST_FOREACH(QString const& comp, front_slashes.split(QChar('/'), QString::KeepEmptyParts)) {
		if (comp.isEmpty()) {
			if (new_components.isEmpty()
#if _WIN32
				|| (new_components.size() == 1 && new_components.front().isEmpty())
#endif
			) {
				// This will create a leading slash or maybe two (for "\\host\share" type of paths).
				new_components.push_back(comp);
			} else {
				// This will get rid of redundant slashes, including the trailing slash.
				continue;
			}
		} else if (comp == ".") {
			continue;
		} else if (comp == "..") {
			if (new_components.isEmpty()) {
				return QString(); // Error.
			}
			QString const& last_comp = new_components.back();
			if (last_comp.isEmpty() || last_comp.endsWith(QChar(':'))) {
				return QString(); // Error.
			}
			new_components.pop_back();
		} else {
			new_components.push_back(comp);
		}
	}

	return new_components.join(QChar('/'));
}
