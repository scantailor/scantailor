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

#include "ThumbnailSequence.h.moc"
#include "ThumbnailFactory.h"
#include "IncompleteThumbnail.h"
#include "PageSequence.h"
#include "PageInfo.h"
#include "PageId.h"
#include "ImageId.h"
#include "RefCountable.h"
#include "IntrusivePtr.h"
#include "ScopedIncDec.h"
#include <boost/multi_index_container.hpp>
#include <boost/multi_index/ordered_index.hpp>
#include <boost/multi_index/sequenced_index.hpp>
#include <boost/multi_index/mem_fun.hpp>
#include <boost/foreach.hpp>
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

using namespace ::boost;
using namespace ::boost::multi_index;


class ThumbnailSequence::Item
{
public:
	Item(PageInfo const& page_info, int page_num, CompositeItem* comp_item)
	: pageInfo(page_info), pageNum(page_num), composite(comp_item),
	m_isSelected(false), m_isSelectionLeader(false) {}
	
	PageId const& pageId() const { return pageInfo.id(); }
	
	bool isSelected() const { return m_isSelected; }
	
	bool isSelectionLeader() const { return m_isSelectionLeader; }
	
	void setSelected(bool selected) const;
	
	void setSelectionLeader(bool selection_leader) const;
	
	PageInfo pageInfo;
	int pageNum;
	mutable CompositeItem* composite;
private:
	mutable bool m_isSelected;
	mutable bool m_isSelectionLeader;
};


class ThumbnailSequence::Impl
{
public:
	Impl(ThumbnailSequence& owner, QSizeF const& max_logical_thumb_size);
	
	~Impl();
	
	void setThumbnailFactory(IntrusivePtr<ThumbnailFactory> const& factory);
	
	void attachView(QGraphicsView* view);
	
	void reset(PageSequenceSnapshot const& pages,
		SelectionAction const selection_action);
	
	void invalidateThumbnail(PageId const& page_id);
	
	void invalidateAllThumbnails();
	
	void setSelection(PageId const& page_id);
	
	void insert(PageInfo const& new_page,
		BeforeOrAfter before_or_after, PageId const& existing);
	
	void remove(ImageId const& image_id);
	
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
	
	void remove(ItemsById::iterator const& id_it);
	
	void selectItemNoModifiers(ItemsById::iterator const& it);
	
	void selectItemWithControl(ItemsById::iterator const& it);
	
	void selectItemWithShift(ItemsById::iterator const& it);
	
	bool multipleItemsSelected() const;
	
	void moveToSelected(Item const* item);
	
	void moveToUnselected(Item const* item);
	
	void setThumbnail(
		ItemsById::iterator const& id_it,
		std::auto_ptr<CompositeItem> composite);
	
	void clear();
	
	void clearSelection();
	
	std::auto_ptr<QGraphicsItem> getThumbnail(
		PageInfo const& page_info, int page_num);
	
	std::auto_ptr<LabelGroup> getLabelGroup(PageInfo const& page_info);
	
