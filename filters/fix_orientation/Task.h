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

#ifndef FIX_ORIENTATION_TASK_H_
#define FIX_ORIENTATION_TASK_H_

#include "NonCopyable.h"
#include "RefCountable.h"
#include "FilterResult.h"
#include "IntrusivePtr.h"
#include "ImageId.h"

class TaskStatus;
class FilterData;
class QImage;

namespace page_split
{
	class Task;
}

namespace fix_orientation
{

class Filter;
class Settings;

class Task : public RefCountable
{
	DECLARE_NON_COPYABLE(Task)
public:
	Task(
		ImageId const& image_id,
		IntrusivePtr<Filter> const& filter,
		IntrusivePtr<Settings> const& settings,
		IntrusivePtr<page_split::Task> const& next_task,
		bool batch_processing);
	
	virtual ~Task();
	
	FilterResultPtr process(TaskStatus const& status, FilterData const& data);
private:
	class UiUpdater;
	
	IntrusivePtr<Filter> m_ptrFilter;
	IntrusivePtr<page_split::Task> m_ptrNextTask; // if null, this task is the final one
	IntrusivePtr<Settings> m_ptrSettings;
	ImageId m_imageId;
	bool m_batchProcessing;
};

} // namespace fix_orientation

#endif
