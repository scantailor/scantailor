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
#include "../../Utils.h"
#include <boost/foreach.hpp>
#include <QMutexLocker>

namespace output
{

Settings::Settings()
:	m_defaultDpi(300, 300)
{
}

Settings::~Settings()
{
}

void
Settings::clear()
{
	QMutexLocker const locker(&m_mutex);
	
	m_defaultDpi = Dpi(300, 300);
	m_defaultColorParams = ColorParams();
	m_perPageDpi.clear();
	m_perPageColorParams.clear();
}

ColorParams
Settings::getColorParams(PageId const& page_id) const
{
	QMutexLocker const locker(&m_mutex);
	
	PerPageColorParams::const_iterator const it(m_perPageColorParams.find(page_id));
	if (it != m_perPageColorParams.end()) {
		return it->second;
	} else {
		return m_defaultColorParams;
	}
}

void
Settings::setColorParams(PageId const& page_id, ColorParams const& params)
{
	QMutexLocker const locker(&m_mutex);
	Utils::mapSetValue(m_perPageColorParams, page_id, params);
}

void
Settings::setColorParamsForAllPages(ColorParams const& params)
{
	QMutexLocker const locker(&m_mutex);
	
	m_defaultColorParams = params;
	m_perPageColorParams.clear();
}

Dpi
Settings::getDpi(PageId const& page_id) const
{
	QMutexLocker const locker(&m_mutex);
	
	PerPageDpi::const_iterator const it(m_perPageDpi.find(page_id));
	if (it != m_perPageDpi.end()) {
		return it->second;
	} else {
		return m_defaultDpi;
	}
}

void
Settings::setDpi(PageId const& page_id, Dpi const& dpi)
{
	QMutexLocker const locker(&m_mutex);
	Utils::mapSetValue(m_perPageDpi, page_id, dpi);
}

void
Settings::setDpiForAllPages(Dpi const& dpi)
{
	QMutexLocker const locker(&m_mutex);
	
	m_defaultDpi = dpi;
	m_perPageDpi.clear();
}

} // namespace output
