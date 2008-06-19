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
#include "filters/fix_orientation/ThumbnailTask.h"
#include <QGraphicsItem>

ThumbnailFactory::ThumbnailFactory(
	ThumbnailPixmapCache& pixmap_cache, QSizeF const& max_size,
	IntrusivePtr<fix_orientation::ThumbnailTask> const& task_chain)
:	m_rPixmapCache(pixmap_cache),
	m_maxSize(max_size),
	m_ptrTaskChain(task_chain)
{
}

ThumbnailFactory::~ThumbnailFactory()
{
}

std::auto_ptr<QGraphicsItem>
ThumbnailFactory::get(PageInfo const& page_info)
{
	return m_ptrTaskChain->process(m_rPixmapCache, m_maxSize, page_info);
}
