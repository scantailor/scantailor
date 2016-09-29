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

#include "ThumbnailSequence.h"
#include "ThumbnailSequence.h.moc"
#include "ThumbnailFactory.h"
#include "IncompleteThumbnail.h"
#include "PageSequence.h"
#include "PageOrderProvider.h"
#include "PageInfo.h"
#include "PageId.h"
#include "ImageId.h"
#include "RefCountable.h"
#include "IntrusivePtr.h"
#include "ScopedIncDec.h"
#ifndef Q_MOC_RUN
#include <boost/multi_index_container.hpp>
#include <boost/multi_index/ordered_index.hpp>
#include <boost/multi_index/sequenced_index.hpp>
#include <boost/multi_index/mem_fun.hpp>
#include <boost/function.hpp>
#include <boost/lambda/lambda.hpp>
#include <boost/lambda/bind.hpp>
#include <boost/foreach.hpp>
#endif
#include <QGraphicsScene>
#include <QGraphicsItem>
#include <QGraphicsItemGroup>
#include <QGraphicsSimpleTextItem>
#include <QGraphicsPixmapItem>
#include <QGraphicsView>
#include <QStyle>
#include <QStyleOptionGraphicsItem>
#include <QGraphicsSceneMouseEvent>
#include <QGraphicsSceneContextMenuEvent>
#include <QPalette>
#include <QFont>
#include <QApplication>
#include <QVariant>
#include <QFileInfo>
#include <QPixmap>
#include <QRectF>
#include <QSizeF>
#include <QPointF>
#include <QPainter>
#include <QPainterPath>
#include <QTransform>
#include <QPen>
#include <QBrush>
#include <QColor>
#include <QString>
#include <QObject>
#include <QCursor>
#include <Qt>
#include <QDebug>
#include <algorithm>
#include <stddef.h>
#include <assert.h>

using namespace ::boost::multi_index;
using namespace ::boost::lambda;


class ThumbnailSequence::Item
{
public:
	Item(PageInfo const& page_info, CompositeItem* comp_item);
	
	PageId const& pageId() const { return pageInfo.id(); }
	
	bool isSelected() const { return m_isSelected; }
	
	bool isSelectionLeader() const { return m_isSelectionLeader; }
	
	void setSelected(bool selected) const;
	
	void setSelectionLeader(bool selection_leader) const;
	
	PageInfo pageInfo;
	mutable CompositeItem* composite;
	mutable bool incompleteThumbnail;
private:
	mutable bool m_isSelected;
	mutable bool m_isSelectionLeader;
};


class ThumbnailSequence::GraphicsScene : public QGraphicsScene
{
public:
	typedef boost::function<void (QGraphicsSceneContextMenuEvent*)> ContextMenuEventCallback;

	void setContextMenuEventCallback(ContextMenuEventCallback callback) {
		m_contextMenuEventCallback = callback;
	}
protected:
	virtual void contextMenuEvent(QGraphicsSceneContextMenuEvent* event) {
		QGraphicsScene::contextMenuEvent(event);
		
		if (!event->isAccepted() && m_contextMenuEventCallback) {
			m_contextMenuEventCallback(event);
		}
	}
private:
	ContextMenuEventCallback m_contextMenuEventCallback;
};


class ThumbnailSequence::Impl
{
public:
	Impl(ThumbnailSequence& owner, QSizeF const& max_logical_thumb_size);
	
	~Impl();
	
	void setThumbnailFactory(IntrusivePtr<ThumbnailFactory> const& factory);

	void attachView(QGraphicsView* view);
	
	void reset(PageSequence const& pages,
		SelectionAction const selection_action,
		IntrusivePtr<PageOrderProvider const> const& provider);

	IntrusivePtr<PageOrderProvider const> pageOrderProvider() const;

	PageSequence toPageSequence() const;
	
	void invalidateThumbnail(PageId const& page_id);

	void invalidateThumbnail(PageInfo const& page_info);
	
	void invalidateAllThumbnails();
	
	bool setSelection(PageId const& page_id);

	PageInfo selectionLeader() const;

	PageInfo prevPage(PageId const& page_id) const;

	PageInfo nextPage(PageId const& page_id) const;

	PageInfo firstPage() const;

	PageInfo lastPage() const;

	void insert(PageInfo const& new_page,
		BeforeOrAfter before_or_after, ImageId const& image);

	void removePages(std::set<PageId> const& pages);
	
	QRectF selectionLeaderSceneRect() const;
	
	std::set<PageId> selectedItems() const;
	
	std::vector<PageRange> selectedRanges() const;
	
	void contextMenuRequested(
		PageInfo const& page_info, QPoint const& screen_pos, bool selected);
		
	void itemSelectedByUser(CompositeItem* item, Qt::KeyboardModifiers modifiers);
private:
	class ItemsByIdTag;
	class ItemsInOrderTag;
	class SelectedThenUnselectedTag;
	
	typedef multi_index_container<
		Item,
		indexed_by<
			ordered_unique<
				tag<ItemsByIdTag>,
				const_mem_fun<Item, PageId const&, &Item::pageId>
			>,
			sequenced<tag<ItemsInOrderTag> >,
			sequenced<tag<SelectedThenUnselectedTag> >
		>
	> Container;
	
	typedef Container::index<ItemsByIdTag>::type ItemsById;
	typedef Container::index<ItemsInOrderTag>::type ItemsInOrder;
	typedef Container::index<SelectedThenUnselectedTag>::type SelectedThenUnselected;
	
	void invalidateThumbnailImpl(ItemsById::iterator id_it);

	void sceneContextMenuEvent(QGraphicsSceneContextMenuEvent* evt);

	void selectItemNoModifiers(ItemsById::iterator const& it);
	
	void selectItemWithControl(ItemsById::iterator const& it);
	
	void selectItemWithShift(ItemsById::iterator const& it);
	
	bool multipleItemsSelected() const;
	
	void moveToSelected(Item const* item);
	
	void moveToUnselected(Item const* item);
	
	void clear();
	
	void clearSelection();

