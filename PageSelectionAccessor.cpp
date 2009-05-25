/*
    Scan Tailor - Interactive post-processing tool for scanned pages.
    Copyright (C) 2007-2008  Joseph Artsimovich <joseph_a@mail.ru>

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
#include "MainWindow.h"

PageSelectionAccessor::PageSelectionAccessor(MainWindow* main_window)
:	m_ptrMainWindow(main_window)
{
}

PageSelectionAccessor::PageSelectionAccessor(PageSelectionAccessor const& other)
:	m_ptrMainWindow(other.m_ptrMainWindow)
{
}

PageSelectionAccessor&
PageSelectionAccessor::operator=(PageSelectionAccessor const& rhs)
{
	m_ptrMainWindow = rhs.m_ptrMainWindow;
	return *this;
}

std::set<PageId>
PageSelectionAccessor::selectedPages() const
{
	if (m_ptrMainWindow) {
		return m_ptrMainWindow->selectedPages();
	}
	
	return std::set<PageId>();
}

std::vector<PageRange>
PageSelectionAccessor::selectedRanges() const
{
	if (m_ptrMainWindow) {
		return m_ptrMainWindow->selectedRanges();
	}
	
	return std::vector<PageRange>();
}

