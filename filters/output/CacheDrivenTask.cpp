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
#include "ImageTransformation.h"
#include "OrthogonalRotation.h"
#include "PageInfo.h"
#include "PageId.h"
#include "ImageId.h"
#include "Utils.h"
#include "filter_dc/AbstractFilterDataCollector.h"
#include "filter_dc/ThumbnailCollector.h"

namespace output
{

CacheDrivenTask::CacheDrivenTask(QString const& out_dir)
:	m_outDir(out_dir)
{
}

CacheDrivenTask::~CacheDrivenTask()
{
}

void
CacheDrivenTask::process(
	PageInfo const& page_info, int const page_num,
	AbstractFilterDataCollector* collector,
	ImageTransformation const& xform,
	QPolygonF const& content_rect_phys, QPolygonF const& page_rect_phys)
{
	if (ThumbnailCollector* thumb_col = dynamic_cast<ThumbnailCollector*>(collector)) {
#if 0
		thumb_col->processThumbnail(
			std::auto_ptr<QGraphicsItem>(
				new IncompleteThumbnail(
					thumb_col->thumbnailCache(),
					thumb_col->maxLogicalThumbSize(),
					page_info.imageId(), xform
				)
			)
		);
#else
		QString const out_path(
			Utils::outFilePath(page_info.id(), page_num, m_outDir)
		);
		
		ImageTransformation out_xform(xform);
		// This resets all transforms.
		out_xform.setPreRotation(OrthogonalRotation());
		
		thumb_col->processThumbnail(
			std::auto_ptr<QGraphicsItem>(
				new Thumbnail(
					thumb_col->thumbnailCache(),
					thumb_col->maxLogicalThumbSize(),
					ImageId(out_path), out_xform
				)
			)
		);
#endif
	}
}

} // namespace output
