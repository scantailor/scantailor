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

#include "Settings.h"
#include "Utils.h"
#include <QMutexLocker>

namespace select_content
{

Settings::Settings()
{
}

Settings::~Settings()
{
}

void
Settings::clear()
{
	QMutexLocker locker(&m_mutex);
	m_pageParams.clear();
}

void
Settings::setPageParams(PageId const& page_id, Params const& params)
{
	QMutexLocker locker(&m_mutex);
	Utils::mapSetValue(m_pageParams, page_id, params);
}

void
Settings::clearPageParams(PageId const& page_id)
{
	QMutexLocker locker(&m_mutex);
	m_pageParams.erase(page_id);
}

std::auto_ptr<Params>
Settings::getPageParams(PageId const& page_id) const
{
	QMutexLocker locker(&m_mutex);
	
	PageParams::const_iterator const it(m_pageParams.find(page_id));
	if (it != m_pageParams.end()) {
		return std::auto_ptr<Params>(new Params(it->second));
	} else {
		return std::auto_ptr<Params>();
	}
}

} // namespace select_content
