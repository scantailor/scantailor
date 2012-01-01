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

#ifndef PAGE_LAYOUT_FILTER_H_
#define PAGE_LAYOUT_FILTER_H_

#include "NonCopyable.h"
#include "AbstractFilter.h"
#include "PageView.h"
#include "IntrusivePtr.h"
#include "FilterResult.h"
#include "SafeDeletingQObjectPtr.h"
#include "PageOrderOption.h"
#include <QCoreApplication>
#include <vector>

class PageId;
class ProjectPages;
class PageSelectionAccessor;
class ImageTransformation;
class QString;
class QRectF;

namespace output
{
	class Task;
	class CacheDrivenTask;
}

namespace page_layout
{

class OptionsWidget;
class Task;
class CacheDrivenTask;
class Settings;

class Filter : public AbstractFilter
{
	DECLARE_NON_COPYABLE(Filter)
	Q_DECLARE_TR_FUNCTIONS(page_layout::Filter)
public:
	Filter(IntrusivePtr<ProjectPages> const& page_sequence,
		PageSelectionAccessor const& page_selection_accessor);
	
	virtual ~Filter();
	
	virtual QString getName() const;
	
	virtual PageView getView() const;

	virtual void selected();

	virtual int selectedPageOrder() const;

	virtual void selectPageOrder(int option);

	virtual std::vector<PageOrderOption> pageOrderOptions() const;

	virtual void performRelinking(AbstractRelinker const& relinker);

	virtual void preUpdateUI(FilterUiInterface* ui, PageId const& page_id);
	
	virtual QDomElement saveSettings(
		ProjectWriter const& writer, QDomDocument& doc) const;
	
	virtual void loadSettings(
		ProjectReader const& reader, QDomElement const& filters_el);
	
	void setContentBox(
		PageId const& page_id, ImageTransformation const& xform,
		QRectF const& content_rect);
	
	void invalidateContentBox(PageId const& page_id);
	
	bool checkReadyForOutput(
		ProjectPages const& pages, PageId const* ignore = 0);
	
	IntrusivePtr<Task> createTask(
		PageId const& page_id,
		IntrusivePtr<output::Task> const& next_task,
		bool batch, bool debug);
	
	IntrusivePtr<CacheDrivenTask> createCacheDrivenTask(
		IntrusivePtr<output::CacheDrivenTask> const& next_task);
	
	OptionsWidget* optionsWidget() { return m_ptrOptionsWidget.get(); };
	Settings* getSettings() { return m_ptrSettings.get(); };
private:
	void writePageSettings(
		QDomDocument& doc, QDomElement& filter_el,
		PageId const& page_id, int numeric_id) const;
	
	IntrusivePtr<ProjectPages> m_ptrPages;
	IntrusivePtr<Settings> m_ptrSettings;
	SafeDeletingQObjectPtr<OptionsWidget> m_ptrOptionsWidget;
	std::vector<PageOrderOption> m_pageOrderOptions;
	int m_selectedPageOrder;
};

} // namespace page_layout

#endif
