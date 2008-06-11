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
#include "PageInfo.h"
#include "ImageTransformation.h"
#include "Settings.h"

namespace select_content
{

ThumbnailTask::ThumbnailTask(IntrusivePtr<Settings> const& settings)
:	m_ptrSettings(settings)
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
	std::auto_ptr<Params> params(m_ptrSettings->getPageParams(page_info.id()));
	Dependencies const deps(xform.resultingCropArea());
	if (!params.get() || !params->dependencies().matches(deps)) {
		return std::auto_ptr<QGraphicsItem>(
			new IncompleteThumbnail(
				thumbnail_cache, page_info.id(), xform
			)
		);
	}
	
	return std::auto_ptr<QGraphicsItem>(
		new Thumbnail(
			thumbnail_cache, page_info.id().imageId(), xform,
			params->contentRect()
		)
	);
}

} // namespace select_content
