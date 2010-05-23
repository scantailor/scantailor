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

#ifndef PAGE_SPLIT_OPTIONSWIDGET_H_
#define PAGE_SPLIT_OPTIONSWIDGET_H_

#include "ui_PageSplitOptionsWidget.h"
#include "FilterOptionsWidget.h"
#include "IntrusivePtr.h"
#include "LayoutType.h"
#include "PageLayout.h"
#include "ImageId.h"
#include "PageId.h"
#include "PageSelectionAccessor.h"
#include "Dependencies.h"
#include "AutoManualMode.h"
#include <set>

class ProjectPages;

namespace page_split
{

class Settings;

class OptionsWidget :
	public FilterOptionsWidget, private Ui::PageSplitOptionsWidget
{
	Q_OBJECT
public:
	class UiData
	{
		// Member-wise copying is OK.
	public:
		UiData();
		
		~UiData();
		
		void setPageLayout(PageLayout const& layout);
		
		PageLayout const& pageLayout() const;
		
		void setDependencies(Dependencies const& deps);
		
		Dependencies const& dependencies() const;
		
		void setSplitLineMode(AutoManualMode mode);
		
		AutoManualMode splitLineMode() const;
		
		bool layoutTypeAutoDetected() const;
		
		void setLayoutTypeAutoDetected(bool val);
	private:
		PageLayout m_pageLayout;
		Dependencies m_deps;
		AutoManualMode m_splitLineMode;
		bool m_layoutTypeAutoDetected;
	};
	
	
	OptionsWidget(IntrusivePtr<Settings> const& settings,
		IntrusivePtr<ProjectPages> const& page_sequence,
		PageSelectionAccessor const& page_selection_accessor);
	
	virtual ~OptionsWidget();
	
	void preUpdateUI(PageId const& page_id);
	
	void postUpdateUI(UiData const& ui_data);
signals:
	void pageLayoutSetLocally(PageLayout const& page_layout);
public slots:
	void pageLayoutSetExternally(PageLayout const& page_layout);
private slots:
	void layoutTypeButtonToggled(bool checked);
	
	void showChangeDialog();
	
	void layoutTypeSet(
		std::set<PageId> const& pages,
		bool all_pages, LayoutType layout_type);
	
	void splitLineModeChanged(bool auto_mode);
private:
	void commitCurrentParams();
	
	IntrusivePtr<Settings> m_ptrSettings;
	IntrusivePtr<ProjectPages> m_ptrPages;
	PageSelectionAccessor m_pageSelectionAccessor;
	PageId m_pageId;
	UiData m_uiData;
	int m_ignoreAutoManualToggle;
	int m_ignoreLayoutTypeToggle;
};

} // namespace page_split

#endif
