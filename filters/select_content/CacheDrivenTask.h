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

#ifndef SELECT_CONTENT_CACHEDRIVENTASK_H_
#define SELECT_CONTENT_CACHEDRIVENTASK_H_

#include "NonCopyable.h"
#include "RefCountable.h"
#include "IntrusivePtr.h"

class QSizeF;
class PageInfo;
class AbstractFilterDataCollector;
class ImageTransformation;

namespace page_layout
{
	class CacheDrivenTask;
}

namespace select_content
{

class Settings;

class CacheDrivenTask : public RefCountable
{
	DECLARE_NON_COPYABLE(CacheDrivenTask)
public:
	CacheDrivenTask(IntrusivePtr<Settings> const& settings,
		IntrusivePtr<page_layout::CacheDrivenTask> const& next_task);
	
	virtual ~CacheDrivenTask();
	
	void process(
		PageInfo const& page_info, AbstractFilterDataCollector* collector,
		ImageTransformation const& xform);
private:
	IntrusivePtr<Settings> m_ptrSettings;
	IntrusivePtr<page_layout::CacheDrivenTask> m_ptrNextTask;
};

} // namespace select_content

#endif
