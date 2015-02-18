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

#ifndef OUTPUT_FILTER_H_
#define OUTPUT_FILTER_H_

#include "NonCopyable.h"
#include "AbstractFilter.h"
#include "PageView.h"
#include "IntrusivePtr.h"
#include "FilterResult.h"
#include "SafeDeletingQObjectPtr.h"
#include "PictureZonePropFactory.h"
#include "FillZonePropFactory.h"
//begin of modified by monday2000
//Original_Foreground_Mixed
//added:
#include <QImage>
//end of modified by monday2000

class PageId;
class PageSelectionAccessor;
class ThumbnailPixmapCache;
class OutputFileNameGenerator;
class QString;

namespace output
{

class OptionsWidget;
class Task;
class CacheDrivenTask;
class Settings;

class Filter : public AbstractFilter
{
	DECLARE_NON_COPYABLE(Filter)
public:
	Filter(PageSelectionAccessor const& page_selection_accessor);
	
	virtual ~Filter();
	
	virtual QString getName() const;
	
	virtual PageView getView() const;
	
	virtual void performRelinking(AbstractRelinker const& relinker);

	virtual void preUpdateUI(FilterUiInterface* ui, PageId const& page_id);
	
	virtual QDomElement saveSettings(
		ProjectWriter const& writer, QDomDocument& doc) const;
	
	virtual void loadSettings(
		ProjectReader const& reader, QDomElement const& filters_el);
	
	IntrusivePtr<Task> createTask(
		PageId const& page_id,
		IntrusivePtr<ThumbnailPixmapCache> const& thumbnail_cache,
		OutputFileNameGenerator const& out_file_name_gen,
//begin of modified by monday2000
//Dont_Equalize_Illumination_Pic_Zones
//Original_Foreground_Mixed
		//bool batch, bool debug);
		bool batch, bool debug, bool dont_equalize_illumination_pic_zones = false, // "false" as cli workaround
		bool keep_orig_fore_subscan = false, 
		QImage* p_orig_fore_subscan = NULL);
//end of modified by monday2000

	IntrusivePtr<CacheDrivenTask> createCacheDrivenTask(
		OutputFileNameGenerator const& out_file_name_gen);
	
	OptionsWidget* optionsWidget() { return m_ptrOptionsWidget.get(); };
	Settings* getSettings() { return m_ptrSettings.get(); };
private:
	void writePageSettings(
		QDomDocument& doc, QDomElement& filter_el,
		PageId const& page_id, int numeric_id) const;
	
	IntrusivePtr<Settings> m_ptrSettings;
	SafeDeletingQObjectPtr<OptionsWidget> m_ptrOptionsWidget;
	PictureZonePropFactory m_pictureZonePropFactory;
	FillZonePropFactory m_fillZonePropFactory;
};

} // namespace output

#endif
