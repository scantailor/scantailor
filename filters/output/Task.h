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

#ifndef OUTPUT_TASK_H_
#define OUTPUT_TASK_H_

#include "NonCopyable.h"
#include "RefCountable.h"
#include "FilterResult.h"
#include "PageInfo.h"
#include "ImageViewTab.h"
#include <QString>
#include <QColor>
#include <Qt>
#include <memory>

class DebugImages;
class TaskStatus;
class FilterData;
class ThumbnailPixmapCache;
class ImageTransformation;
class QPolygonF;
class QSize;
class QImage;
class Dpi;

namespace imageproc
{
	class BinaryImage;
}

namespace output
{

class Filter;
class Settings;

class Task : public RefCountable
{
	DECLARE_NON_COPYABLE(Task)
public:
	Task(IntrusivePtr<Filter> const& filter,
		IntrusivePtr<Settings> const& settings,
		ThumbnailPixmapCache& thumbnail_cache,
		PageInfo const& page_info, QString const& out_dir,
		Qt::LayoutDirection layout_direction,
		ImageViewTab last_tab, bool batch, bool debug);
	
	virtual ~Task();
	
	FilterResultPtr process(
		TaskStatus const& status, FilterData const& data,
		QPolygonF const& content_rect_phys,
		QPolygonF const& page_rect_phys);
private:
	class UiUpdater;
	
	IntrusivePtr<Filter> m_ptrFilter;
	IntrusivePtr<Settings> m_ptrSettings;
	ThumbnailPixmapCache& m_rThumbnailCache;
	std::auto_ptr<DebugImages> m_ptrDbg;
	PageInfo m_pageInfo;
	QString m_outDir;
	Qt::LayoutDirection m_layoutDirection;
	ImageViewTab m_lastTab;
	bool m_batchProcessing;
	bool m_debug;
};

} // namespace output

#endif
