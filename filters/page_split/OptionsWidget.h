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

#ifndef PAGE_SPLIT_OPTIONSWIDGET_H_
#define PAGE_SPLIT_OPTIONSWIDGET_H_

#include "ui_PageSplitOptionsWidget.h"
#include "FilterOptionsWidget.h"
#include "IntrusivePtr.h"
#include "Rule.h"
#include "PageLayout.h"
#include "ImageId.h"
#include "Dependencies.h"
#include "AutoManualMode.h"
#include "AutoDetectedLayout.h"

class ImageId;

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
		
		void setMode(AutoManualMode mode);
		
		AutoManualMode mode() const;
		
		void setAutoDetectedLayout(AutoDetectedLayout layout);
		
		AutoDetectedLayout autoDetectedLayout() const;
	private:
		PageLayout m_pageLayout;
		Dependencies m_deps;
		AutoManualMode m_mode;
		AutoDetectedLayout m_autoDetectedLayout;
	};
	
	
	OptionsWidget(IntrusivePtr<Settings> const& settings);
	
	virtual ~OptionsWidget();
	
	void preUpdateUI(ImageId const& image_id);
	
	void postUpdateUI(UiData const& ui_data);
public slots:
	void manualPageLayoutSet(PageLayout const& page_layout);
private slots:
	void singlePageSelected();
	
	void twoPagesSelected();
	
	void showChangeDialog();
	
	void ruleSet(Rule const& rule);
	
	void modeChanged(bool auto_mode);
private:
	void commitCurrentParams();
	
	IntrusivePtr<Settings> m_ptrSettings;
	ImageId m_imageId;
	UiData m_uiData;
	int m_ignoreAutoManualToggle;
};

} // namespace page_split

#endif