	/**
	 * Calculates the insertion position for an item with the given PageId
	 * based on m_ptrOrderProvider.
	 *
	 * \param begin Beginning of the interval to consider.
	 * \param end End of the interval to consider.
	 * \param page_id The item to find insertion position for.
	 * \param page_incomplete Whether the page is represented by IncompleteThumbnail.
	 * \param hint The place to start the search.  Must be within [begin, end].
	 * \param dist_from_hint If provided, the distance from \p hint
	 *        to the calculated insertion position will be written there.
	 *        For example, \p dist_from_hint == -2 would indicate that the
	 *        insertion position is two elements to the left of \p hint.
	 */
	ItemsInOrder::iterator itemInsertPosition(
		ItemsInOrder::iterator begin, ItemsInOrder::iterator end,
		PageId const& page_id, bool page_incomplete,
		ItemsInOrder::iterator hint, int* dist_from_hint = 0);
	
	std::auto_ptr<QGraphicsItem> getThumbnail(PageInfo const& page_info);
	
	std::auto_ptr<LabelGroup> getLabelGroup(PageInfo const& page_info);
	
	std::auto_ptr<CompositeItem> getCompositeItem(
		Item const* item, PageInfo const& info);
	
	void commitSceneRect();
	
	static int const SPACING = 10;
	ThumbnailSequence& m_rOwner;
	QSizeF m_maxLogicalThumbSize;
	Container m_items;
	ItemsById& m_itemsById;
	ItemsInOrder& m_itemsInOrder;
	
	/**
	 * As the name implies, selected items go first here (in no particular order),
	 * then go unselected items (also in no particular order).
	 */
	SelectedThenUnselected& m_selectedThenUnselected;
	
	Item const* m_pSelectionLeader;
	IntrusivePtr<ThumbnailFactory> m_ptrFactory;
	IntrusivePtr<PageOrderProvider const> m_ptrOrderProvider;
	GraphicsScene m_graphicsScene;
	QRectF m_sceneRect;
};


class ThumbnailSequence::PlaceholderThumb : public QGraphicsItem
{
public:
	PlaceholderThumb(QSizeF const& max_size);
	
	virtual QRectF boundingRect() const;
	
	virtual void paint(QPainter* painter,
		QStyleOptionGraphicsItem const* option, QWidget *widget);
private:
	static QPainterPath m_sCachedPath;
	QSizeF m_maxSize;
};


class ThumbnailSequence::LabelGroup : public QGraphicsItemGroup
{
public:
	LabelGroup(std::auto_ptr<QGraphicsSimpleTextItem> label);
	
	LabelGroup(
		std::auto_ptr<QGraphicsSimpleTextItem> normal_label,
		std::auto_ptr<QGraphicsSimpleTextItem> bold_label,
		std::auto_ptr<QGraphicsPixmapItem> pixmap = std::auto_ptr<QGraphicsPixmapItem>());
	
	void updateAppearence(bool selected, bool selection_leader);
private:
	QGraphicsSimpleTextItem* m_pNormalLabel;
	QGraphicsSimpleTextItem* m_pBoldLabel;
};


class ThumbnailSequence::CompositeItem : public QGraphicsItemGroup
{
public:
	CompositeItem(
		ThumbnailSequence::Impl& owner,
		std::auto_ptr<QGraphicsItem> thumbnail,
		std::auto_ptr<LabelGroup> label_group);
	
	void setItem(Item const* item) { m_pItem = item; }
	
	Item const* item() { return m_pItem; }

	bool incompleteThumbnail() const;
	
	void updateSceneRect(QRectF& scene_rect);
	
	void updateAppearence(bool selected, bool selection_leader);
	
	virtual QRectF boundingRect() const;
	
	virtual void paint(QPainter* painter,
		QStyleOptionGraphicsItem const* option, QWidget *widget);
protected:
	virtual void contextMenuEvent(QGraphicsSceneContextMenuEvent* event);
	
	virtual void mousePressEvent(QGraphicsSceneMouseEvent* event);
private:
	// We no longer use QGraphicsView's selection mechanism, so we
	// shadow isSelected() and setSelected() with unimplemented private
	// functions.  Just to be safe.
	bool isSelected() const;
	
	void setSelected(bool selected);
	
	ThumbnailSequence::Impl& m_rOwner;
	ThumbnailSequence::Item const* m_pItem;
	QGraphicsItem* m_pThumb;
	LabelGroup* m_pLabelGroup;
};


/*============================= ThumbnailSequence ===========================*/

ThumbnailSequence::ThumbnailSequence(QSizeF const& max_logical_thumb_size)
:	m_ptrImpl(new Impl(*this, max_logical_thumb_size))
{
}

ThumbnailSequence::~ThumbnailSequence()
{
}

void
ThumbnailSequence::setThumbnailFactory(IntrusivePtr<ThumbnailFactory> const& factory)
{
	m_ptrImpl->setThumbnailFactory(factory);
}

void
ThumbnailSequence::attachView(QGraphicsView* const view)
{
	m_ptrImpl->attachView(view);
}

void
ThumbnailSequence::reset(
	PageSequence const& pages,
	SelectionAction const selection_action,
	IntrusivePtr<PageOrderProvider const> const& order_provider)
{
	m_ptrImpl->reset(pages, selection_action, order_provider);
}

IntrusivePtr<PageOrderProvider const>
ThumbnailSequence::pageOrderProvider() const
{
	return m_ptrImpl->pageOrderProvider();
}

PageSequence
ThumbnailSequence::toPageSequence() const
{
	return m_ptrImpl->toPageSequence();
}

void
ThumbnailSequence::invalidateThumbnail(PageId const& page_id)
{
	m_ptrImpl->invalidateThumbnail(page_id);
}

void
ThumbnailSequence::invalidateThumbnail(PageInfo const& page_info)
{
	m_ptrImpl->invalidateThumbnail(page_info);
}

void
ThumbnailSequence::invalidateAllThumbnails()
{
	m_ptrImpl->invalidateAllThumbnails();
}

bool
ThumbnailSequence::setSelection(PageId const& page_id)
{
	return m_ptrImpl->setSelection(page_id);
}

PageInfo
ThumbnailSequence::selectionLeader() const
{
	return m_ptrImpl->selectionLeader();
}

PageInfo
ThumbnailSequence::prevPage(PageId const& reference_page) const
{
	return m_ptrImpl->prevPage(reference_page);
}

PageInfo
ThumbnailSequence::nextPage(PageId const& reference_page) const
{
	return m_ptrImpl->nextPage(reference_page);
}

PageInfo
ThumbnailSequence::firstPage() const
{
	return m_ptrImpl->firstPage();
}

PageInfo
ThumbnailSequence::lastPage() const
{
	return m_ptrImpl->lastPage();
}

