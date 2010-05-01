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
#include "PageId.h"
#include "PageInfo.h"

class PageOrderProvider : public RefCountable
{
public:
	class Comparator
	{
		// Member-wise copying is OK.
	public:
		Comparator(IntrusivePtr<PageOrderProvider const> const& provider)
			: m_ptrProvider(provider) {}

		bool operator()(PageInfo const& lhs, PageInfo const& rhs) const {
			return (*this)(lhs.id(), rhs.id());
		}

		bool operator()(PageId const& lhs, PageId const& rhs) const {
			return m_ptrProvider->precedes(lhs, rhs);
		}
	private:
		IntrusivePtr<PageOrderProvider const> m_ptrProvider;
	};

	/**
	 * Returns true if \p lhs precedes \p rhs.
	 */
	virtual bool precedes(PageId const& lhs, PageId const& rhs) const = 0;

	/**
	 * Provides a comparator suitable for sort()-like kinds of functions.
	 */
	Comparator comparator() const {
		return Comparator(IntrusivePtr<PageOrderProvider const>(this));
	}
};

#endif
