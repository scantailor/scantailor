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

#ifndef SELECT_CONTENT_THUMBNAILTASK_H_
#define SELECT_CONTENT_THUMBNAILTASK_H_

#include "NonCopyable.h"
#include "RefCountable.h"
#include "IntrusivePtr.h"
#include <memory>

class PageInfo;
class ImageTransformation;
class ThumbnailPixmapCache;
class QGraphicsItem;

namespace select_content
{

class Settings;

class ThumbnailTask : public RefCountable
{
	DECLARE_NON_COPYABLE(ThumbnailTask)
public:
	ThumbnailTask(IntrusivePtr<Settings> const& settings);
	
	virtual ~ThumbnailTask();
	
	std::auto_ptr<QGraphicsItem> process(
		ThumbnailPixmapCache& thumbnail_cache, PageInfo const& page_info,
		ImageTransformation const& xform);
private:
	IntrusivePtr<Settings> m_ptrSettings;
};

} // namespace select_content

#endif
