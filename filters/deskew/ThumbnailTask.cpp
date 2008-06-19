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
#include "Thumbnail.h"
#include "IncompleteThumbnail.h"
#include "Settings.h"
#include "PageInfo.h"
#include "PageLayout.h"
#include "ImageTransformation.h"
#include "filters/select_content/ThumbnailTask.h"

namespace deskew
{

ThumbnailTask::ThumbnailTask(
	IntrusivePtr<Settings> const& settings,
	IntrusivePtr<select_content::ThumbnailTask> const& next_task)
:	m_ptrNextTask(next_task),
	m_ptrSettings(settings)
{
}

ThumbnailTask::~ThumbnailTask()
{
}

std::auto_ptr<QGraphicsItem>
ThumbnailTask::process(
	ThumbnailPixmapCache& thumbnail_cache, QSizeF const& max_size,
	PageInfo const& page_info, ImageTransformation const& xform,
	PageLayout const& layout)
{
	QRectF const rect(xform.rectBeforeCropping());
	QPolygonF const page_outline(
		layout.pageOutline(rect, page_info.id().subPage())
	);
	ImageTransformation new_xform(xform);
	new_xform.setCropArea(page_outline);
	
	Dependencies const deps(page_outline, xform.preRotation());
	std::auto_ptr<Params> params(m_ptrSettings->getPageParams(page_info.id()));
	if (!params.get() || !deps.matches(params->dependencies())) {
		return std::auto_ptr<QGraphicsItem>(
			new IncompleteThumbnail(
				thumbnail_cache, max_size,
				page_info.imageId(), new_xform
			)
		);
	}
	
	new_xform.setPostRotation(params->deskewAngle());
	
	if (m_ptrNextTask) {
		return m_ptrNextTask->process(
			thumbnail_cache, max_size, page_info, new_xform
		);
	} else {
		return std::auto_ptr<QGraphicsItem>(
			new Thumbnail(
				thumbnail_cache, max_size,
				page_info.imageId(), new_xform
			)
		);
	}
}

} // namespace deskew