void
ThumbnailSequence::insert(
	PageInfo const& new_page,
	BeforeOrAfter before_or_after, ImageId const& image)
{
	m_ptrImpl->insert(new_page, before_or_after, image);
}

void
ThumbnailSequence::removePages(std::set<PageId> const& pages)
{
	m_ptrImpl->removePages(pages);
}

QRectF
ThumbnailSequence::selectionLeaderSceneRect() const
{
	return m_ptrImpl->selectionLeaderSceneRect();
}

std::set<PageId>
ThumbnailSequence::selectedItems() const
{
	return m_ptrImpl->selectedItems();
}

std::vector<PageRange>
ThumbnailSequence::selectedRanges() const
{
	return m_ptrImpl->selectedRanges();
}

void
ThumbnailSequence::emitNewSelectionLeader(
	PageInfo const& page_info, CompositeItem const* composite,
	SelectionFlags const flags)
{
	QRectF const thumb_rect(
		composite->mapToScene(composite->boundingRect()).boundingRect()
	);
	emit newSelectionLeader(page_info, thumb_rect, flags);
}


/*======================== ThumbnailSequence::Impl ==========================*/

ThumbnailSequence::Impl::Impl(
	ThumbnailSequence& owner, QSizeF const& max_logical_thumb_size)
:	m_rOwner(owner),
	m_maxLogicalThumbSize(max_logical_thumb_size),
	m_items(),
	m_itemsById(m_items.get<ItemsByIdTag>()),
	m_itemsInOrder(m_items.get<ItemsInOrderTag>()),
	m_selectedThenUnselected(m_items.get<SelectedThenUnselectedTag>()),
	m_pSelectionLeader(0)
{
	m_graphicsScene.setContextMenuEventCallback(
		boost::lambda::bind(&Impl::sceneContextMenuEvent, this, boost::lambda::_1)
	);
}

ThumbnailSequence::Impl::~Impl()
{
}

void
ThumbnailSequence::Impl::setThumbnailFactory(
	IntrusivePtr<ThumbnailFactory> const& factory)
{
	m_ptrFactory = factory;
}

void
ThumbnailSequence::Impl::attachView(QGraphicsView* const view)
{
	view->setScene(&m_graphicsScene);
}

void
ThumbnailSequence::Impl::reset(
	PageSequence const& pages,
	SelectionAction const selection_action,
	IntrusivePtr<PageOrderProvider const> const& order_provider)
{
	m_ptrOrderProvider = order_provider;

	std::set<PageId> selected;
	PageInfo selection_leader;
	
	if (selection_action == KEEP_SELECTION) {
		selectedItems().swap(selected);
		if (m_pSelectionLeader) {
			selection_leader = m_pSelectionLeader->pageInfo;
		}
	}
	
	clear(); // Also clears the selection.
	
	size_t const num_pages = pages.numPages();
	if (num_pages == 0) {
		return;
	}

	Item const* some_selected_item = 0;

	for (size_t i = 0; i < num_pages; ++i) {
		PageInfo const& page_info(pages.pageAt(i));
		
		std::auto_ptr<CompositeItem> composite(getCompositeItem(0, page_info));
		m_itemsInOrder.push_back(Item(page_info, composite.release()));
		Item const* item = &m_itemsInOrder.back();
		item->composite->setItem(item);

		if (selected.find(page_info.id()) != selected.end()) {
			item->setSelected(true);
			moveToSelected(item);
			some_selected_item = item;
		}
		if (page_info.id() == selection_leader.id()) {
			m_pSelectionLeader = item;
		}
	}

	invalidateAllThumbnails();
	
	if (!m_pSelectionLeader) {
		if (some_selected_item) {
			m_pSelectionLeader = some_selected_item;
		}
	}
	
	if (m_pSelectionLeader) {
		m_pSelectionLeader->setSelectionLeader(true);
		m_rOwner.emitNewSelectionLeader(
			selection_leader, m_pSelectionLeader->composite, DEFAULT_SELECTION_FLAGS
		);
	}
}

IntrusivePtr<PageOrderProvider const>
ThumbnailSequence::Impl::pageOrderProvider() const
{
	return m_ptrOrderProvider;
}

PageSequence
ThumbnailSequence::Impl::toPageSequence() const
{
	PageSequence pages;

	BOOST_FOREACH(Item const& item, m_itemsInOrder) {
		pages.append(item.pageInfo);
	}

	return pages;
}

void
ThumbnailSequence::Impl::invalidateThumbnail(PageId const& page_id)
{
	ItemsById::iterator const id_it(m_itemsById.find(page_id));
	if (id_it != m_itemsById.end()) {
		invalidateThumbnailImpl(id_it);
	}
}

void
ThumbnailSequence::Impl::invalidateThumbnail(PageInfo const& page_info)
{
	ItemsById::iterator const id_it(m_itemsById.find(page_info.id()));
	if (id_it != m_itemsById.end()) {
		m_itemsById.modify(id_it, boost::lambda::bind(&Item::pageInfo, boost::lambda::_1) = page_info);
		invalidateThumbnailImpl(id_it);
	}
}

