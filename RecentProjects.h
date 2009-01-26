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

#ifndef RECENT_PROJECTS_H_
#define RECENT_PROJECTS_H_

#include <QString>
#include <list>
#include <limits>

class RecentProjects
{
public:
	/**
	 * \brief The default value for max_items parameters of
	 *        write() and enumerate().
	 */
	enum { DEFAULT_MAX_ITEMS = 7 };
	
	/**
	 * \brief Reads the list of recent projects from QSettings
	 *        without validating them.
	 *
	 * The current list will be overwritten.
	 */
	void read();
	
	/**
	 * \brief Removes non-existing project files.
	 *
	 * \return true if no projects were removed, false otherwise.
	 */
	bool validate();
	
	/**
	 * \brief Appends a project to the list or moves it to the
	 *        top of the list, if it was already there.
	 */
	void setMostRecent(QString const& file_path);
	
	void write(int max_items = DEFAULT_MAX_ITEMS) const;
	
	bool isEmpty() const { return m_projectFiles.empty(); }
	
	/**
	 * \brief Calls out((QString const&)file_path) for every entry.
	 *
	 * Modifying this object from the callback is not allowed.
	 */
	template<typename Out>
	void enumerate(Out out,
		int max_items = DEFAULT_MAX_ITEMS) const;
private:
	std::list<QString> m_projectFiles;
};


template<typename Out>
void
RecentProjects::enumerate(Out out, int max_items) const
{
	std::list<QString>::const_iterator it(m_projectFiles.begin());
	std::list<QString>::const_iterator const end(m_projectFiles.end());
	for (; it != end && max_items > 0; ++it, --max_items) {
		out(*it);
	}
}

#endif
