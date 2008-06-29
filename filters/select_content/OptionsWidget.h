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

#ifndef SELECT_CONTENT_OPTIONSWIDGET_H_
#define SELECT_CONTENT_OPTIONSWIDGET_H_

#include "ui_SelectContentOptionsWidget.h"
#include "FilterOptionsWidget.h"
#include "IntrusivePtr.h"
#include "AutoManualMode.h"
#include "Dependencies.h"
#include "PageId.h"
#include <QRectF>
#include <memory>

namespace select_content
{

class Settings;

class OptionsWidget :
	public FilterOptionsWidget, private Ui::SelectContentOptionsWidget
{
	Q_OBJECT
public:
	class UiData
	{
		// Member-wise copying is OK.
	public:
		UiData();
		
		~UiData();
		
		void setContentRect(QRectF const& content_rect);
		
		QRectF const& contentRect() const;
		
		void setDependencies(Dependencies const& deps);
		
		Dependencies const& dependencies() const;
		
		void setMode(AutoManualMode mode);
		
		AutoManualMode mode() const;
	private:
		QRectF m_contentRect; // In virtual image coordinates.
		Dependencies m_deps;
		AutoManualMode m_mode;
	};
	
	OptionsWidget(IntrusivePtr<Settings> const& settings);
	
	virtual ~OptionsWidget();
	
	void preUpdateUI(PageId const& page_id);
	
	void postUpdateUI(UiData const& ui_data);
public slots:
	void manualContentRectSet(QRectF const& content_rect);
private slots:
	void modeChanged(bool auto_mode);
private:
	void updateModeIndication(AutoManualMode const mode);
	
	void commitCurrentParams();
	
	IntrusivePtr<Settings> m_ptrSettings;
	UiData m_uiData;
	PageId m_pageId;
	int m_ignoreAutoManualToggle;
};

} // namespace select_content

#endif
