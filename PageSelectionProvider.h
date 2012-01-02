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

#ifndef PAGE_SELECTION_PROVIDER_H_
#define PAGE_SELECTION_PROVIDER_H_

#include "RefCountable.h"
#include <set>
#include <vector>

class PageSequence;
class PageId;
class PageRange;

class PageSelectionProvider : public RefCountable
{
public:
	virtual PageSequence allPages() const = 0;

	virtual std::set<PageId> selectedPages() const = 0;
	
	virtual std::vector<PageRange> selectedRanges() const = 0;
};

#endif
