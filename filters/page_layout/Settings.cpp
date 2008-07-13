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
#include <boost/foreach.hpp>
#include <QMutexLocker>
#include <algorithm>

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

void
Settings::setContentSizeMM(
	PageId const& page_id,
	QSizeF const& content_size_mm, Dependencies const& deps)
{
	QMutexLocker const locker(&m_mutex);
	
	ContentSizePlusDeps const value(content_size_mm, deps);
	Utils::mapSetValue(m_perPageContentSizeMM, page_id, value);
}

QSizeF
Settings::getAggregatePageSizeMM() const
{
	QMutexLocker const locker(&m_mutex);
	
	double max_width = 0.0;
	double max_height = 0.0;
	
	typedef PerPageContentSize::value_type KeyVal;
	BOOST_FOREACH (KeyVal const& kv, m_perPageContentSizeMM) {
		double width = kv.second.contentSizeMM().width();
		double height = kv.second.contentSizeMM().height();
		Margins const* margins;
		
		PerPageMargins::const_iterator const it(
			m_perPageMarginsMM.find(kv.first)
		);
		if (it != m_perPageMarginsMM.end()) {
			margins = &it->second;
		} else {
			margins = &m_defaultMarginsMM;
		}
		
		width += margins->left() + margins->right();
		height += margins->top() + margins->bottom();
		
		max_width = std::max(max_width, width);
		max_height = std::max(max_height, height);
	}
	
	return QSizeF(max_width, max_height);
}

QSizeF
Settings::getAggregatePageSizeMM(
	PageId const& page_id, QSizeF const& content_plus_margins_size_mm)
{
	QMutexLocker const locker(&m_mutex);
	
	double max_width = 0.0;
	double max_height = 0.0;
	
	typedef PerPageContentSize::value_type KeyVal;
	BOOST_FOREACH (KeyVal const& kv, m_perPageContentSizeMM) {
		double width;
		double height;
		
		if (kv.first == page_id) {
			width = content_plus_margins_size_mm.width();
			height = content_plus_margins_size_mm.height();
		} else {
			width = kv.second.contentSizeMM().width();
			height = kv.second.contentSizeMM().height();
			
			Margins const* margins;
			
			PerPageMargins::const_iterator const it(
				m_perPageMarginsMM.find(kv.first)
			);
			if (it != m_perPageMarginsMM.end()) {
				margins = &it->second;
			} else {
				margins = &m_defaultMarginsMM;
			}
			
			width += margins->left() + margins->right();
			height += margins->top() + margins->bottom();
		}
		
		max_width = std::max(max_width, width);
		max_height = std::max(max_height, height);
	}
	
	return QSizeF(max_width, max_height);
}

} // namespace page_layout
