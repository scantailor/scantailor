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

#include "OrderBySplitLineProvider.h"
#include "Params.h"
#include <QSizeF>
#include <memory>

namespace page_split
{

OrderBySplitLineProvider::OrderBySplitLineProvider(IntrusivePtr<Settings> const& settings)
:	m_ptrSettings(settings)
{
}

bool
OrderBySplitLineProvider::precedes(
	PageId const& lhs_page, bool const lhs_incomplete,
	PageId const& rhs_page, bool const rhs_incomplete) const
{
	Settings::Record const lhs_record = m_ptrSettings->getPageRecord(lhs_page.imageId());
	Settings::Record const rhs_record = m_ptrSettings->getPageRecord(rhs_page.imageId());

	int lhs(-1);
	if(!lhs_record.isNull()) {
		lhs = lhs_record.splitLineCount();
	}

	int rhs(-1);
	if(!rhs_record.isNull()) {
		rhs = rhs_record.splitLineCount();
	}

	bool const lhs_valid = !lhs_incomplete && lhs!=-1;
	bool const rhs_valid = !rhs_incomplete && rhs!=-1;

	if (lhs_valid != rhs_valid) {
		//Invalid (unknown) layoutType go to the back.
		return lhs_valid;
	}

	return lhs > rhs;
}

} // namespace page_split
