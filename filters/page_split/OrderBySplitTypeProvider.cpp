/*
    Scan Tailor - Interactive post-processing tool for scanned pages.
    Copyright (C)  Joseph Artsimovich <joseph.artsimovich@gmail.com>
    Copyright (C)  Vadim Kuznetsov ()DikBSD <dikbsd@gmail.com>

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

#include "OrderBySplitTypeProvider.h"
#include "Params.h"
#include "PageLayout.h"
#include <QSizeF>
#include <memory>
#include <assert.h>

namespace page_split
{

OrderBySplitTypeProvider::OrderBySplitTypeProvider(IntrusivePtr<Settings> const& settings)
:	m_ptrSettings(settings)
{
}

bool
OrderBySplitTypeProvider::precedes(
	PageId const& lhs_page, bool const lhs_incomplete,
	PageId const& rhs_page, bool const rhs_incomplete) const
{
	if (lhs_incomplete != rhs_incomplete) {
		// Pages with question mark go to the bottom.
		return rhs_incomplete;
	} else if (lhs_incomplete) {
		assert(rhs_incomplete);
		// Two pages with question marks are ordered naturally.
		return lhs_page < rhs_page;
	}

	assert(lhs_incomplete == false);
	assert(rhs_incomplete == false);

	Settings::Record const lhs_record(m_ptrSettings->getPageRecord(lhs_page.imageId()));
	Settings::Record const rhs_record(m_ptrSettings->getPageRecord(rhs_page.imageId()));

	Params const* lhs_params = lhs_record.params();
	Params const* rhs_params = rhs_record.params();

	int lhs_layout_type = lhs_record.combinedLayoutType();
	if (lhs_params) {
		lhs_layout_type = lhs_params->pageLayout().toLayoutType();
	}
	if (lhs_layout_type == AUTO_LAYOUT_TYPE) {
		lhs_layout_type = 100; // To force it below pages with known layout.
	}

	int rhs_layout_type = rhs_record.combinedLayoutType();
	if (rhs_params) {
		rhs_layout_type = rhs_params->pageLayout().toLayoutType();
	}
	if (rhs_layout_type == AUTO_LAYOUT_TYPE) {
		rhs_layout_type = 100; // To force it below pages with known layout.
	}

	if (lhs_layout_type == rhs_layout_type) {
		return lhs_page < rhs_page;
	} else {
		return lhs_layout_type < rhs_layout_type;
	}
}

} // namespace page_split
