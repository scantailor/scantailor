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
#include <QMutexLocker>
#include <boost/foreach.hpp>
#include <assert.h>

namespace page_split
{

Settings::Settings()
:	m_defaultLayoutType(AUTO_LAYOUT_TYPE)
{
}

Settings::~Settings()
{
}

void
Settings::clear()
{
	QMutexLocker locker(&m_mutex);
	
	m_perPageRecords.clear();
	m_defaultLayoutType = AUTO_LAYOUT_TYPE;
}

LayoutType
Settings::defaultLayoutType() const
{
	QMutexLocker locker(&m_mutex);
	return m_defaultLayoutType;
}

void
Settings::setLayoutTypeForAllPages(LayoutType const layout_type)
{
	QMutexLocker locker(&m_mutex);
	
	PerPageRecords::iterator it(m_perPageRecords.begin());
	PerPageRecords::iterator const end(m_perPageRecords.end());
	while (it != end) {
		if (it->second.hasLayoutTypeConflict(layout_type)) {
			m_perPageRecords.erase(it++);
		} else {
			++it;
		}
	}
	
	m_defaultLayoutType = layout_type;
}

void
Settings::setLayoutTypeFor(LayoutType const layout_type, std::set<PageId> const& pages)
{
	QMutexLocker locker(&m_mutex);
	
	UpdateAction action;
	action.setLayoutType(layout_type);
	
	BOOST_FOREACH(PageId const& page_id, pages) {
		updatePageLocked(page_id.imageId(), action);
	}
}

Settings::Record
Settings::getPageRecord(ImageId const& image_id) const
{
	QMutexLocker locker(&m_mutex);
	
	PerPageRecords::const_iterator it(m_perPageRecords.find(image_id));
	if (it == m_perPageRecords.end()) {
		return Record(m_defaultLayoutType);
	} else {
		return Record(it->second, m_defaultLayoutType);
	}
}

void
Settings::updatePage(ImageId const& image_id, UpdateAction const& action)
{
	QMutexLocker locker(&m_mutex);
	updatePageLocked(image_id, action);
}

void
Settings::updatePageLocked(ImageId const& image_id, UpdateAction const& action)
{
	PerPageRecords::iterator it(m_perPageRecords.lower_bound(image_id));
	if (it == m_perPageRecords.end() ||
			m_perPageRecords.key_comp()(image_id, it->first)) {
		// No record exists for this page.
		
		Record record(m_defaultLayoutType);
		record.update(action);
		
		if (record.hasLayoutTypeConflict()) {
			record.clearParams();
		}
		
		if (!record.isNull()) {
			m_perPageRecords.insert(
				it, PerPageRecords::value_type(image_id, record)
			);
		}
	} else {
		// A record was found.
		
		Record record(it->second, m_defaultLayoutType);
		record.update(action);
		
		if (record.hasLayoutTypeConflict()) {
			record.clearParams();
		}
		
		if (record.isNull()) {
			m_perPageRecords.erase(it);
		} else {
			it->second = record;
		}
	}
}

Settings::Record
Settings::conditionalUpdate(
	ImageId const& image_id, UpdateAction const& action, bool* conflict)
{
	QMutexLocker locker(&m_mutex);
	
	PerPageRecords::iterator it(m_perPageRecords.lower_bound(image_id));
	if (it == m_perPageRecords.end() ||
			m_perPageRecords.key_comp()(image_id, it->first)) {
		// No record exists for this page.
		
		Record record(m_defaultLayoutType);
		record.update(action);
		
		if (record.hasLayoutTypeConflict()) {
			if (conflict) {
				*conflict = true;
			}
			return Record(m_defaultLayoutType);
		}
		
		if (!record.isNull()) {
			m_perPageRecords.insert(
				it, PerPageRecords::value_type(image_id, record)
			);
		}
		
		if (conflict) {
			*conflict = false;
		}
		return record;
	} else {
		// A record was found.
		
		Record record(it->second, m_defaultLayoutType);
		record.update(action);
		
		if (record.hasLayoutTypeConflict()) {
			if (conflict) {
				*conflict = true;
			}
			return Record(m_defaultLayoutType);
		}
		
		if (record.isNull()) {
			m_perPageRecords.erase(it);
		} else {
			it->second = record;
		}
		
		if (conflict) {
			*conflict = false;
		}
		return record;
	}
}


/*======================= Settings::BaseRecord ======================*/

Settings::BaseRecord::BaseRecord()
:	m_params(PageLayout(), Dependencies(), MODE_AUTO),
	m_layoutType(AUTO_LAYOUT_TYPE),
	m_paramsValid(false),
	m_layoutTypeValid(false)
{
}

void
Settings::BaseRecord::setParams(Params const& params)
{
	m_params = params;
	m_paramsValid = true;
}

void
Settings::BaseRecord::setLayoutType(LayoutType const layout_type)
{
	m_layoutType = layout_type;
	m_layoutTypeValid = true;
}

bool
Settings::BaseRecord::hasLayoutTypeConflict(
	LayoutType const default_layout_type) const
{
	if (!m_paramsValid) {
		// No data - no conflict.
		return false;
	}
	
	LayoutType layout_type = default_layout_type;
	if (m_layoutTypeValid) {
		layout_type = m_layoutType;
	}
	
	if (layout_type == AUTO_LAYOUT_TYPE) {
		// This one is compatible with everything.
		return false;
	}
	
	switch (m_params.pageLayout().type()) {
		case PageLayout::SINGLE_PAGE_UNCUT:
			return layout_type != SINGLE_PAGE_UNCUT;
		case PageLayout::LEFT_PAGE_PLUS_OFFCUT:
		case PageLayout::RIGHT_PAGE_PLUS_OFFCUT:
			return layout_type != PAGE_PLUS_OFFCUT;
		case PageLayout::TWO_PAGES:
			return layout_type != TWO_PAGES;
	}
	
	assert(!"Unreachable");
	return false;
}


/*========================= Settings::Record ========================*/

Settings::Record::Record(LayoutType const default_layout_type)
:	m_defaultLayoutType(default_layout_type)
{
}
	
Settings::Record::Record(
	BaseRecord const& base_record,
	LayoutType const default_layout_type)
:	BaseRecord(base_record),
	m_defaultLayoutType(default_layout_type)
{
}

LayoutType
Settings::Record::combinedLayoutType() const
{
	return m_layoutTypeValid ? m_layoutType : m_defaultLayoutType;
}

void
Settings::Record::update(UpdateAction const& action)
{
	switch (action.m_layoutTypeAction) {
		case UpdateAction::SET:
			setLayoutType(action.m_layoutType);
			break;
		case UpdateAction::CLEAR:
			clearLayoutType();
			break;
		case UpdateAction::DONT_TOUCH:
			break;
	}
	
	switch (action.m_paramsAction) {
		case UpdateAction::SET:
			setParams(action.m_params);
			break;
		case UpdateAction::CLEAR:
			clearParams();
			break;
		case UpdateAction::DONT_TOUCH:
			break;
	}
}

bool
Settings::Record::hasLayoutTypeConflict() const
{
	return BaseRecord::hasLayoutTypeConflict(m_defaultLayoutType);
}


/*======================= Settings::UpdateAction ======================*/

void
Settings::UpdateAction::setLayoutType(LayoutType const layout_type)
{
	m_layoutType = layout_type;
	m_layoutTypeAction = SET;
}

void
Settings::UpdateAction::clearLayoutType()
{
	m_layoutTypeAction = CLEAR;
}

void
Settings::UpdateAction::setParams(Params const& params)
{
	m_params = params;
	m_paramsAction = SET;
}

void
Settings::UpdateAction::clearParams()
{
	m_paramsAction = CLEAR;
}

} // namespace page_split