void
ThumbnailSequence::Impl::invalidateThumbnailImpl(ItemsById::iterator const id_it)
{
	std::auto_ptr<CompositeItem> composite(
		getCompositeItem(&*id_it, id_it->pageInfo)
	);
	CompositeItem* const new_composite = composite.get();
	CompositeItem* const old_composite = id_it->composite;
	QSizeF const old_size(old_composite->boundingRect().size());
	QSizeF const new_size(new_composite->boundingRect().size());
	QPointF const old_pos(new_composite->pos());
	
	new_composite->updateAppearence(id_it->isSelected(), id_it->isSelectionLeader());
	
	m_graphicsScene.addItem(composite.release());
	id_it->composite = new_composite;
	id_it->incompleteThumbnail = new_composite->incompleteThumbnail();
	delete old_composite;
	
	ItemsInOrder::iterator after_old(m_items.project<ItemsInOrderTag>(id_it));
	// Notice after_old++ below.

	// Move our item to the beginning of m_itemsInOrder, to make it out of range
	// we are going to pass to itemInsertPosition().
	m_itemsInOrder.relocate(m_itemsInOrder.begin(), after_old++);

	int dist = 0;
	ItemsInOrder::iterator const after_new(
		itemInsertPosition(
			++m_itemsInOrder.begin(), m_itemsInOrder.end(),
			id_it->pageInfo.id(), id_it->incompleteThumbnail,
			after_old, &dist
		)
	);

	// Move our item to its intended position.
	m_itemsInOrder.relocate(after_new, m_itemsInOrder.begin());


	// Now let's reposition the items on the scene.
	
	ItemsInOrder::iterator ord_it, ord_end;

	// The range of [ord_it, ord_end) is supposed to contain all items
	// between the old and new positions of our item, with the new
	// position in range.

	if (dist <= 0) { // New position is before or equals to the old one.
		ord_it = after_new;
		--ord_it; // Include new item position in the range.
		ord_end = after_old;
	} else { // New position is after the old one.
		ord_it = after_old;
		ord_end = after_new;
	}
	
	double offset = 0;

	if (ord_it != m_itemsInOrder.begin()) {
		ItemsInOrder::iterator prev(ord_it);
		--prev;
		offset = prev->composite->pos().y() + prev->composite->boundingRect().height() + SPACING;
	}

	// Reposition items between the old and the new position of our item,
	// including the item itself.
	for (; ord_it != ord_end; ++ord_it) {
		ord_it->composite->setPos(0.0, offset);
		offset += ord_it->composite->boundingRect().height() + SPACING;
	}

	// Reposition the items following both the new and the old position
	// of the item, if the item size has changed.
	if (old_size != new_size) {
		for (; ord_it != m_itemsInOrder.end(); ++ord_it) {
			ord_it->composite->setPos(0.0, offset);
			offset += ord_it->composite->boundingRect().height() + SPACING;
		}
	}
	
	// Update scene rect.
	m_sceneRect.setTop(m_sceneRect.bottom());
	m_itemsInOrder.front().composite->updateSceneRect(m_sceneRect);
	m_sceneRect.setBottom(m_sceneRect.top());
	m_itemsInOrder.back().composite->updateSceneRect(m_sceneRect);
	id_it->composite->updateSceneRect(m_sceneRect);
	commitSceneRect();

	// Possibly emit the newSelectionLeader() signal.
	if (m_pSelectionLeader == &*id_it) {
		if (old_size != new_size || old_pos != id_it->composite->pos()) {
			m_rOwner.emitNewSelectionLeader(
				id_it->pageInfo, id_it->composite, REDUNDANT_SELECTION
			);
		}
	}
}

void
ThumbnailSequence::Impl::invalidateAllThumbnails()
{
	// Recreate thumbnails now, whether a thumbnail is incomplete
	// is taken into account when sorting.
	ItemsInOrder::iterator ord_it(m_itemsInOrder.begin());
	ItemsInOrder::iterator const ord_end(m_itemsInOrder.end());
	for (; ord_it != ord_end; ++ord_it) {
		CompositeItem* const old_composite = ord_it->composite;
		ord_it->composite = getCompositeItem(&*ord_it, ord_it->pageInfo).release();
		ord_it->incompleteThumbnail = ord_it->composite->incompleteThumbnail();
		delete old_composite;
	}

	// Sort pages in m_itemsInOrder using m_ptrOrderProvider.
	if (m_ptrOrderProvider.get()) {
		m_itemsInOrder.sort(
			boost::lambda::bind(
				&PageOrderProvider::precedes, m_ptrOrderProvider.get(),
				boost::lambda::bind(&Item::pageId, boost::lambda::_1), bind(&Item::incompleteThumbnail, boost::lambda::_1),
				boost::lambda::bind(&Item::pageId, boost::lambda::_2), bind(&Item::incompleteThumbnail, boost::lambda::_2)
			)
		);
	}
	
	m_sceneRect = QRectF(0.0, 0.0, 0.0, 0.0);
	double offset = 0;

	for (ord_it = m_itemsInOrder.begin(); ord_it != ord_end; ++ord_it) {
		CompositeItem* composite = ord_it->composite;
		composite->setPos(0.0, offset);
		composite->updateSceneRect(m_sceneRect);
		composite->updateAppearence(ord_it->isSelected(), ord_it->isSelectionLeader());

		offset += composite->boundingRect().height() + SPACING;
		m_graphicsScene.addItem(composite);
	}
	
	commitSceneRect();
}

bool
ThumbnailSequence::Impl::setSelection(PageId const& page_id)
{
	ItemsById::iterator const id_it(m_itemsById.find(page_id));
	if (id_it == m_itemsById.end()) {
		return false;
	}
	
	bool const was_selection_leader = (&*id_it == m_pSelectionLeader);
	
	// Clear selection from all items except the one for which
	// selection is requested.
	SelectedThenUnselected::iterator it(m_selectedThenUnselected.begin());
	while (it != m_selectedThenUnselected.end()) {
		Item const& item = *it;
		if (!item.isSelected()) {
			break;
		}

		++it;

		if (&*id_it != &item) {
			item.setSelected(false);
			moveToUnselected(&item);
			if (m_pSelectionLeader == &item) {
				m_pSelectionLeader = 0;
			}
		}
	}
	
	if (!was_selection_leader) {
		m_pSelectionLeader = &*id_it;
		m_pSelectionLeader->setSelectionLeader(true);
		moveToSelected(m_pSelectionLeader);
	}
	
	SelectionFlags flags = DEFAULT_SELECTION_FLAGS;
	if (was_selection_leader) {
		flags |= REDUNDANT_SELECTION;
	}
	
	m_rOwner.emitNewSelectionLeader(id_it->pageInfo, id_it->composite, flags);

	return true;
}

PageInfo
ThumbnailSequence::Impl::selectionLeader() const
{
	if (m_pSelectionLeader) {
		return m_pSelectionLeader->pageInfo;
	} else {
		return PageInfo();
	}
}

PageInfo
ThumbnailSequence::Impl::prevPage(PageId const& reference_page) const
{
	ItemsInOrder::iterator ord_it;

	if (m_pSelectionLeader && m_pSelectionLeader->pageInfo.id() == reference_page) {
		// Common case optimization.
		ord_it = m_itemsInOrder.iterator_to(*m_pSelectionLeader);
	} else {
		ord_it = m_items.project<ItemsInOrderTag>(m_itemsById.find(reference_page));
	}

	if (ord_it != m_itemsInOrder.end()) {
		if (ord_it != m_itemsInOrder.begin()) {
			--ord_it;
			return ord_it->pageInfo;
		}
	}

	return PageInfo();
}

