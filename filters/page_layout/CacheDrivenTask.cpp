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
#include "Settings.h"
#include "Params.h"
#include "Thumbnail.h"
#include "IncompleteThumbnail.h"
#include "ImageTransformation.h"
#include "PageInfo.h"
#include "PageId.h"
#include "Utils.h"
#include "filter_dc/AbstractFilterDataCollector.h"
#include "filter_dc/ThumbnailCollector.h"
#include "filter_dc/PageLayoutParamsCollector.h"
#include <QSizeF>
#include <QRectF>
#include <memory>

namespace page_layout
{

CacheDrivenTask::CacheDrivenTask(IntrusivePtr<Settings> const& settings)
:	m_ptrSettings(settings)
{
}

CacheDrivenTask::~CacheDrivenTask()
{
}

void
CacheDrivenTask::process(
	PageInfo const& page_info, AbstractFilterDataCollector* collector,
	ImageTransformation const& xform, QRectF const& content_rect)
{
	if (PageLayoutParamsCollector* plc = dynamic_cast<PageLayoutParamsCollector*>(collector)) {
		/*
		Here we perform two tasks:
		1. Propagating content rect from "Select Content" to "Page Layout".
		2. Collecting page parameters.
		Note that we can't combine the first task with thumbnail
		generation, because updating content rect may change the
		aggregate content size, which will require to re-generate
		all thumbnails.
		*/
		QSizeF const content_size_mm(
			Utils::calcRectSizeMM(xform, content_rect)
		);
		Params const params(
			m_ptrSettings->updateContentSizeAndGetParams(
				page_info.id(), content_size_mm
			)
		);
		plc->processPageParams(params);
	}
	
	if (ThumbnailCollector* thumb_col = dynamic_cast<ThumbnailCollector*>(collector)) {
		
		std::auto_ptr<Params> const params(
			m_ptrSettings->getPageParams(page_info.id())
		);
		
		if (!params.get()) {
			thumb_col->processThumbnail(
				std::auto_ptr<QGraphicsItem>(
					new IncompleteThumbnail(
						thumb_col->thumbnailCache(),
						thumb_col->maxLogicalThumbSize(),
						page_info.imageId(), xform
					)
				)
			);
		} else {
			thumb_col->processThumbnail(
				std::auto_ptr<QGraphicsItem>(
					new Thumbnail(
						thumb_col->thumbnailCache(),
						thumb_col->maxLogicalThumbSize(),
						page_info.imageId(), xform,
						*params, content_rect,
						m_ptrSettings->getAggregateHardSizeMM()
					)
				)
			);
		}
	}
}

} // namespace page_layout
