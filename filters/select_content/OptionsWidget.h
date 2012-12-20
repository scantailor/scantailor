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

#ifndef SELECT_CONTENT_OPTIONSWIDGET_H_
#define SELECT_CONTENT_OPTIONSWIDGET_H_

#include "ui_SelectContentOptionsWidget.h"
#include "FilterOptionsWidget.h"
#include "IntrusivePtr.h"
#include "AutoManualMode.h"
#include "Dependencies.h"
#include "PhysSizeCalc.h"
#include "PageId.h"
#include "PageSelectionAccessor.h"
#include "Params.h"
#include <QSizeF>
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
		
		void setSizeCalc(PhysSizeCalc const& calc);

		void setContentRect(QRectF const& content_rect);
		void setPageRect(QRectF const& content_rect);
		
		QRectF const& contentRect() const;
		QRectF const& pageRect() const;

		QSizeF contentSizeMM() const;
		
		void setDependencies(Dependencies const& deps);
		
		Dependencies const& dependencies() const;
		
		void setMode(AutoManualMode mode);

		bool contentDetection() const { return m_contentDetection; }
		bool pageDetection() const { return m_pageDetection; }
		bool fineTuning() const { return m_fineTuneCorners; }

		void setContentDetection(bool detect);
		void setPageDetection(bool detect);
		void setFineTuneCorners(bool fine_tune);
		
        void setPageBorders(double left, double top, double right, double bottom);
        void setPageBorders(Margins const& borders) { m_borders = borders; };
        Margins pageBorders() const { return m_borders; }
        
		AutoManualMode mode() const;
	private:
		QRectF m_contentRect; // In virtual image coordinates.
		QRectF m_pageRect;
		PhysSizeCalc m_sizeCalc;
		Dependencies m_deps;
		AutoManualMode m_mode;
		bool m_contentDetection;
		bool m_pageDetection;
		bool m_fineTuneCorners;
        Margins m_borders;
	};
	
	OptionsWidget(IntrusivePtr<Settings> const& settings,
		PageSelectionAccessor const& page_selection_accessor);
	
	virtual ~OptionsWidget();
	
	void preUpdateUI(PageId const& page_id);
	
	void postUpdateUI(UiData const& ui_data);
public slots:
	void manualContentRectSet(QRectF const& content_rect);
private slots:
	void showApplyToDialog();

	void applySelection(std::set<PageId> const& pages, bool apply_content_box);

	void modeChanged(bool auto_mode);
	void autoMode();
	void manualMode();
	void fineTuningChanged(bool checked);
	void contentDetectionDisabled(void);
	void pageDetectionDisabled(void);
	void pageDetectionEnabled(void);
    
    void borderChanged();

private:
	void updateModeIndication(AutoManualMode const mode);
	
	void commitCurrentParams();
	
	IntrusivePtr<Settings> m_ptrSettings;
	UiData m_uiData;
	PageSelectionAccessor m_pageSelectionAccessor;
	PageId m_pageId;
	int m_ignoreAutoManualToggle;
};

} // namespace select_content

#endif
