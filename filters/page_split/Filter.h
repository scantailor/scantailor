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

#ifndef PAGE_SPLIT_FILTER_H_
#define PAGE_SPLIT_FILTER_H_

#include "NonCopyable.h"
#include "AbstractFilter.h"
#include "PageView.h"
#include "IntrusivePtr.h"
#include "FilterResult.h"
#include "SafeDeletingQObjectPtr.h"
#include <set>
#include "PageOrderOption.h"
#include <QCoreApplication>

class PageId;
class ImageId;
class PageInfo;
class ProjectPages;
class PageSelectionAccessor;
class OrthogonalRotation;

namespace deskew
{
	class Task;
	class CacheDrivenTask;
}

namespace page_split
{

class OptionsWidget;
class Task;
class CacheDrivenTask;
class Settings;
class Params;

class Filter : public AbstractFilter
{
	DECLARE_NON_COPYABLE(Filter)
	Q_DECLARE_TR_FUNCTIONS(page_split::Filter)
public:
	Filter(IntrusivePtr<ProjectPages> const& page_sequence,
		PageSelectionAccessor const& page_selection_accessor);
	
	virtual ~Filter();
	
	virtual QString getName() const;
	
	virtual PageView getView() const;

	virtual void performRelinking(AbstractRelinker const& relinker);
	
	virtual void preUpdateUI(FilterUiInterface* ui, PageId const& page_id);
	
	virtual QDomElement saveSettings(
		ProjectWriter const& wirter, QDomDocument& doc) const;
	
	virtual void loadSettings(
		ProjectReader const& reader, QDomElement const& filters_el);
	
	IntrusivePtr<Task> createTask(PageInfo const& page_info,
		IntrusivePtr<deskew::Task> const& next_task,
		bool batch_processing, bool debug);
	
	IntrusivePtr<CacheDrivenTask> createCacheDrivenTask(
		IntrusivePtr<deskew::CacheDrivenTask> const& next_task);
	
	OptionsWidget* optionsWidget() { return m_ptrOptionsWidget.get(); }

	void pageOrientationUpdate(
		ImageId const& image_id, OrthogonalRotation const& orientation);

	Settings* getSettings() { return m_ptrSettings.get(); };
	
	virtual std::vector<PageOrderOption> pageOrderOptions() const;
	virtual int selectedPageOrder() const;
	virtual void selectPageOrder(int option);
private:
	void writeImageSettings(
		QDomDocument& doc, QDomElement& filter_el,
		ImageId const& image_id, int const numeric_id) const;
	
	IntrusivePtr<ProjectPages> m_ptrPages;
	IntrusivePtr<Settings> m_ptrSettings;
	SafeDeletingQObjectPtr<OptionsWidget> m_ptrOptionsWidget;
	std::vector<PageOrderOption> m_pageOrderOptions;
	int m_selectedPageOrder;
};

} // namespace page_split

#endif
