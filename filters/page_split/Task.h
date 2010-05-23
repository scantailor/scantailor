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

#ifndef PAGE_SPLIT_TASK_H_
#define PAGE_SPLIT_TASK_H_

#include "NonCopyable.h"
#include "RefCountable.h"
#include "FilterResult.h"
#include "IntrusivePtr.h"
#include "PageInfo.h"
#include <memory>

class TaskStatus;
class FilterData;
class DebugImages;
class ProjectPages;
class QImage;

namespace deskew
{
	class Task;
}

namespace page_split
{

class Filter;
class Settings;
class PageLayout;

class Task : public RefCountable
{
	DECLARE_NON_COPYABLE(Task)
public:
	Task(
		IntrusivePtr<Filter> const& filter,
		IntrusivePtr<Settings> const& settings,
		IntrusivePtr<ProjectPages> const& pages,
		IntrusivePtr<deskew::Task> const& next_task,
		PageInfo const& page_info,
		bool batch_processing, bool debug);
	
	virtual ~Task();
	
	FilterResultPtr process(TaskStatus const& status, FilterData const& data);
private:
	class UiUpdater;

	IntrusivePtr<Filter> m_ptrFilter;
	IntrusivePtr<Settings> m_ptrSettings;
	IntrusivePtr<ProjectPages> m_ptrPages;
	IntrusivePtr<deskew::Task> m_ptrNextTask;
	std::auto_ptr<DebugImages> m_ptrDbg;
	PageInfo m_pageInfo;
	bool m_batchProcessing;
};

} // namespace PageSplit

#endif
