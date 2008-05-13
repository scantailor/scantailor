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

namespace page_split
{

Settings::Settings()
:	m_defaultLayoutType(Rule::AUTO_DETECT)
{
}

Settings::~Settings()
{
}

void
Settings::clear()
{
	QMutexLocker locker(&m_mutex);
	
	m_perPageLayoutType.clear();
	m_perPageParams.clear();
	m_defaultLayoutType = Rule::AUTO_DETECT;
}

void
Settings::applyToPage(
	ImageId const& image_id, Rule::LayoutType const layout_type)
{
	QMutexLocker locker(&m_mutex);
	Utils::mapSetValue(m_perPageLayoutType, image_id, layout_type);
}

void
Settings::applyToAllPages(Rule::LayoutType const layout_type)
{
	QMutexLocker locker(&m_mutex);
	
	m_perPageLayoutType.clear();
	m_defaultLayoutType = layout_type;
}

Rule
Settings::getRuleFor(ImageId const& image_id) const
{
	QMutexLocker locker(&m_mutex);
	
	PerPageLayoutType::const_iterator it(m_perPageLayoutType.find(image_id));
	if (it != m_perPageLayoutType.end()) {
		return Rule(it->second, Rule::THIS_PAGE_ONLY);
	} else {
		return Rule(m_defaultLayoutType, Rule::ALL_PAGES);
	}
}

Rule::LayoutType
Settings::defaultLayoutType() const
{
	QMutexLocker locker(&m_mutex);
	return m_defaultLayoutType;
}

void
Settings::setPageParams(
	ImageId const& image_id, Params const& params)
{
	QMutexLocker locker(&m_mutex);
	Utils::mapSetValue(m_perPageParams, image_id, params);
}

void
Settings::clearPageParams(ImageId const& image_id)
{
	QMutexLocker locker(&m_mutex);
	m_perPageParams.erase(image_id);
}

std::auto_ptr<Params>
Settings::getPageParams(ImageId const& image_id) const
{
	QMutexLocker locker(&m_mutex);
	
	PerPageParams::const_iterator it(m_perPageParams.find(image_id));
	if (it != m_perPageParams.end()) {
		return std::auto_ptr<Params>(new Params(it->second));
	} else {
		return std::auto_ptr<Params>();
	}
}

} // namespace page_split