PageInfo
ThumbnailSequence::Impl::nextPage(PageId const& reference_page) const
{
	ItemsInOrder::iterator ord_it;

	if (m_pSelectionLeader && m_pSelectionLeader->pageInfo.id() == reference_page) {
		// Common case optimization.
		ord_it = m_itemsInOrder.iterator_to(*m_pSelectionLeader);
	} else {
		ord_it = m_items.project<ItemsInOrderTag>(m_itemsById.find(reference_page));
	}

	if (ord_it != m_itemsInOrder.end()) {
		++ord_it;
		if (ord_it != m_itemsInOrder.end()) {
			return ord_it->pageInfo;
		}
	}

	return PageInfo();
}

PageInfo
ThumbnailSequence::Impl::firstPage() const
{
	if (m_items.empty()) {
		return PageInfo();
	}

	return m_itemsInOrder.front().pageInfo;
}

PageInfo
ThumbnailSequence::Impl::lastPage() const
{
	if (m_items.empty()) {
		return PageInfo();
	}

	return m_itemsInOrder.back().pageInfo;
}

void
ThumbnailSequence::Impl::insert(
	PageInfo const& page_info,
	BeforeOrAfter before_or_after, ImageId const& image)
{
	ItemsInOrder::iterator ord_it;

	if (before_or_after == BEFORE && image.isNull()) {
		ord_it = m_itemsInOrder.end();
	} else {
		// Note that we have to use lower_bound() rather than find() because
		// we are not searching for PageId(image) exactly, which implies
		// PageId::SINGLE_PAGE configuration, but rather we search for
		// a page with any configuration, as long as it references the same image. 
		ItemsById::iterator id_it(m_itemsById.lower_bound(PageId(image)));
		if (id_it == m_itemsById.end() || id_it->pageInfo.imageId() != image) {
			// Reference page not found.
			return;
		}
		
		ord_it = m_items.project<ItemsInOrderTag>(id_it);
		
		if (before_or_after == AFTER) {
			++ord_it;
			if (!m_ptrOrderProvider.get()) { 
				// Advance past not only the target page, but also its other half, if it follows.
				while (ord_it != m_itemsInOrder.end() && ord_it->pageInfo.imageId() == image) {
					++ord_it;
				}
			}
		}
	}

	// If m_ptrOrderProvider is not set, ord_it won't change.
	ord_it = itemInsertPosition(
		m_itemsInOrder.begin(), m_itemsInOrder.end(), page_info.id(),
		/*page_incomplete=*/true, ord_it
	);
	
	double offset = 0.0;
	if (!m_items.empty()) {
		if (ord_it != m_itemsInOrder.end()) {
			offset = ord_it->composite->pos().y();
		} else {
			ItemsInOrder::iterator it(ord_it);
			--it;
			offset = it->composite->y()
				+ it->composite->boundingRect().height() + SPACING;
		}
	}
	std::auto_ptr<CompositeItem> composite(
		getCompositeItem(0, page_info)
	);
	composite->setPos(0.0, offset);
	composite->updateSceneRect(m_sceneRect);
	
	QPointF const pos_delta(0.0, composite->boundingRect().height() + SPACING);
	
	Item const item(page_info, composite.get());
	std::pair<ItemsInOrder::iterator, bool> const ins(
		m_itemsInOrder.insert(ord_it, item)
	);
	composite->setItem(&*ins.first);
	m_graphicsScene.addItem(composite.release());
	
	ItemsInOrder::iterator const ord_end(m_itemsInOrder.end());
	for (; ord_it != ord_end; ++ord_it) {
		ord_it->composite->setPos(ord_it->composite->pos() + pos_delta);
		ord_it->composite->updateSceneRect(m_sceneRect);
	}
	
	commitSceneRect();
}

void
ThumbnailSequence::Impl::removePages(std::set<PageId> const& to_remove)
{
	m_sceneRect = QRectF(0, 0, 0, 0);

	std::set<PageId>::const_iterator const to_remove_end(to_remove.end());
	QPointF pos_delta(0, 0);

	ItemsInOrder::iterator ord_it(m_itemsInOrder.begin());
	ItemsInOrder::iterator const ord_end(m_itemsInOrder.end());
	while (ord_it != ord_end) {
		if (to_remove.find(ord_it->pageInfo.id()) == to_remove_end) {
			// Keeping this page.
			if (pos_delta != QPointF(0, 0)) {
				ord_it->composite->setPos(ord_it->composite->pos() + pos_delta);
			}
			ord_it->composite->updateSceneRect(m_sceneRect);
			++ord_it;
		} else {
			// Removing this page.
			if (m_pSelectionLeader == &*ord_it) {
				m_pSelectionLeader = 0;
			}
			pos_delta.ry() -= ord_it->composite->boundingRect().height() + SPACING;
			delete ord_it->composite;
			m_itemsInOrder.erase(ord_it++);
		}
	}

	commitSceneRect();
}

bool
ThumbnailSequence::Impl::multipleItemsSelected() const
{
	SelectedThenUnselected::iterator it(m_selectedThenUnselected.begin());
	SelectedThenUnselected::iterator const end(m_selectedThenUnselected.end());
	for (int i = 0; i < 2; ++i, ++it) {
		if (it == end || !it->isSelected()) {
			return false;
		}
	}
	return true;
}

void
ThumbnailSequence::Impl::moveToSelected(Item const* item)
{
	m_selectedThenUnselected.relocate(
		m_selectedThenUnselected.begin(),
		m_selectedThenUnselected.iterator_to(*item)
	);
}

void
ThumbnailSequence::Impl::moveToUnselected(Item const* item)
{
	m_selectedThenUnselected.relocate(
		m_selectedThenUnselected.end(),
		m_selectedThenUnselected.iterator_to(*item)
	);
}

QRectF
ThumbnailSequence::Impl::selectionLeaderSceneRect() const
{
	if (!m_pSelectionLeader) {
		return QRectF();
	}
	
	return m_pSelectionLeader->composite->mapToScene(
		m_pSelectionLeader->composite->boundingRect()
	).boundingRect();
}

std::set<PageId>
ThumbnailSequence::Impl::selectedItems() const
{
	std::set<PageId> selection;
	BOOST_FOREACH(Item const& item, m_selectedThenUnselected) {
		if (!item.isSelected()) {
			break;
		}
		selection.insert(item.pageInfo.id());
	}
	return selection;
}

