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
#include "Settings.h"
#include "PageInfo.h"
#include "ImageTransformation.h"
#include "OrthogonalRotation.h"
#include "PageSequence.h"
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
	ThumbnailPixmapCache& thumbnail_cache, PageInfo const& page_info,
	ImageTransformation const& xform)
{
	OrthogonalRotation const pre_rotation(xform.preRotation());
	
	// TODO: maybe make the Dependencies CTOR taking rule and xform?
	
	Rule const rule(m_ptrSettings->getRuleFor(page_info.id()));
	int num_logical_pages = 1;
	switch (rule.layoutType()) {
		case Rule::AUTO_DETECT: {
			num_logical_pages = PageSequence::adviseNumberOfLogicalPages(
				page_info.metadata(), pre_rotation
			);
			break;
		}
		case Rule::SINGLE_PAGE:
			num_logical_pages = true;
			break;
		case Rule::TWO_PAGES:
			num_logical_pages = false;
			break;
	}
	
	bool const single_page = (num_logical_pages == 1);
	Dependencies const deps(page_info.metadata().size(), pre_rotation, single_page);
	
	std::auto_ptr<Params> params(m_ptrSettings->getPageParams(page_info.id()));
	if (!params.get() || !deps.matches(params->dependencies())) {
		return std::auto_ptr<QGraphicsItem>();
	}
	
	PageLayout const layout(params->pageLayout());
	
	if (m_ptrNextTask) {
		return m_ptrNextTask->process(thumbnail_cache, page_info, xform, layout);
	} else {
		return std::auto_ptr<QGraphicsItem>(
			new Thumbnail(thumbnail_cache, page_info.id(), xform, layout)
		);
	}
}

} // namespace page_split
