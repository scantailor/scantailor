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

#ifndef DESKEW_TASK_H_
#define DESKEW_TASK_H_

#include "NonCopyable.h"
#include "RefCountable.h"
#include "FilterResult.h"
#include "PageId.h"
#include <memory>

class TaskStatus;
class FilterData;
class QImage;
class QSize;
class Dpi;
class DebugImages;

namespace imageproc
{
	class BinaryImage;
};

namespace select_content
{
	class Task;
}

namespace deskew
{

class Filter;
class Settings;

class Task : public RefCountable
{
	DECLARE_NON_COPYABLE(Task)
public:
	Task(IntrusivePtr<Filter> const& filter,
		IntrusivePtr<Settings> const& settings,
		IntrusivePtr<select_content::Task> const& next_task,
		PageId const& page_id, bool batch_processing, bool debug);
	
	virtual ~Task();
	
	FilterResultPtr process(
		TaskStatus const& status, FilterData const& data);
private:
	class UiUpdater;
		
	static void cleanup(
		TaskStatus const& status,
		imageproc::BinaryImage& img, Dpi const& dpi);
	
	static int from150dpi(int size, int target_dpi);
	
	static QSize from150dpi(QSize const& size, Dpi const& target_dpi);
	
	IntrusivePtr<Filter> m_ptrFilter;
	IntrusivePtr<Settings> m_ptrSettings;
	IntrusivePtr<select_content::Task> m_ptrNextTask;
	std::auto_ptr<DebugImages> m_ptrDbg;
	PageId m_pageId;
	bool m_batchProcessing;
};

} // namespace deskew

#endif