std::vector<PageRange>
ThumbnailSequence::Impl::selectedRanges() const
{
	std::vector<PageRange> ranges;
	
	ItemsInOrder::iterator it(m_itemsInOrder.begin());
	ItemsInOrder::iterator const end(m_itemsInOrder.end());
	for (;;) {
		for (; it != end && !it->isSelected(); ++it) {
			// Skip unselected items.
		}
		if (it == end) {
			break;
		}
		
		ranges.push_back(PageRange());
		PageRange& range = ranges.back();
		for (; it != end && it->isSelected(); ++it) {
			range.pages.push_back(it->pageInfo.id());
		}
	}
	
	return ranges;
}

void
ThumbnailSequence::Impl::contextMenuRequested(
	PageInfo const& page_info, QPoint const& screen_pos, bool selected)
{
	emit m_rOwner.pageContextMenuRequested(page_info, screen_pos, selected);
}

void
ThumbnailSequence::Impl::sceneContextMenuEvent(QGraphicsSceneContextMenuEvent* evt)
{
	if (!m_itemsInOrder.empty()) {
		CompositeItem* composite = m_itemsInOrder.back().composite;
		QRectF const last_thumb_rect(
			composite->mapToScene(composite->boundingRect()).boundingRect()
		);
		if (evt->scenePos().y() <= last_thumb_rect.bottom()) {
			return;
		}
	}

	emit m_rOwner.pastLastPageContextMenuRequested(evt->screenPos());
}

void
ThumbnailSequence::Impl::itemSelectedByUser(
	CompositeItem* composite, Qt::KeyboardModifiers const modifiers)
{
	ItemsById::iterator const id_it(m_itemsById.iterator_to(*composite->item()));
	
	if (modifiers & Qt::ControlModifier) {
		selectItemWithControl(id_it);
	} else if (modifiers & Qt::ShiftModifier) {
		selectItemWithShift(id_it);
	} else {
		selectItemNoModifiers(id_it);
	}
}

void
ThumbnailSequence::Impl::selectItemWithControl(ItemsById::iterator const& id_it)
{
	SelectionFlags flags = SELECTED_BY_USER;
	
	if (!id_it->isSelected()) {
		if (m_pSelectionLeader) {
			m_pSelectionLeader->setSelectionLeader(false);
		}
		m_pSelectionLeader = &*id_it;
		m_pSelectionLeader->setSelectionLeader(true);
		moveToSelected(m_pSelectionLeader);
		
		m_rOwner.emitNewSelectionLeader(
			m_pSelectionLeader->pageInfo,
			m_pSelectionLeader->composite, flags
		);
		return;
	}

	if (!multipleItemsSelected()) {
		// Clicked on the only selected item.
		flags |= REDUNDANT_SELECTION;
		m_rOwner.emitNewSelectionLeader(
			m_pSelectionLeader->pageInfo,
			m_pSelectionLeader->composite, flags
		);
		return;
	}
	
	// Unselect it.
	id_it->setSelected(false);
	moveToUnselected(&*id_it);
	
	if (m_pSelectionLeader != &*id_it) {
		// The selection leader remains the same - we are done.
		return;
	}
	
	// Select the new selection leader among other selected items.
	m_pSelectionLeader = 0;
	flags |= AVOID_SCROLLING_TO;
	ItemsInOrder::iterator ord_it1(m_items.project<ItemsInOrderTag>(id_it));
	ItemsInOrder::iterator ord_it2(ord_it1);
	for (;;) {
		if (ord_it1 != m_itemsInOrder.begin()) {
			--ord_it1;
			if (ord_it1->isSelected()) {
				m_pSelectionLeader = &*ord_it1;
				break;
			}
		}
		if (ord_it2 != m_itemsInOrder.end()) {
			++ord_it2;
			if (ord_it2 != m_itemsInOrder.end()) {
				if (ord_it2->isSelected()) {
					m_pSelectionLeader = &*ord_it2;
					break;
				}
			}
		}
	}
	assert(m_pSelectionLeader); // We had multiple selected items.

	m_pSelectionLeader->setSelectionLeader(true);
	// No need to moveToSelected() as it was and remains selected.
	
	m_rOwner.emitNewSelectionLeader(
		m_pSelectionLeader->pageInfo, m_pSelectionLeader->composite, flags
	);
}

void
ThumbnailSequence::Impl::selectItemWithShift(ItemsById::iterator const& id_it)
{
	if (!m_pSelectionLeader) {
		selectItemNoModifiers(id_it);
		return;
	}
	
	SelectionFlags flags = SELECTED_BY_USER;
	if (m_pSelectionLeader == &*id_it) {
		flags |= REDUNDANT_SELECTION;
	}
	
	// Select all the items between the selection leader and the item that was clicked.
	ItemsInOrder::iterator endpoint1(m_itemsInOrder.iterator_to(*m_pSelectionLeader));
	ItemsInOrder::iterator endpoint2(m_items.project<ItemsInOrderTag>(id_it));
	
	if (endpoint1 == endpoint2) {
		// One-element sequence, already selected.
		return;
	}
	
	// The problem is that we don't know which endpoint precedes the other.
	// Let's find out.
	ItemsInOrder::iterator ord_it1(endpoint1);
	ItemsInOrder::iterator ord_it2(endpoint1);
	for (;;) {
		if (ord_it1 != m_itemsInOrder.begin()) {
			--ord_it1;
			if (ord_it1 == endpoint2) {
				// endpoint2 was found before endpoint1.
				std::swap(endpoint1, endpoint2);
				break;
			}
			
		}
		if (ord_it2 != m_itemsInOrder.end()) {
			++ord_it2;
			if (ord_it2 != m_itemsInOrder.end()) {
				if (ord_it2 == endpoint2) {
					// endpoint2 was found after endpoint1.
					break;
				}
			}
		}
	}
	
	++endpoint2; // Make the interval inclusive.
	for (; endpoint1 != endpoint2; ++endpoint1) {
		endpoint1->setSelected(true);
		moveToSelected(&*endpoint1);
	}
	
	// Switch the selection leader.
	assert(m_pSelectionLeader);
	m_pSelectionLeader->setSelectionLeader(false);
	m_pSelectionLeader = &*id_it;
	m_pSelectionLeader->setSelectionLeader(true);
	
	m_rOwner.emitNewSelectionLeader(id_it->pageInfo, id_it->composite, flags);
}

void
ThumbnailSequence::Impl::selectItemNoModifiers(ItemsById::iterator const& id_it)
{
	SelectionFlags flags = SELECTED_BY_USER;
	if (m_pSelectionLeader == &*id_it) {
		flags |= REDUNDANT_SELECTION;
	}
	
	clearSelection();
	
	m_pSelectionLeader = &*id_it;
	m_pSelectionLeader->setSelectionLeader(true);
	moveToSelected(m_pSelectionLeader);
	
	m_rOwner.emitNewSelectionLeader(id_it->pageInfo, id_it->composite, flags);
}

