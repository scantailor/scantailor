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

#include "PageSelectionAccessor.h"
#include "PageSequence.h"

PageSelectionAccessor::PageSelectionAccessor(
	IntrusivePtr<PageSelectionProvider const> const& provider)
:	m_ptrProvider(provider)
{
}

PageSequence
PageSelectionAccessor::allPages() const
{
	return m_ptrProvider->allPages();
}

std::set<PageId>
PageSelectionAccessor::selectedPages() const
{
	return m_ptrProvider->selectedPages();
}

std::vector<PageRange>
PageSelectionAccessor::selectedRanges() const
{
	return m_ptrProvider->selectedRanges();
}
