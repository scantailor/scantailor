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

#ifndef PAGE_SELECTION_ACCESSOR_H_
#define PAGE_SELECTION_ACCESSOR_H_

#include "PageId.h"
#include "PageRange.h"
#include <QPointer>
#include <set>
#include <vector>

class MainWindow;

class PageSelectionAccessor
{
public:
	PageSelectionAccessor(MainWindow* main_window);
	
	PageSelectionAccessor(PageSelectionAccessor const& other);
	
	PageSelectionAccessor& operator=(PageSelectionAccessor const& rhs);
	
	std::set<PageId> selectedPages() const;
	
	std::vector<PageRange> selectedRanges() const;
private:
	QPointer<MainWindow> m_ptrMainWindow;
};

#endif
