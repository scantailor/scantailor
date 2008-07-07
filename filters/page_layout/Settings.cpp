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

namespace page_layout
{

Settings::Settings()
{
	m_defaultMarginsMM.setTop(10.0);
	m_defaultMarginsMM.setBottom(10.0);
	m_defaultMarginsMM.setLeft(10.0);
	m_defaultMarginsMM.setRight(10.0);
}

Settings::~Settings()
{
}

Margins
Settings::getPageMarginsMM(PageId const& page_id) const
{
	QMutexLocker const locker(&m_mutex);
	PerPageMargins::const_iterator it(m_perPageMarginsMM.find(page_id));
	if (it != m_perPageMarginsMM.end()) {
		return it->second;
	}
	return m_defaultMarginsMM;
}

void
Settings::setPageMarginsMM(PageId const& page_id, Margins const& margins)
{
	QMutexLocker const locker(&m_mutex);
	Utils::mapSetValue(m_perPageMarginsMM, page_id, margins);
}

} // namespace page_layout
