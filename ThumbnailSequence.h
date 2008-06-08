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

#ifndef THUMBNAILSEQUENCE_H_
#define THUMBNAILSEQUENCE_H_

#include "NonCopyable.h"
#include "IntrusivePtr.h"
#include <memory>

class QGraphicsItem;
class QGraphicsView;
class PageId;
class PageSequenceSnapshot;
class ThumbnailFactory;

class ThumbnailSequence
{
	DECLARE_NON_COPYABLE(ThumbnailSequence)
public:
	ThumbnailSequence();
	
	~ThumbnailSequence();
	
	void setThumbnailFactory(IntrusivePtr<ThumbnailFactory> const& factory);
	
	void attachView(QGraphicsView* view);
	
	void syncWith(PageSequenceSnapshot const& pages);
	
	void reset(PageSequenceSnapshot const& pages);
	
	void invalidateThumbnail(PageId const& page_id);
private:
	class Item;
	class Impl;
	class PlaceholderThumb;
	
	std::auto_ptr<Impl> m_ptrImpl;
};

#endif
