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

#include "CacheDrivenTask.h"
#include "Thumbnail.h"
#include "IncompleteThumbnail.h"
#include "Settings.h"
#include "PageInfo.h"
#include "ImageTransformation.h"
#include "filter_dc/AbstractFilterDataCollector.h"
#include "filter_dc/ThumbnailCollector.h"
#include "filters/deskew/CacheDrivenTask.h"

namespace page_split
{

CacheDrivenTask::CacheDrivenTask(
	IntrusivePtr<Settings> const& settings,
	IntrusivePtr<deskew::CacheDrivenTask> const& next_task)
:	m_ptrNextTask(next_task),
	m_ptrSettings(settings)
{
}

CacheDrivenTask::~CacheDrivenTask()
{
}

void
CacheDrivenTask::process(
	PageInfo const& page_info, AbstractFilterDataCollector* collector,
	ImageTransformation const& xform)
{
	Settings::Record const record(
		m_ptrSettings->getPageRecord(page_info.imageId())
	);
	
	OrthogonalRotation const pre_rotation(xform.preRotation());
	Dependencies const deps(
		page_info.metadata().size(), pre_rotation,
		record.combinedLayoutType()
	);
	
	Params const* params = record.params();
	
	if (!params || !deps.compatibleWith(*params)) {
		if (ThumbnailCollector* thumb_col = dynamic_cast<ThumbnailCollector*>(collector)) {
			thumb_col->processThumbnail(
				std::auto_ptr<QGraphicsItem>(
					new IncompleteThumbnail(
						thumb_col->thumbnailCache(),
						thumb_col->maxLogicalThumbSize(),
						page_info.imageId(), xform
					)
				)
			);
		}
		
		return;
	}
	
	PageLayout layout(params->pageLayout());
	if (layout.uncutOutline().isEmpty()) {
		// Backwards compatibility with versions < 0.9.9
		layout.setUncutOutline(xform.resultingRect());
	}

	if (m_ptrNextTask) {
		ImageTransformation new_xform(xform);
		new_xform.setPreCropArea(layout.pageOutline(page_info.id().subPage()));
		m_ptrNextTask->process(page_info, collector, new_xform);
		return;
	}
	
	if (ThumbnailCollector* thumb_col = dynamic_cast<ThumbnailCollector*>(collector)) {
		thumb_col->processThumbnail(
			std::auto_ptr<QGraphicsItem>(
				new Thumbnail(
					thumb_col->thumbnailCache(),
					thumb_col->maxLogicalThumbSize(),
					page_info.imageId(), xform, layout,
					page_info.leftHalfRemoved(),
					page_info.rightHalfRemoved()
				)
			)
		);
	}
}

} // namespace page_split
