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
#include "Thumbnail.h"
#include "IncompleteThumbnail.h"
#include "ImageTransformation.h"
#include "PageInfo.h"
#include "PageId.h"
#include "ImageId.h"
#include "Dpi.h"
#include "Utils.h"
#include "../page_layout/Utils.h"
#include "filter_dc/AbstractFilterDataCollector.h"
#include "filter_dc/ThumbnailCollector.h"
#include <QFile>
#include <QRect>
#include <QRectF>
#include <QTransform>

namespace output
{

CacheDrivenTask::CacheDrivenTask(
	IntrusivePtr<Settings> const& settings, QString const& out_dir)
:	m_ptrSettings(settings),
	m_outDir(out_dir)
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
		
		QString const out_path(
			Utils::outFilePath(page_info.id(), page_num, m_outDir)
		);
		
		if (!QFile::exists(out_path)) {
			ImageTransformation const new_xform(
				page_layout::Utils::calcPresentationTransform(
					xform, page_rect_phys
				)
			);
			
			thumb_col->processThumbnail(
				std::auto_ptr<QGraphicsItem>(
					new IncompleteThumbnail(
						thumb_col->thumbnailCache(),
						thumb_col->maxLogicalThumbSize(),
						page_info.imageId(), new_xform
					)
				)
			);
		} else {
			Dpi const out_dpi(m_ptrSettings->getDpi(page_info.id()));
			
			QTransform tmp_xform(xform.transform());
			tmp_xform *= Utils::scaleFromToDpi(
				xform.preScaledDpi(), out_dpi
			);
			
			QRect const crop_rect(
				tmp_xform.map(page_rect_phys).boundingRect().toRect()
			);
			
			ImageTransformation const out_xform(
				crop_rect.translated(-crop_rect.topLeft()), out_dpi
			);
			
			thumb_col->processThumbnail(
				std::auto_ptr<QGraphicsItem>(
					new Thumbnail(
						thumb_col->thumbnailCache(),
						thumb_col->maxLogicalThumbSize(),
						ImageId(out_path), out_xform
					)
				)
			);
		}
	}
}

} // namespace output
