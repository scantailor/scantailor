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

#include "StageSequence.h"
#include "ProjectPages.h"
#include <boost/foreach.hpp>

StageSequence::StageSequence(IntrusivePtr<ProjectPages> const& pages,
	PageSelectionAccessor const& page_selection_accessor)
:	m_ptrFixOrientationFilter(new fix_orientation::Filter(page_selection_accessor)),
	m_ptrPageSplitFilter(new page_split::Filter(pages, page_selection_accessor)),
	m_ptrDeskewFilter(new deskew::Filter(page_selection_accessor)),
	m_ptrSelectContentFilter(new select_content::Filter(page_selection_accessor)),
	m_ptrPageLayoutFilter(new page_layout::Filter(pages, page_selection_accessor)),
	m_ptrOutputFilter(new output::Filter(page_selection_accessor))
{
	m_fixOrientationFilterIdx = m_filters.size();
	m_filters.push_back(m_ptrFixOrientationFilter);
	
	m_pageSplitFilterIdx = m_filters.size();
	m_filters.push_back(m_ptrPageSplitFilter);
	
	m_deskewFilterIdx = m_filters.size();
	m_filters.push_back(m_ptrDeskewFilter);
	
	m_selectContentFilterIdx = m_filters.size();
	m_filters.push_back(m_ptrSelectContentFilter);
	
	m_pageLayoutFilterIdx = m_filters.size();
	m_filters.push_back(m_ptrPageLayoutFilter);
	
	m_outputFilterIdx = m_filters.size();
	m_filters.push_back(m_ptrOutputFilter);
}

void
StageSequence::performRelinking(AbstractRelinker const& relinker)
{
	BOOST_FOREACH(FilterPtr& filter, m_filters) {
		filter->performRelinking(relinker);
	}
}

int
StageSequence::findFilter(FilterPtr const& filter) const
{
	int idx = 0;
	BOOST_FOREACH(FilterPtr const& f, m_filters) {
		if (f == filter) {
			return idx;
		}
		++idx;
	}
	return -1;
}
