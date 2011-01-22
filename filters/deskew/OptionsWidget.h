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

#ifndef DESKEW_OPTIONSWIDGET_H_
#define DESKEW_OPTIONSWIDGET_H_

#include "ui_DeskewOptionsWidget.h"
#include "FilterOptionsWidget.h"
#include "IntrusivePtr.h"
#include "PageId.h"
#include "Dependencies.h"
#include "AutoManualMode.h"
#include "PageSelectionAccessor.h"
#include <set>

namespace deskew
{

class Settings;

class OptionsWidget :
	public FilterOptionsWidget, private Ui::DeskewOptionsWidget
{
	Q_OBJECT
public:
	class UiData
	{
		// Member-wise copying is OK.
	public:
		UiData();
		
		~UiData();
		
		void setEffectiveDeskewAngle(double degrees);
		
		double effectiveDeskewAngle() const;
		
		void setDependencies(Dependencies const& deps);
		
		Dependencies const& dependencies() const;
		
		void setMode(AutoManualMode mode);
		
		AutoManualMode mode() const;
	private:
		double m_effDeskewAngle;
		Dependencies m_deps;
		AutoManualMode m_mode;
	};
	
	OptionsWidget(IntrusivePtr<Settings> const& settings,
		PageSelectionAccessor const& page_selection_accessor);
	
	virtual ~OptionsWidget();
signals:
	void manualDeskewAngleSet(double degrees);
public slots:
	void manualDeskewAngleSetExternally(double degrees);
public:
	void preUpdateUI(PageId const& page_id);
	
	void postUpdateUI(UiData const& ui_data);
private slots:
	void spinBoxValueChanged(double skew_degrees);
	
	void modeChanged(bool auto_mode);
	void showDeskewDialog();
	void appliedTo(std::set<PageId> const& pages);
	void appliedToAllPages(std::set<PageId> const& pages);
private:
	void updateModeIndication(AutoManualMode mode);
	
	void setSpinBoxUnknownState();
	
	void setSpinBoxKnownState(double angle);
	
	void commitCurrentParams();
	
	static double spinBoxToDegrees(double sb_value);
	
	static double degreesToSpinBox(double degrees);
	
	static double const MAX_ANGLE;
	
	IntrusivePtr<Settings> m_ptrSettings;
	PageId m_pageId;
	UiData m_uiData;
	int m_ignoreAutoManualToggle;
	int m_ignoreSpinBoxChanges;
	
	PageSelectionAccessor m_pageSelectionAccessor;
};

} // namespace deskew

#endif
