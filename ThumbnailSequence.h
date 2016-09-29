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

#ifndef THUMBNAILSEQUENCE_H_
#define THUMBNAILSEQUENCE_H_

#include "NonCopyable.h"
#include "FlagOps.h"
#include "IntrusivePtr.h"
#include "PageRange.h"
#include "PageOrderProvider.h"
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
class PageSequence;
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
		 * is undesirable.
		 */
		AVOID_SCROLLING_TO = 1 << 2
	};
	
	ThumbnailSequence(QSizeF const& max_logical_thumb_size);
	
	~ThumbnailSequence();
	
	void setThumbnailFactory(IntrusivePtr<ThumbnailFactory> const& factory);
	
	void attachView(QGraphicsView* view);
	
	/**
	 * \brief Re-populate the list of thumbnails.
	 *
	 * \param pages Pages to put in the sequence.
	 * \param selection_action Whether to keep the selection, provided
	 *        selected item(s) are still present in the new list of pages.
	 * \param order_provider The source of ordering information.  It will
	 *        be preserved until the next reset() call and will be taken
	 *        into account by other methods, like invalidateThumbnail()
	 *        and insert().  A null order provider indicates to keep the
	 *        order of ProjectPages.
	 */
	void reset(PageSequence const& pages,
		SelectionAction const selection_action,
		IntrusivePtr<PageOrderProvider const> const& order_provider
			= IntrusivePtr<PageOrderProvider const>());

	/** Returns the current page order provider, which may be null. */
	IntrusivePtr<PageOrderProvider const> pageOrderProvider() const;
	
	PageSequence toPageSequence() const;

	/**
	 * \brief Updates appearance and possibly position of a thumbnail.
	 *
	 * If thumbnail's size or position have changed and this thumbnail
	 * is a selection leader, newSelectionLeader() signal will be emitted
	 * with REDUNDANT_SELECTION flag set.
	 *
	 * \note This function assumes the thumbnail specified by page_id
	 *       is the only thumbnail at incorrect position.  If you do
	 *       something that changes the logical position of more than
	 *       one thumbnail at once, use invalidateAllThumbnails()
	 *       instead of sequentially calling invalidateThumbnail().
	 */
	void invalidateThumbnail(PageId const& page_id);

	/**
	 * This signature differs from invalidateThumbnail(PageId) in that
	 * it will cause PageInfo stored by ThumbnailSequence to be updated.
	 */
	void invalidateThumbnail(PageInfo const& page_info);
	
	/**
	 * \brief Updates appearance of all thumbnails and possibly their order.
	 *
	 * Whether or not order will be updated depends on whether an order provider
	 * was specified by the most recent reset() call.
	 */
	void invalidateAllThumbnails();
	
	/**
	 * \brief Makes the item a selection leader, and unselects other items.
	 *
	 * \param page_id The page to select.
	 * \return true on success, false if the requested page wasn't found.
	 *
	 * On success, the newSelectionLeader() signal is emitted, possibly
	 * with REDUNDANT_SELECTION flag set, in case our page was already the
	 * selection leader.
	 */
	bool setSelection(PageId const& page_id);

	/**
	 * \brief Returns the current selection leader.
	 *
	 * A null PageInfo is returned if no items are currently selected.
	 */
	PageInfo selectionLeader() const;

	/**
	 * \brief Returns the page immediately following the given one.
	 *
	 * A null PageInfo is returned if the given page wasn't found or
	 * there are no pages preceding it.
	 */
	PageInfo prevPage(PageId const& reference_page) const;

	/**
	 * \brief Returns the page immediately following the given one.
	 *
	 * A null PageInfo is returned if the given page wasn't found or
	 * there are no pages following it.
	 */
	PageInfo nextPage(PageId const& reference_page) const;
	
	/**
	 * \brief Returns the first page in the sequence.
	 *
	 * A null PageInfo is returned if the sequence is empty.
	 */
	PageInfo firstPage() const;

	/**
	 * \brief Returns the last page in the sequence.
	 *
	 * A null PageInfo is returned if the sequence is empty.
	 */
	PageInfo lastPage() const;

	/**
	 * \brief Inserts a page before the first page with matching ImageId.
	 *
	 * If no order provider was specified by the previous reset() call,
	 * we won't allow inserting a page between two halves of another page,
	 * to be compatible with what reset() does.  Otherwise, the new
	 * page will be inserted at a correct position according to the current
	 * order provider.  In this case \p before_or_after doesn't really matter.
	 *
	 * If there are no pages with matching ImageId, the new page won't
	 * be inserted, unless the request is to insert BEFORE a null ImageId(),
	 * which would cause insertion at the end.
	 */
	void insert(PageInfo const& new_page,
		BeforeOrAfter before_or_after, ImageId const& image);

	void removePages(std::set<PageId> const& pages);
	
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
	
	/**
	 * Emitted when a user right-clicks on a page thumbnail.
	 */
	void pageContextMenuRequested(
		PageInfo const& page_info, QPoint const& screen_pos, bool selected);

	/**
	 * Emitted when a user right clicks on area below the last page.
	 * In the absence of any pages, all the area is considered to be
	 * below the last page.
	 */
	void pastLastPageContextMenuRequested(QPoint const& screen_pos);
private:
	class Item;
	class Impl;
	class GraphicsScene;
	class PlaceholderThumb;
	class LabelGroup;
	class CompositeItem;
	
	void emitNewSelectionLeader(
		PageInfo const& page_info, CompositeItem const* composite,
		SelectionFlags flags);
	
	std::auto_ptr<Impl> m_ptrImpl;
};

DEFINE_FLAG_OPS(ThumbnailSequence::SelectionFlags)

#endif
