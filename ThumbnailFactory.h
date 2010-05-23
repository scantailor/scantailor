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

#ifndef THUMBNAILFACTORY_H_
#define THUMBNAILFACTORY_H_

#include "NonCopyable.h"
#include "RefCountable.h"
#include "IntrusivePtr.h"
#include "ThumbnailPixmapCache.h"
#include <QSizeF>
#include <memory>

class PageInfo;
class CompositeCacheDrivenTask;
class QGraphicsItem;

class ThumbnailFactory : public RefCountable
{
	DECLARE_NON_COPYABLE(ThumbnailFactory)
public:
	ThumbnailFactory(
		IntrusivePtr<ThumbnailPixmapCache> const& pixmap_cache,
		QSizeF const& max_size, IntrusivePtr<CompositeCacheDrivenTask> const& task);
	
	virtual ~ThumbnailFactory();
	
	std::auto_ptr<QGraphicsItem> get(PageInfo const& page_info);
private:
	class Collector;
	
	IntrusivePtr<ThumbnailPixmapCache> m_ptrPixmapCache;
	QSizeF m_maxSize;
	IntrusivePtr<CompositeCacheDrivenTask> m_ptrTask;
};

#endif
