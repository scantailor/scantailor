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

#include "SelectedPage.h"

SelectedPage::SelectedPage(PageId const& page_id, PageView view)
{
	set(page_id, view);
}

void
SelectedPage::set(PageId const& page_id, PageView view)
{
	if (view == PAGE_VIEW || page_id.imageId() != m_pageId.imageId()) {
		m_pageId = page_id;
	}
}

PageId
SelectedPage::get(PageView view) const
{
	if (view == PAGE_VIEW) {
		return m_pageId;
	} else {
		return PageId(m_pageId.imageId(), PageId::SINGLE_PAGE);
	}
}
