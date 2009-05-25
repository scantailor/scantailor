/*
    Scan Tailor - Interactive post-processing tool for scanned pages.
    Copyright (C) 2007-2009  Joseph Artsimovich <joseph_a@mail.ru>

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
#include "FlagOps.h"
#include "IntrusivePtr.h"
#include "PageRange.h"
#include "BeforeOrAfter.h"
#include <QObject>
#include <memory>
#include <vector>
#include <set>

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
	enum SelectionAction { KEEP_SELECTION, RESET_SELECTION };
	
	enum SelectionFlags {
		DEFAULT_SELECTION_FLAGS = 0,
		
		/** Indicates the item was selected by a user action, rather than programmatically. */
		SELECTED_BY_USER = 1 << 0,
		
		/**
		 * Indicates that the request to make this item a selection leader was redundant,
		 * as it's already a selection leader.
		 */
		REDUNDANT_SELECTION = 1 << 1,
		
		/**
		 * This flag is set when Ctrl-clicking the current selection leader while other
		 * selected items exist.  In this case, the leader will become unselected, and
		 * one of the other selected items will be promoted to a selection leader.
		 * In these circumstances, scrolling to make the new selection leader visible
		 * is undesireable.
		 */
		AVOID_SCROLLING_TO = 1 << 2
	};
	
	ThumbnailSequence(QSizeF const& max_logical_thumb_size);
	
	~ThumbnailSequence();
	
	void setThumbnailFactory(IntrusivePtr<ThumbnailFactory> const& factory);
	
	void attachView(QGraphicsView* view);
	
	void reset(PageSequenceSnapshot const& pages,
		SelectionAction const selection_action);
	
	void invalidateThumbnail(PageId const& page_id);
	
	void invalidateThumbnail(ImageId const& image_id);
	
	void invalidateAllThumbnails();
	
	/**
	 * \brief Makes the item a selection leader, and unselects the other items.
	 */
	void setSelection(PageId const& page_id);
	
	void insert(PageInfo const& new_page,
		BeforeOrAfter before_or_after, PageId const& existing);
	
	void remove(ImageId const& image_id);
	
	/**
	 * \brief The bounding rectangle in scene coordinates of the selection leader.
	 *
	 * Returns a null rectangle if no item is currently selected.
	 */
	QRectF selectionLeaderSceneRect() const;
	
	std::set<PageId> selectedItems() const;
	
	std::vector<PageRange> selectedRanges() const;
signals:
	void newSelectionLeader(
		PageInfo const& page_info, QRectF const& thumb_rect,
		ThumbnailSequence::SelectionFlags flags);
	
	void contextMenuRequested(
		PageInfo const& page_info, QPoint const& screen_pos, bool selected);
private:
	class Item;
	class Impl;
	class PlaceholderThumb;
	class LabelGroup;
	class CompositeItem;
	
	void emitNewSelectionLeader(
		PageInfo const& page_info, CompositeItem const* composite,
		SelectionFlags flags);
		
	void emitContextMenuRequested(
		PageInfo const& page_info, QPoint const& screen_pos, bool selected);
	
	std::auto_ptr<Impl> m_ptrImpl;
};

DEFINE_FLAG_OPS(ThumbnailSequence::SelectionFlags)

#endif
