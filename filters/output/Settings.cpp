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
#include "Dpi.h"
#include "Utils.h"
#include <boost/foreach.hpp>
#include <QMutexLocker>

namespace output
{

Settings::Settings()
:	m_defaultParams(initialDefaultParams())
{
}

Settings::~Settings()
{
}

void
Settings::clear()
{
	QMutexLocker const locker(&m_mutex);
	
	m_defaultParams = initialDefaultParams();
	m_perPageParams.clear();
}

Params
Settings::getPageParams(PageId const& page_id) const
{
	QMutexLocker const locker(&m_mutex);
	
	PerPageParams::const_iterator const it(m_perPageParams.find(page_id));
	if (it != m_perPageParams.end()) {
		return it->second;
	} else {
		return m_defaultParams;
	}
}

void
Settings::setPageParams(PageId const& page_id, Params const& params)
{
	QMutexLocker const locker(&m_mutex);
	Utils::mapSetValue(m_perPageParams, page_id, params);
}

Dpi
Settings::getDpi(PageId const& page_id) const
{
	QMutexLocker const locker(&m_mutex);
	
	PerPageParams::const_iterator const it(m_perPageParams.find(page_id));
	if (it != m_perPageParams.end()) {
		return it->second.dpi();
	} else {
		return m_defaultParams.dpi();
	}
}

void
Settings::setDpi(PageId const& page_id, Dpi const& dpi)
{
	QMutexLocker const locker(&m_mutex);
	
	PerPageParams::iterator const it(m_perPageParams.lower_bound(page_id));
	if (it == m_perPageParams.end() || page_id < it->first) {
		m_perPageParams.insert(
			it, PerPageParams::value_type(page_id, Params(dpi))
		);
	} else {
		it->second.setDpi(dpi);
	}
}

void
Settings::setDpiForAllPages(Dpi const& dpi)
{
	QMutexLocker const locker(&m_mutex);
	
	typedef PerPageParams::value_type KeyVal;
	BOOST_FOREACH (KeyVal& kv, m_perPageParams) {
		kv.second.setDpi(dpi);
	}
	m_defaultParams.setDpi(dpi);
}

Params
Settings::initialDefaultParams()
{
	return Params(Dpi(300, 300));
}

} // namespace output