void
ThumbnailSequence::Impl::clear()
{
	m_pSelectionLeader = 0;
	
	ItemsInOrder::iterator it(m_itemsInOrder.begin());
	ItemsInOrder::iterator const end(m_itemsInOrder.end());
	while (it != end) {
		delete it->composite;
		m_itemsInOrder.erase(it++);
	}
	
	assert(m_graphicsScene.items().empty());
	
	m_sceneRect = QRectF(0.0, 0.0, 0.0, 0.0);
	commitSceneRect();
}

void
ThumbnailSequence::Impl::clearSelection()
{
	m_pSelectionLeader = 0;
	
	BOOST_FOREACH(Item const& item, m_selectedThenUnselected) {
		if (!item.isSelected()) {
			break;
		}
		item.setSelected(false);
	}
}

ThumbnailSequence::Impl::ItemsInOrder::iterator
ThumbnailSequence::Impl::itemInsertPosition(
	ItemsInOrder::iterator const begin, ItemsInOrder::iterator const end,
	PageId const& page_id, bool const page_incomplete,
	ItemsInOrder::iterator const hint, int* dist_from_hint)
{
	// Note that to preserve stable ordering, this function *must* return hint,
	// as long as it's an acceptable position.

	if (!m_ptrOrderProvider.get()) {
		if (dist_from_hint) {
			*dist_from_hint = 0;
		}
		return hint;
	}

	ItemsInOrder::iterator ins_pos(hint);
	int dist = 0;

	// While the element immediately preceding ins_pos is supposed to
	// follow the page we are inserting, move ins_pos one element back.
	while (ins_pos != begin) {
		ItemsInOrder::iterator prev(ins_pos);
		--prev;
		bool const precedes = m_ptrOrderProvider->precedes(
			page_id, page_incomplete, prev->pageId(), prev->incompleteThumbnail
		);
		if (precedes) {
			ins_pos = prev;
			--dist;
		} else {
			break;
		}
	}

	// While the element pointed to by ins_pos is supposed to precede
	// the page we are inserting, advance ins_pos.
	while (ins_pos != end) {
		bool const precedes = m_ptrOrderProvider->precedes(
			ins_pos->pageId(), ins_pos->incompleteThumbnail,
			page_id, page_incomplete
		);
		if (precedes) {
			++ins_pos;
			++dist;
		} else {
			break;
		}
	}

	if (dist_from_hint) {
		*dist_from_hint = dist;
	}

	return ins_pos;
}

std::auto_ptr<QGraphicsItem>
ThumbnailSequence::Impl::getThumbnail(PageInfo const& page_info)
{
	std::auto_ptr<QGraphicsItem> thumb;
	
	if (m_ptrFactory.get()) {
		thumb = m_ptrFactory->get(page_info);
	}
	
	if (!thumb.get()) {
		thumb.reset(new PlaceholderThumb(m_maxLogicalThumbSize));
	}
	
	return thumb;
}

std::auto_ptr<ThumbnailSequence::LabelGroup>
ThumbnailSequence::Impl::getLabelGroup(PageInfo const& page_info)
{
	PageId const& page_id = page_info.id();
	QFileInfo const file_info(page_id.imageId().filePath());
	QString const file_name(file_info.fileName());
	
	QString text(file_name);
	if (page_info.imageId().isMultiPageFile()) {
		text = ThumbnailSequence::tr(
			"%1 (page %2)"
		).arg(file_name).arg(page_id.imageId().page());
	}
	
	std::auto_ptr<QGraphicsSimpleTextItem> normal_text_item(new QGraphicsSimpleTextItem);
	normal_text_item->setText(text);
	
	std::auto_ptr<QGraphicsSimpleTextItem> bold_text_item(new QGraphicsSimpleTextItem);
	bold_text_item->setText(text);
	QFont bold_font(bold_text_item->font());
	bold_font.setWeight(QFont::Bold);
	bold_text_item->setFont(bold_font);
	bold_text_item->setBrush(QApplication::palette().highlightedText());
	
	QRectF normal_text_box(normal_text_item->boundingRect());
	QRectF bold_text_box(bold_text_item->boundingRect());
	normal_text_box.moveCenter(bold_text_box.center());
	normal_text_box.moveRight(bold_text_box.right());
	normal_text_item->setPos(normal_text_box.topLeft());
	bold_text_item->setPos(bold_text_box.topLeft());
	
	char const* pixmap_resource = 0;
	switch (page_id.subPage()) {
		case PageId::LEFT_PAGE:
			pixmap_resource = ":/icons/left_page_thumb.png";
			break;
		case PageId::RIGHT_PAGE:
			pixmap_resource = ":/icons/right_page_thumb.png";
			break;
		default:
			return std::auto_ptr<LabelGroup>(new LabelGroup(normal_text_item, bold_text_item));
	}
	
	QPixmap const pixmap(pixmap_resource);
	std::auto_ptr<QGraphicsPixmapItem> pixmap_item(new QGraphicsPixmapItem);
	pixmap_item->setPixmap(pixmap);
	
	int const label_pixmap_spacing = 5;
	
	QRectF pixmap_box(pixmap_item->boundingRect());
	pixmap_box.moveCenter(bold_text_box.center());
	pixmap_box.moveLeft(bold_text_box.right() + label_pixmap_spacing);
	pixmap_item->setPos(pixmap_box.topLeft());
	
	return std::auto_ptr<LabelGroup>(new LabelGroup(normal_text_item, bold_text_item, pixmap_item));
}

std::auto_ptr<ThumbnailSequence::CompositeItem>
ThumbnailSequence::Impl::getCompositeItem(
	Item const* item, PageInfo const& page_info)
{
	std::auto_ptr<QGraphicsItem> thumb(getThumbnail(page_info));
	std::auto_ptr<LabelGroup> label_group(getLabelGroup(page_info));
	std::auto_ptr<CompositeItem> composite(
		new CompositeItem(*this, thumb, label_group)
	);
	composite->setItem(item);
	return composite;
}

void
ThumbnailSequence::Impl::commitSceneRect()
{
	if (m_sceneRect.isNull()) {
		m_graphicsScene.setSceneRect(QRectF(0.0, 0.0, 1.0, 1.0));
	} else {
		m_graphicsScene.setSceneRect(m_sceneRect);
	}
}


