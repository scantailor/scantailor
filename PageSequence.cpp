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

#include "PageSequence.h"
#include <boost/foreach.hpp>

void
PageSequence::append(PageInfo const& page_info)
{
	m_pages.push_back(page_info);	
}

PageInfo const&
PageSequence::pageAt(size_t const idx) const
{
	return m_pages.at(idx); // may throw
}

std::set<PageId>
PageSequence::selectAll() const
{
	std::set<PageId> selection;

	BOOST_FOREACH(PageInfo const& page_info, m_pages) {
		selection.insert(page_info.id());
	}

	return selection;
}

std::set<PageId>
PageSequence::selectPagePlusFollowers(PageId const& page) const
{
	std::set<PageId> selection;
	
	std::vector<PageInfo>::const_iterator it(m_pages.begin());
	std::vector<PageInfo>::const_iterator const end(m_pages.end());
	for (; it != end && it->id() != page; ++it) {
		// Continue until we have a match.
	}
	for (; it != end; ++it) {
		selection.insert(it->id());
	}

	return selection;
}

std::set<PageId>
PageSequence::selectEveryOther(PageId const& base) const
{
	std::set<PageId> selection;
	
	std::vector<PageInfo>::const_iterator it(m_pages.begin());
	std::vector<PageInfo>::const_iterator const end(m_pages.end());
	for (; it != end && it->id() != base; ++it) {
		// Continue until we have a match.
	}
	if (it == end) {
		return selection;
	}

	int const base_idx = it - m_pages.begin();
	int idx = 0;
	BOOST_FOREACH(PageInfo const& page_info, m_pages) {
		if (((idx - base_idx) & 1) == 0) {
			selection.insert(page_info.id());
		}
		++idx;
	}

	return selection;
}
