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
#include "RelinkablePath.h"
#include "AbstractRelinker.h"
#include <QMutexLocker>
#ifndef Q_MOC_RUN
#include <boost/foreach.hpp>
#endif
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

void
Settings::performRelinking(AbstractRelinker const& relinker)
{
	QMutexLocker locker(&m_mutex);
	PerPageRecords new_records;

	BOOST_FOREACH(PerPageRecords::value_type const& kv, m_perPageRecords) {
		RelinkablePath const old_path(kv.first.filePath(), RelinkablePath::File);
		ImageId new_image_id(kv.first);
		new_image_id.setFilePath(relinker.substitutionPathFor(old_path));
		new_records.insert(PerPageRecords::value_type(new_image_id, kv.second));
	}

	m_perPageRecords.swap(new_records);
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
			it->second.clearLayoutType();
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
	return getPageRecordLocked(image_id);
}

Settings::Record
Settings::getPageRecordLocked(ImageId const& image_id) const
{
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
		updatePageLocked(it, action);
	}
}

void
Settings::updatePageLocked(PerPageRecords::iterator const it, UpdateAction const& action)
{
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
			return Record(it->second, m_defaultLayoutType);
		}

		if (conflict) {
			*conflict = false;
		}
		
		if (record.isNull()) {
			m_perPageRecords.erase(it);
			return Record(m_defaultLayoutType);
		} else {
			it->second = record;
			return record;
		}
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
Settings::BaseRecord::hasLayoutTypeConflict(LayoutType const layout_type) const
{
	if (!m_paramsValid) {
		// No data - no conflict.
		return false;
	}
	
	if (layout_type == AUTO_LAYOUT_TYPE) {
		// This one is compatible with everything.
		return false;
	}
	
	switch (m_params.pageLayout().type()) {
		case PageLayout::SINGLE_PAGE_UNCUT:
			return layout_type != SINGLE_PAGE_UNCUT;
		case PageLayout::SINGLE_PAGE_CUT:
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
	return BaseRecord::hasLayoutTypeConflict(combinedLayoutType());
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