/*==================== ThumbnailSequence::Item ======================*/

ThumbnailSequence::Item::Item(PageInfo const& page_info, CompositeItem* comp_item)
:	pageInfo(page_info),
	composite(comp_item),
	incompleteThumbnail(comp_item->incompleteThumbnail()),
	m_isSelected(false),
	m_isSelectionLeader(false)
{
}

void
ThumbnailSequence::Item::setSelected(bool selected) const
{
	bool const was_selected = m_isSelected;
	bool const was_selection_leader = m_isSelectionLeader;
	m_isSelected = selected;
	m_isSelectionLeader = m_isSelectionLeader && selected;
	
	if (was_selected != m_isSelected || was_selection_leader != m_isSelectionLeader) {
		composite->updateAppearence(m_isSelected, m_isSelectionLeader);
	}
	if (was_selected != m_isSelected) {
		composite->update();
	}
}

void
ThumbnailSequence::Item::setSelectionLeader(bool selection_leader) const
{
	bool const was_selected = m_isSelected;
	bool const was_selection_leader = m_isSelectionLeader;
	m_isSelected = m_isSelected || selection_leader;
	m_isSelectionLeader = selection_leader;
	
	if (was_selected != m_isSelected || was_selection_leader != m_isSelectionLeader) {
		composite->updateAppearence(m_isSelected, m_isSelectionLeader);
	}
	if (was_selected != m_isSelected) {
		composite->update();
	}
}


/*================== ThumbnailSequence::PlaceholderThumb ====================*/

QPainterPath ThumbnailSequence::PlaceholderThumb::m_sCachedPath;

ThumbnailSequence::PlaceholderThumb::PlaceholderThumb(QSizeF const& max_size)
:	m_maxSize(max_size)
{
}

QRectF
ThumbnailSequence::PlaceholderThumb::boundingRect() const
{
	return QRectF(QPointF(0.0, 0.0), m_maxSize);
}

void
ThumbnailSequence::PlaceholderThumb::paint(
	QPainter* painter, QStyleOptionGraphicsItem const*, QWidget*)
{
	IncompleteThumbnail::drawQuestionMark(*painter, boundingRect());
}


/*====================== ThumbnailSequence::LabelGroup ======================*/

ThumbnailSequence::LabelGroup::LabelGroup(
	std::auto_ptr<QGraphicsSimpleTextItem> normal_label,
	std::auto_ptr<QGraphicsSimpleTextItem> bold_label,
	std::auto_ptr<QGraphicsPixmapItem> pixmap)
:	m_pNormalLabel(normal_label.get()),
	m_pBoldLabel(bold_label.get())
{
	m_pNormalLabel->setVisible(true);
	m_pBoldLabel->setVisible(false);
	
	addToGroup(normal_label.release());
	addToGroup(bold_label.release());
	if (pixmap.get()) {
		addToGroup(pixmap.release());
	}
}

void
ThumbnailSequence::LabelGroup::updateAppearence(bool selected, bool selection_leader)
{
	m_pNormalLabel->setVisible(!selection_leader);
	m_pBoldLabel->setVisible(selection_leader);
	
	if (selection_leader) {
		assert(selected);
	} else if (selected) {
		m_pNormalLabel->setBrush(QApplication::palette().highlightedText());
	} else {
		m_pNormalLabel->setBrush(QApplication::palette().text());
	}
}


/*==================== ThumbnailSequence::CompositeItem =====================*/

ThumbnailSequence::CompositeItem::CompositeItem(
	ThumbnailSequence::Impl& owner,
	std::auto_ptr<QGraphicsItem> thumbnail,
	std::auto_ptr<LabelGroup> label_group)
:	m_rOwner(owner),
	m_pItem(0),
	m_pThumb(thumbnail.get()),
	m_pLabelGroup(label_group.get())
{
	QSizeF const thumb_size(thumbnail->boundingRect().size());
	QSizeF const label_size(label_group->boundingRect().size());
	
	int const thumb_label_spacing = 1;
	thumbnail->setPos(-0.5 * thumb_size.width(), 0.0);
	label_group->setPos(
		thumbnail->pos().x() + thumb_size.width() - label_size.width(),
		thumb_size.height() + thumb_label_spacing
	);
	
	addToGroup(thumbnail.release());
	addToGroup(label_group.release());
	
	setCursor(Qt::PointingHandCursor);
	setZValue(-1);
}

bool
ThumbnailSequence::CompositeItem::incompleteThumbnail() const
{
	return dynamic_cast<IncompleteThumbnail*>(m_pThumb) != 0;
}

void
ThumbnailSequence::CompositeItem::updateSceneRect(QRectF& scene_rect)
{
	QRectF rect(m_pThumb->boundingRect());
	rect.translate(m_pThumb->pos());
	rect.translate(pos());
	
	QRectF bounding_rect(boundingRect());
	bounding_rect.translate(pos());
	
	rect.setTop(bounding_rect.top());
	rect.setBottom(bounding_rect.bottom());
	
	scene_rect |= rect;
}

void
ThumbnailSequence::CompositeItem::updateAppearence(bool selected, bool selection_leader)
{
	m_pLabelGroup->updateAppearence(selected, selection_leader);
}

QRectF
ThumbnailSequence::CompositeItem::boundingRect() const
{
	QRectF rect(QGraphicsItemGroup::boundingRect());
	rect.adjust(-100, -5, 100, 3);
	return rect;
}

void
ThumbnailSequence::CompositeItem::paint(
	QPainter* painter, QStyleOptionGraphicsItem const* option, QWidget *widget)
{
	if (m_pItem->isSelected()) {
		painter->fillRect(
			boundingRect(),
			QApplication::palette().color(QPalette::Highlight)
		);
	}
}

void
ThumbnailSequence::CompositeItem::mousePressEvent(
	QGraphicsSceneMouseEvent* const event)
{
	QGraphicsItemGroup::mousePressEvent(event);
	
	event->accept();
	
	if (event->button() == Qt::LeftButton) {
		m_rOwner.itemSelectedByUser(this, event->modifiers());
	}
}

void
ThumbnailSequence::CompositeItem::contextMenuEvent(
	QGraphicsSceneContextMenuEvent* const event)
{
	event->accept(); // Prevent it from propagating further.
	m_rOwner.contextMenuRequested(
		m_pItem->pageInfo, event->screenPos(), m_pItem->isSelected()
	);
}
