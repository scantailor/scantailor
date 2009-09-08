/*
    Scan Tailor - Interactive post-processing tool for scanned pages.
    Copyright (C) 2007-2009  Joseph Artsimovich <joseph_a@mail.ru>

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

#ifndef OUTPUT_OPTIONSWIDGET_H_
#define OUTPUT_OPTIONSWIDGET_H_

#include "ui_OutputOptionsWidget.h"
#include "FilterOptionsWidget.h"
#include "IntrusivePtr.h"
#include "PageId.h"
#include "PageSelectionAccessor.h"
#include "ColorParams.h"
#include "Dpi.h"
#include <set>

class PageSequence;

namespace output
{

class Settings;

class OptionsWidget
	: public FilterOptionsWidget, private Ui::OutputOptionsWidget
{
	Q_OBJECT
public:
	OptionsWidget(IntrusivePtr<Settings> const& settings,
		IntrusivePtr<PageSequence> const& pages,
		PageSelectionAccessor const& page_selection_accessor);
	
	virtual ~OptionsWidget();
	
	void preUpdateUI(PageId const& page_id);
	
	void postUpdateUI();
public slots:
	void reloadIfZonesChanged();
private slots:
	void changeDpiButtonClicked();
	
	void applyColorsButtonClicked();
	
	void dpiChanged(std::set<PageId> const& pages, Dpi const& dpi);
	
	void applyColorsConfirmed(std::set<PageId> const& pages);
	
	void colorModeChanged(int idx);
	
	void whiteMarginsToggled(bool checked);
	
	void equalizeIlluminationToggled(bool checked);
	
	void despeckleToggled(bool checked);
	
	void setLighterThreshold();
	
	void setDarkerThreshold();
	
	void setNeutralThreshold();
	
	void bwThresholdChanged(int value);
private:
	void updateDpiDisplay();
	
	void updateColorsDisplay();
	
	IntrusivePtr<Settings> m_ptrSettings;
	IntrusivePtr<PageSequence> m_ptrPages;
	PageSelectionAccessor m_pageSelectionAccessor;
	PageId m_pageId;
	Dpi m_dpi;
	ColorParams m_colorParams;
	int m_ignoreThresholdChanges;
};

} // namespace output

#endif
