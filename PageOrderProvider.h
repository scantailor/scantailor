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

#ifndef PAGE_ORDER_PROVIDER_H_
#define PAGE_ORDER_PROVIDER_H_

#include "RefCountable.h"

class PageId;

/**
 * A base class for different page ordering strategies.
 */
class PageOrderProvider : public RefCountable
{
public:
	/**
	 * Returns true if \p lhs_page precedes \p rhs_page.
	 * \p lhs_incomplete and \p rhs_incomplete indicate whether
	 * a page is represented by IncompleteThumbnail.
	 */
	virtual bool precedes(
		PageId const& lhs_page, bool lhs_incomplete,
		PageId const& rhs_page, bool rhs_incomplete) const = 0;
};

#endif