	std::auto_ptr<CompositeItem> getCompositeItem(
		Item const* item, PageInfo const& info, int page_num);
	
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
	QGraphicsScene m_graphicsScene;
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
ThumbnailSequence::reset(PageSequenceSnapshot const& pages,
	SelectionAction const selection_action)
{
	m_ptrImpl->reset(pages, selection_action);
}

void
ThumbnailSequence::invalidateThumbnail(PageId const& page_id)
{
	m_ptrImpl->invalidateThumbnail(page_id);
}

void
ThumbnailSequence::invalidateAllThumbnails()
{
	m_ptrImpl->invalidateAllThumbnails();
}

void
ThumbnailSequence::setSelection(PageId const& page_id)
{
	m_ptrImpl->setSelection(page_id);
}

void
ThumbnailSequence::insert(
	PageInfo const& new_page,
	BeforeOrAfter before_or_after, PageId const& existing)
{
	m_ptrImpl->insert(new_page, before_or_after, existing);
}

void
ThumbnailSequence::remove(ImageId const& image_id)
{
	m_ptrImpl->remove(image_id);
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

void
ThumbnailSequence::emitContextMenuRequested(
	PageInfo const& page_info, QPoint const& screen_pos, bool selected)
{
	emit contextMenuRequested(page_info, screen_pos, selected);
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
ThumbnailSequence::Impl::reset(PageSequenceSnapshot const& pages,
	SelectionAction const selection_action)
{
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
	double offset = 0;
	
	for (size_t i = 0; i < num_pages; ++i) {
		PageInfo const& page_info(pages.pageAt(i));
		
		std::auto_ptr<CompositeItem> composite(
			getCompositeItem(0, page_info, i)
		);
		composite->setPos(0.0, offset);
		composite->updateSceneRect(m_sceneRect);
		
		offset += composite->boundingRect().height() + SPACING;
		
		m_itemsInOrder.push_back(Item(page_info, i, composite.get()));
		Item const* item = &m_itemsInOrder.back();
		composite->setItem(item);
		
		if (selected.find(page_info.id()) != selected.end()) {
			item->setSelected(true);
			moveToSelected(item);
			some_selected_item = item;
		}
		if (page_info.id() == selection_leader.id()) {
			m_pSelectionLeader = item;
		}
		
		m_graphicsScene.addItem(composite.release());
	}
	
	commitSceneRect();
	
	if (!m_pSelectionLeader) {
		if (some_selected_item) {
			m_pSelectionLeader = some_selected_item;
		} else {
			ItemsById::iterator id_it(m_itemsById.find(pages.curPage().id()));
			if (id_it != m_itemsById.end()) {
				m_pSelectionLeader = &*id_it;
				moveToSelected(m_pSelectionLeader);
			}
		}
	}
	
	if (m_pSelectionLeader) {
		m_pSelectionLeader->setSelectionLeader(true);
		m_rOwner.emitNewSelectionLeader(
			selection_leader, m_pSelectionLeader->composite, DEFAULT_SELECTION_FLAGS
		);
	}
}

void
ThumbnailSequence::Impl::invalidateThumbnail(PageId const& page_id)
{
	ItemsById::iterator const id_it(m_itemsById.find(page_id));
	if (id_it != m_itemsById.end()) {
		setThumbnail(
			id_it,
			getCompositeItem(&*id_it, id_it->pageInfo, id_it->pageNum)
		);
		id_it->composite->updateAppearence(id_it->isSelected(), id_it->isSelectionLeader());
	}
}

void
ThumbnailSequence::Impl::invalidateAllThumbnails()
{
	m_sceneRect = QRectF(0.0, 0.0, 0.0, 0.0);
	double offset = 0;
	
	ItemsInOrder::iterator ord_it(m_itemsInOrder.begin());
	ItemsInOrder::iterator const ord_end(m_itemsInOrder.end());
	for (; ord_it != ord_end; ++ord_it) {
		std::auto_ptr<CompositeItem> composite(
			getCompositeItem(&*ord_it, ord_it->pageInfo, ord_it->pageNum)
		);
		
		CompositeItem* const old_composite = ord_it->composite;
		CompositeItem* const new_composite = composite.get();
		
		new_composite->setPos(0.0, offset);
		new_composite->updateSceneRect(m_sceneRect);
		new_composite->updateAppearence(ord_it->isSelected(), ord_it->isSelectionLeader());
		
		offset += new_composite->boundingRect().height() + SPACING;
		delete old_composite;
		
		ord_it->composite = new_composite;
		m_graphicsScene.addItem(composite.release());
	}
	
	commitSceneRect();
}

void
ThumbnailSequence::Impl::setSelection(PageId const& page_id)
{
	ItemsById::iterator const id_it(m_itemsById.find(page_id));
	if (id_it == m_itemsById.end()) {
		return;
	}
	
	bool const was_selection_leader = (&*id_it == m_pSelectionLeader);
	
	// Clear selection from all items except the one for which
	// selection is requested.
	BOOST_FOREACH(Item const& item, m_selectedThenUnselected) {
		if (!item.isSelected()) {
			break;
		}
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
}

void
ThumbnailSequence::Impl::insert(
	PageInfo const& page_info,
	BeforeOrAfter before_or_after, PageId const& existing)
{
	ItemsById::iterator id_it(m_itemsById.find(existing));
	if (id_it == m_itemsById.end()) {
		return;
	}
	
	ItemsInOrder::iterator ord_it(m_items.project<ItemsInOrderTag>(id_it));
	if (before_or_after == AFTER) {
		++ord_it;
	}
	
	int page_num = 0;
	double offset = 0.0;
	if (!m_items.empty()) {
		// That's the best thing we can do here.
		// A proper solution would require renaming files.
		page_num = m_itemsInOrder.rbegin()->pageNum + 1;
		
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
		getCompositeItem(0, page_info, page_num)
	);
	composite->setPos(0.0, offset);
	composite->updateSceneRect(m_sceneRect);
	
	QPointF const pos_delta(0.0, composite->boundingRect().height() + SPACING);
	
	Item const item(page_info, page_num, composite.get());
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
ThumbnailSequence::Impl::remove(ImageId const& image_id)
{
	ItemsById::iterator id_it(
		m_itemsById.find(PageId(image_id, PageId::SINGLE_PAGE))
	);
	if (id_it != m_itemsById.end()) {
		remove(id_it);
	}
	id_it = m_itemsById.find(PageId(image_id, PageId::LEFT_PAGE));
	if (id_it != m_itemsById.end()) {
		remove(id_it);
	}
	id_it = m_itemsById.find(PageId(image_id, PageId::RIGHT_PAGE));
	if (id_it != m_itemsById.end()) {
		remove(id_it);
	}
}

void
ThumbnailSequence::Impl::remove(ItemsById::iterator const& id_it)
{
	QPointF const pos_delta(
		0.0, -(id_it->composite->boundingRect().height() + SPACING)
	);
	
	if (m_pSelectionLeader == &*id_it) {
		m_pSelectionLeader = 0;
	}
	delete id_it->composite;
	
	if (pos_delta != QPointF()) {
		ItemsInOrder::iterator ord_it(m_items.project<ItemsInOrderTag>(id_it));
		ItemsInOrder::iterator const ord_end(m_itemsInOrder.end());
		++ord_it; // Skip the item itself.
		for (; ord_it != ord_end; ++ord_it) {
			ord_it->composite->setPos(ord_it->composite->pos() + pos_delta);
		}
	}
	
	m_itemsById.erase(id_it);
	
	m_sceneRect.adjust(0.0, 0.0, pos_delta.x(), pos_delta.y());
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
	int page_idx = 0;
	for (;;) {
		for (; it != end && !it->isSelected(); ++it, ++page_idx) {
			// Skip unselected items.
		}
		if (it == end) {
			break;
		}
		
		ranges.push_back(PageRange());
		PageRange& range = ranges.back();
		range.firstPageIdx = page_idx;
		for (; it != end && it->isSelected(); ++it, ++page_idx) {
			range.pages.push_back(it->pageInfo.id());
		}
	}
	
	return ranges;
}

void
ThumbnailSequence::Impl::contextMenuRequested(
	PageInfo const& page_info, QPoint const& screen_pos, bool selected)
{
	m_rOwner.emitContextMenuRequested(page_info, screen_pos, selected);
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
ThumbnailSequence::Impl::setThumbnail(
	ItemsById::iterator const& id_it, std::auto_ptr<CompositeItem> composite)
{
	CompositeItem* const old_composite = id_it->composite;
	CompositeItem* const new_composite = composite.get();
	QSizeF const old_size(old_composite->boundingRect().size());
	QSizeF const new_size(new_composite->boundingRect().size());
	
	new_composite->setPos(old_composite->pos());
	composite->updateSceneRect(m_sceneRect);
	
	QPointF const pos_delta(0.0, new_size.height() - old_size.height());
	delete old_composite;
	
	if (pos_delta != QPointF(0.0, 0.0)) {
		ItemsInOrder::iterator ord_it(m_items.project<ItemsInOrderTag>(id_it));
		ItemsInOrder::iterator const ord_end(m_itemsInOrder.end());
		++ord_it; // Skip the item itself.
		for (; ord_it != ord_end; ++ord_it) {
			ord_it->composite->setPos(ord_it->composite->pos() + pos_delta);
			ord_it->composite->updateSceneRect(m_sceneRect);
		}
	}
	
	id_it->composite = new_composite;
	m_graphicsScene.addItem(composite.release());
	
	commitSceneRect();
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

std::auto_ptr<QGraphicsItem>
ThumbnailSequence::Impl::getThumbnail(
	PageInfo const& page_info, int const page_num)
{
	std::auto_ptr<QGraphicsItem> thumb;
	
	if (m_ptrFactory.get()) {
		thumb = m_ptrFactory->get(page_info, page_num);
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
	int const page_num = page_id.imageId().page();
	
	QString text(file_name);
	if (page_info.isMultiPageFile() || page_num > 0) {
		text = ThumbnailSequence::tr(
			"%1 (page %2)"
		).arg(file_name).arg(page_num + 1);
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
	Item const* item, PageInfo const& page_info, int const page_num)
{
	std::auto_ptr<QGraphicsItem> thumb(getThumbnail(page_info, page_num));
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
