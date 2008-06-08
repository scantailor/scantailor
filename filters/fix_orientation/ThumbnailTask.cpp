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

#include "ThumbnailTask.h"
#include "Settings.h"
#include "PageInfo.h"
#include "ImageTransformation.h"
#include "ThumbnailBase.h"
#include "filters/page_split/ThumbnailTask.h"

namespace fix_orientation
{

ThumbnailTask::ThumbnailTask(
	IntrusivePtr<Settings> const& settings,
	IntrusivePtr<page_split::ThumbnailTask> const& next_task)
:	m_ptrNextTask(next_task),
	m_ptrSettings(settings)
{
}

ThumbnailTask::~ThumbnailTask()
{
}

std::auto_ptr<QGraphicsItem>
ThumbnailTask::process(
	ThumbnailPixmapCache& thumbnail_cache, PageInfo const& page_info)
{
	QRectF const initial_rect(QPointF(0.0, 0.0), page_info.metadata().size());
	ImageTransformation xform(initial_rect, page_info.metadata().dpi());
	xform.setPreRotation(m_ptrSettings->getRotationFor(page_info.id()));
	
	if (m_ptrNextTask) {
		return m_ptrNextTask->process(thumbnail_cache, page_info, xform);
	} else {
		return std::auto_ptr<QGraphicsItem>(
			new ThumbnailBase(thumbnail_cache, page_info.id(), xform)
		);
	}
}

} // namespace fix_orientation
