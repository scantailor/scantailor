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

#ifndef PAGE_SPLIT_TASK_H_
#define PAGE_SPLIT_TASK_H_

#include "NonCopyable.h"
#include "RefCountable.h"
#include "FilterResult.h"
#include "IntrusivePtr.h"
#include "ImageId.h"
#include <memory>

class TaskStatus;
class PageSequence;
class FilterData;
class DebugImages;
class QImage;

namespace deskew
{
	class Task;
}

namespace page_split
{

class Filter;
class Settings;

class Task : public RefCountable
{
	DECLARE_NON_COPYABLE(Task)
public:
	Task(
		IntrusivePtr<Filter> const& filter,
		IntrusivePtr<Settings> const& settings,
		IntrusivePtr<PageSequence> const& page_sequence,
		IntrusivePtr<deskew::Task> const& next_task,
		ImageId const& image_id,
		bool batch_processing, bool debug);
	
	virtual ~Task();
	
	FilterResultPtr process(TaskStatus const& status, FilterData const& data);
private:
	class UiUpdater;
	
	IntrusivePtr<Filter> m_ptrFilter;
	IntrusivePtr<Settings> m_ptrSettings;
	IntrusivePtr<PageSequence> m_ptrPageSequence;
	IntrusivePtr<deskew::Task> m_ptrNextTask;
	std::auto_ptr<DebugImages> m_ptrDbg;
	ImageId m_imageId;
	bool m_batchProcessing;
};

} // namespace PageSplit

#endif
