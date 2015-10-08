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

#include "Settings.h"
#include "Utils.h"
#include "RelinkablePath.h"
#include "AbstractRelinker.h"
#include <QMutexLocker>
#ifndef Q_MOC_RUN
#include <boost/foreach.hpp>
#endif

#include <cmath>
#include <iostream>
#include "CommandLine.h" 

namespace select_content
{

Settings::Settings() :
    m_avg(0.0),
    m_sigma(0.0),
    m_pageDetectionBox(0.0, 0.0),
    m_pageDetectionTolerance(0.1)
{
    m_maxDeviation = CommandLine::get().getContentDeviation();
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
Settings::performRelinking(AbstractRelinker const& relinker)
{
	QMutexLocker locker(&m_mutex);
	PageParams new_params;

	BOOST_FOREACH(PageParams::value_type const& kv, m_pageParams) {
		RelinkablePath const old_path(kv.first.imageId().filePath(), RelinkablePath::File);
		PageId new_page_id(kv.first);
		new_page_id.imageId().setFilePath(relinker.substitutionPathFor(old_path));
		new_params.insert(PageParams::value_type(new_page_id, kv.second));
	}

    m_pageParams.swap(new_params);
}

void Settings::updateDeviation()
{
    m_avg = 0.0;
    BOOST_FOREACH(PageParams::value_type & kv, m_pageParams) {
		kv.second.computeDeviation(0.0);
		m_avg += -1 * kv.second.deviation();
    }
    m_avg = m_avg / double(m_pageParams.size());
#ifdef DEBUG
    std::cout << "avg_content = " << m_avg << std::endl;
#endif

    double sigma2 = 0.0;
    BOOST_FOREACH(PageParams::value_type & kv, m_pageParams) {
        kv.second.computeDeviation(m_avg);
        sigma2 += kv.second.deviation() * kv.second.deviation();
    }
    sigma2 = sigma2 / double(m_pageParams.size());
    m_sigma = sqrt(sigma2);
#if DEBUG
    std::cout << "sigma2 = " << sigma2 << std::endl;
    std::cout << "sigma = " << m_sigma << std::endl;
#endif
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
