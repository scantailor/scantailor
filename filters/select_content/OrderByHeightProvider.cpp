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

#include "OrderByHeightProvider.h"
#include "Params.h"
#include <QSizeF>
#include <memory>

namespace select_content
{

OrderByHeightProvider::OrderByHeightProvider(IntrusivePtr<Settings> const& settings)
:	m_ptrSettings(settings)
{
}

bool
OrderByHeightProvider::precedes(PageId const& lhs, PageId const& rhs) const
{
	std::auto_ptr<Params> const lhs_params(m_ptrSettings->getPageParams(lhs));
	std::auto_ptr<Params> const rhs_params(m_ptrSettings->getPageParams(rhs));
	
	QSizeF lhs_size;
	if (lhs_params.get()) {
		lhs_size = lhs_params->contentSizeMM();
	}
	QSizeF rhs_size;
	if (rhs_params.get()) {
		rhs_size = rhs_params->contentSizeMM();
	}
	
	if (lhs_size.isValid() != rhs_size.isValid()) {
		// Invalid (unknown) sizes go to the back.
		return lhs_size.isValid();
	}

	return lhs_size.height() < rhs_size.height();
}

} // namespace select_content
