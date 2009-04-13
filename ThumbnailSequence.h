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
#include "BeforeOrAfter.h"
#include <QObject>
#include <memory>

class QGraphicsItem;
class QGraphicsView;
class PageId;
class ImageId;
class PageInfo;
class PageSequenceSnapshot;
class ThumbnailFactory;
class QSizeF;
class QRectF;
class QPoint;

class ThumbnailSequence : public QObject
{
	Q_OBJECT
	DECLARE_NON_COPYABLE(ThumbnailSequence)
public:
	ThumbnailSequence(QSizeF const& max_logical_thumb_size);
	
	~ThumbnailSequence();
	
	void setThumbnailFactory(IntrusivePtr<ThumbnailFactory> const& factory);
	
	void attachView(QGraphicsView* view);
	
	void reset(PageSequenceSnapshot const& pages);
	
	void invalidateThumbnail(PageId const& page_id);
	
	void invalidateThumbnail(ImageId const& image_id);
	
	void invalidateAllThumbnails();
	
	void setCurrentThumbnail(PageId const& page_id);
	
	void insert(PageInfo const& new_page,
		BeforeOrAfter before_or_after, PageId const& existing);
	
	void remove(ImageId const& image_id);
	
	/**
	 * \brief The bounding rectangle in scene coordinates of the current item.
	 *
	 * Returns a null rectangle if no item is currently selected.
	 */
	QRectF currentItemSceneRect() const;
signals:
	void pageSelected(
		PageInfo const& page_info, QRectF const& thumb_rect,
		bool by_user, bool was_already_selected);
	
	void contextMenuRequested(
		PageInfo const& page_info, QPoint const& screen_pos, bool selected);
private:
	class Item;
	class Impl;
	class PlaceholderThumb;
	class LabelGroup;
	class CompositeItem;
	template<typename Base> class NoSelectionItem;
	
	void emitPageSelected(
		PageInfo const& page_info, CompositeItem const* composite,
		bool by_user, bool was_already_selected);
		
	void emitContextMenuRequested(
		PageInfo const& page_info, QPoint const& screen_pos, bool selected);
	
	std::auto_ptr<Impl> m_ptrImpl;
};

#endif
