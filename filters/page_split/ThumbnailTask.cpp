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
#include "LayoutTypeResolver.h"
#include "IncompleteThumbnail.h"
#include "Settings.h"
#include "PageInfo.h"
#include "ImageTransformation.h"
#include "filters/deskew/ThumbnailTask.h"
#include <QDebug>

namespace page_split
{

ThumbnailTask::ThumbnailTask(
	IntrusivePtr<Settings> const& settings,
	IntrusivePtr<deskew::ThumbnailTask> const& next_task)
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
	PageInfo const& page_info, ImageTransformation const& xform)
{
	OrthogonalRotation const pre_rotation(xform.preRotation());
	
	Rule const rule(m_ptrSettings->getRuleFor(page_info.imageId()));
	LayoutTypeResolver const resolver(rule.layoutType());
	int const num_logical_pages = resolver.numLogicalPages(
		page_info.metadata(), pre_rotation
	);
	bool const single_page = (num_logical_pages == 1);
	
	Dependencies const deps(page_info.metadata().size(), pre_rotation, single_page);
	
	std::auto_ptr<Params> params(m_ptrSettings->getPageParams(page_info.imageId()));
	if (!params.get() || !deps.matches(params->dependencies())) {
		return std::auto_ptr<QGraphicsItem>(
			new IncompleteThumbnail(
				thumbnail_cache, max_size, page_info.imageId(), xform
			)
		);
	}
	
	PageLayout const layout(params->pageLayout());
	
	if (m_ptrNextTask) {
		return m_ptrNextTask->process(
			thumbnail_cache, max_size, page_info, xform, layout
		);
	} else {
		return std::auto_ptr<QGraphicsItem>(
			new Thumbnail(
				thumbnail_cache, max_size,
				page_info.imageId(), xform, layout
			)
		);
	}
}

} // namespace page_split
