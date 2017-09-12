/*
    Scan Tailor - Interactive post-processing tool for scanned pages.
    Copyright (C) 2007-2009  Joseph Artsimovich <joseph_a@mail.ru>

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

#include "RecentProjects.h"
#ifndef Q_MOC_RUN
#include <boost/foreach.hpp>
#endif
#include <QSettings>
#include <QFile>
#include <algorithm>

void
RecentProjects::read()
{
	QSettings settings;
	std::list<QString> new_list;
	
	int const size = settings.beginReadArray("project/recent");
	for (int i = 0; i < size; ++i) {
		settings.setArrayIndex(i);
		QString const path(settings.value("path").toString());
		new_list.push_back(path);
	}
	settings.endArray();
	
	m_projectFiles.swap(new_list);
}

void
RecentProjects::write(int const max_items) const
{
	QSettings settings;
	settings.beginWriteArray("project/recent");
	int idx = 0;
	BOOST_FOREACH(QString const& path, m_projectFiles) {
		if (idx >= max_items) {
			break;
		}
		settings.setArrayIndex(idx);
		settings.setValue("path", path);
		++idx;
	}
	settings.endArray();
}

bool
RecentProjects::validate()
{
	bool all_ok = true;
	
	std::list<QString>::iterator it(m_projectFiles.begin());
	std::list<QString>::iterator const end(m_projectFiles.end());
	while (it != end) {
		if (QFile::exists(*it)) {
			++it;
		} else {
			m_projectFiles.erase(it++);
			all_ok = false;
		}
	}
	
	return all_ok;
}

void
RecentProjects::setMostRecent(QString const& file_path)
{
	std::list<QString>::iterator const begin(m_projectFiles.begin());
	std::list<QString>::iterator const end(m_projectFiles.end());
	std::list<QString>::iterator it(std::find(begin, end, file_path));
	if (it != end) {
		m_projectFiles.splice(begin, m_projectFiles, it);
	} else {
		m_projectFiles.push_front(file_path);
	}
}
