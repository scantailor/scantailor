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

#include "ThumbnailFactory.h"
#include "CompositeCacheDrivenTask.h"
#include "filter_dc/ThumbnailCollector.h"
#include <QGraphicsItem>
#include <QSizeF>

class ThumbnailFactory::Collector : public ThumbnailCollector
{
public:
	Collector(ThumbnailPixmapCache& cache, QSizeF const& max_size);
	
	virtual void processThumbnail(std::auto_ptr<QGraphicsItem> thumbnail);
	
	virtual ThumbnailPixmapCache& thumbnailCache();
	
	virtual QSizeF maxLogicalThumbSize() const;
	
	std::auto_ptr<QGraphicsItem> retrieveThumbnail() { return m_ptrThumbnail; }
private:
	ThumbnailPixmapCache& m_rCache;
	QSizeF m_maxSize;
	std::auto_ptr<QGraphicsItem> m_ptrThumbnail;
};


ThumbnailFactory::ThumbnailFactory(
	ThumbnailPixmapCache& pixmap_cache, QSizeF const& max_size,
	IntrusivePtr<CompositeCacheDrivenTask> const& task)
:	m_rPixmapCache(pixmap_cache),
	m_maxSize(max_size),
	m_ptrTask(task)
{
}

ThumbnailFactory::~ThumbnailFactory()
{
}

std::auto_ptr<QGraphicsItem>
ThumbnailFactory::get(
	PageInfo const& page_info, int const page_num)
{
	Collector collector(m_rPixmapCache, m_maxSize);
	m_ptrTask->process(page_info, page_num, &collector);
	return collector.retrieveThumbnail();
}


/*======================= ThumbnailFactory::Collector ======================*/

ThumbnailFactory::Collector::Collector(
	ThumbnailPixmapCache& cache, QSizeF const& max_size)
:	m_rCache(cache),
	m_maxSize(max_size)
{
}

void
ThumbnailFactory::Collector::processThumbnail(
	std::auto_ptr<QGraphicsItem> thumbnail)
{
	m_ptrThumbnail = thumbnail;
}

ThumbnailPixmapCache&
ThumbnailFactory::Collector::thumbnailCache()
{
	return m_rCache;
}

QSizeF
ThumbnailFactory::Collector::maxLogicalThumbSize() const
{
	return m_maxSize;
}
