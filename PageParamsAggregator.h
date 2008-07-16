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

#ifndef PAGEPARAMSAGGREGATOR_H_
#define PAGEPARAMSAGGREGATOR_H_

#include "IntrusivePtr.h"
#include <QSizeF>

class CompositeCacheDrivenTask;
class PageSequence;

class PageParamsAggregator
{
public:
	PageParamsAggregator(IntrusivePtr<CompositeCacheDrivenTask> const& task);
	
	~PageParamsAggregator();
	
	void aggregate(PageSequence const& pages);
	
	bool haveUndefinedItems() const { return m_haveUndefinedItems; }
private:
	class Collector;
	
	IntrusivePtr<CompositeCacheDrivenTask> m_ptrTask;
	bool m_haveUndefinedItems;
};

#endif
