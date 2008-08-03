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

#include "CacheDrivenTask.h"
#include "Thumbnail.h"
#include "IncompleteThumbnail.h"
#include "Settings.h"
#include "PageInfo.h"
#include "PageLayout.h"
#include "ImageTransformation.h"
#include "filter_dc/AbstractFilterDataCollector.h"
#include "filter_dc/ThumbnailCollector.h"
#include "filters/select_content/CacheDrivenTask.h"

namespace deskew
{

CacheDrivenTask::CacheDrivenTask(
	IntrusivePtr<Settings> const& settings,
	IntrusivePtr<select_content::CacheDrivenTask> const& next_task)
:	m_ptrNextTask(next_task),
	m_ptrSettings(settings)
{
}

CacheDrivenTask::~CacheDrivenTask()
{
}

void
CacheDrivenTask::process(
	PageInfo const& page_info, int const page_num,
	AbstractFilterDataCollector* collector,
	ImageTransformation const& xform, PageLayout const& layout)
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
		
		if (ThumbnailCollector* thumb_col = dynamic_cast<ThumbnailCollector*>(collector)) {
			thumb_col->processThumbnail(
				std::auto_ptr<QGraphicsItem>(
					new IncompleteThumbnail(
						thumb_col->thumbnailCache(),
						thumb_col->maxLogicalThumbSize(),
						page_info.imageId(), new_xform
					)
				)
			);
		}
		
		return;
	}
	
	new_xform.setPostRotation(params->deskewAngle());
	
	if (m_ptrNextTask) {
		m_ptrNextTask->process(page_info, page_num, collector, new_xform);
		return;
	}
	
	if (ThumbnailCollector* thumb_col = dynamic_cast<ThumbnailCollector*>(collector)) {
		thumb_col->processThumbnail(
			std::auto_ptr<QGraphicsItem>(
				new Thumbnail(
					thumb_col->thumbnailCache(),
					thumb_col->maxLogicalThumbSize(),
					page_info.imageId(), new_xform
				)
			)
		);
	}
}

} // namespace deskew
