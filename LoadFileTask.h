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

#ifndef LOADFILETASK_H_
#define LOADFILETASK_H_

#include "NonCopyable.h"
#include "BackgroundTask.h"
#include "FilterResult.h"
#include "IntrusivePtr.h"
#include "ImageId.h"
#include "ImageMetadata.h"

class ThumbnailPixmapCache;
class PageInfo;
class PageSequence;
class QImage;

namespace fix_orientation
{
	class Task;
}

class LoadFileTask : public BackgroundTask
{
	DECLARE_NON_COPYABLE(LoadFileTask)
public:
	LoadFileTask(PageInfo const& page, ThumbnailPixmapCache& thumbnail_cache,
		IntrusivePtr<PageSequence> const& page_sequence,
		IntrusivePtr<fix_orientation::Task> const& next_task);
	
	virtual ~LoadFileTask();
	
	virtual FilterResultPtr operator()();
private:
	class ErrorResult;
	
	void updateImageSizeIfChanged(QImage const& image);
	
	void overrideDpi(QImage& image) const;
	
	ThumbnailPixmapCache& m_rThumbnailCache;
	ImageId m_imageId;
	ImageMetadata m_imageMetadata;
	IntrusivePtr<PageSequence> const m_ptrPageSequence;
	IntrusivePtr<fix_orientation::Task> const m_ptrNextTask;
};

#endif
