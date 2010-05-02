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

#ifndef COMPOSITECACHEDRIVENTASK_H_
#define COMPOSITECACHEDRIVENTASK_H_

#include "RefCountable.h"

class PageInfo;
class AbstractFilterDataCollector;

class CompositeCacheDrivenTask : public RefCountable
{
public:
	virtual ~CompositeCacheDrivenTask() {}
	
	virtual void process(
		PageInfo const& page_info,
		AbstractFilterDataCollector* collector) = 0;
};

#endif
