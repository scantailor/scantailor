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

#ifndef OUTPUT_FILTER_H_
#define OUTPUT_FILTER_H_

#include "NonCopyable.h"
#include "AbstractFilter.h"
#include "IntrusivePtr.h"
#include "FilterResult.h"
#include <memory>

class PageId;
class ThumbnailPixmapCache;
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
	Filter();
	
	virtual ~Filter();
	
	virtual QString getName() const;
	
	virtual PageSequence::View getView() const;
	
	virtual void preUpdateUI(FilterUiInterface* ui, PageId const& page_id);
	
	virtual QDomElement saveSettings(
		ProjectWriter const& writer, QDomDocument& doc) const;
	
	virtual void loadSettings(
		ProjectReader const& reader, QDomElement const& filters_el);
	
	IntrusivePtr<Task> createTask(
		PageId const& page_id, int page_num,
		QString const& out_dir,
		ThumbnailPixmapCache& thumbnail_cache,
		bool batch, bool debug);
	
	IntrusivePtr<CacheDrivenTask> createCacheDrivenTask(
		QString const& out_dir);
	
	OptionsWidget* optionsWidget() { return m_ptrOptionsWidget.get(); }
private:
	void writePageSettings(
		QDomDocument& doc, QDomElement& filter_el,
		PageId const& page_id, int numeric_id) const;
	
	IntrusivePtr<Settings> m_ptrSettings;
	std::auto_ptr<OptionsWidget> m_ptrOptionsWidget;
};

} // namespace output

#endif
