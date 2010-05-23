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

#ifndef SELECTED_PAGE_H_
#define SELECTED_PAGE_H_

#include "PageId.h"
#include "PageView.h"

/**
 * The whole point of this class can be demonstrated with a few lines of code:
 * \code
 * ImageId image_id = ...;
 * SelectedPage page;
 * page.set(PageId(image_id, PageId::RIGHT_PAGE), PAGE_VIEW);
 * page.set(PageId(image_id, PageId::SINGLE_PAGE), IMAGE_VIEW);
 * page.get(PAGE_VIEW); // Returns a RIGHT_PAGE PageId.
 * \endcode
 * As seen above, this class remembers the sub-page as long as image id
 * stays the same.  Note that set(..., PAGE_VIEW) will always overwrite
 * the sub-page, while get(IMAGE_VIEW) will always return SINGLE_PAGE sub-pages.
 */
class SelectedPage
{
public:
	SelectedPage() {}

	SelectedPage(PageId const& page_id, PageView view);

	bool isNull() const { return m_pageId.isNull(); }

	void set(PageId const& page_id, PageView view);

	PageId get(PageView view) const;
private:
	PageId m_pageId;
};

#endif
