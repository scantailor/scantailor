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

#include "ThumbnailSequence.h.moc"
#include "ThumbnailFactory.h"
#include "IncompleteThumbnail.h"
#include "PageSequence.h"
#include "PageInfo.h"
#include "PageId.h"
#include "ImageId.h"
#include "ScopedIncDec.h"
#include <boost/multi_index_container.hpp>
#include <boost/multi_index/ordered_index.hpp>
#include <boost/multi_index/sequenced_index.hpp>
#include <boost/multi_index/mem_fun.hpp>
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
#include <stddef.h>
#include <assert.h>

using namespace ::boost;
using namespace ::boost::multi_index;


class ThumbnailSequence::Item
{
public:
	Item(PageInfo const& page_info, int page_num, CompositeItem* comp_item)
	: pageInfo(page_info), pageNum(page_num), composite(comp_item) {}
	
	PageId const& pageId() const { return pageInfo.id(); }
	
	PageInfo pageInfo;
	int pageNum;
	mutable CompositeItem* composite;
};


class ThumbnailSequence::Impl
{
public:
	Impl(ThumbnailSequence& owner, QSizeF const& max_logical_thumb_size);
	
	~Impl();
	
	void setThumbnailFactory(IntrusivePtr<ThumbnailFactory> const& factory);
	
	void attachView(QGraphicsView* view);
	
	void reset(PageSequenceSnapshot const& pages);
	
	void invalidateThumbnail(PageId const& page_id);
	
	void invalidateAllThumbnails();
	
	void setCurrentThumbnail(PageId const& page_id);
	
	void insert(PageInfo const& new_page,
		BeforeOrAfter before_or_after, PageId const& existing);
	
	void remove(ImageId const& image_id);
	
	QRectF currentItemSceneRect() const;
	
	void itemSelected(PageInfo const& page_info, CompositeItem* item);
	
	void contextMenuRequested(
		PageInfo const& page_info, QPoint const& screen_pos, bool selected);
private:
	class ItemsByIdTag;
	class ItemsInOrderTag;
	
	typedef multi_index_container<
		Item,
		indexed_by<
			ordered_unique<
				tag<ItemsByIdTag>,
				const_mem_fun<Item, PageId const&, &Item::pageId>
			>,
			sequenced<tag<ItemsInOrderTag> >
		>
	> Container;
	
	typedef Container::index<ItemsByIdTag>::type ItemsById;
	typedef Container::index<ItemsInOrderTag>::type ItemsInOrder;
	
	void remove(ItemsById::iterator const& id_it);
	
	void setThumbnail(
		ItemsById::iterator const& id_it,
		std::auto_ptr<CompositeItem> composite);
	
	void clear();
	
	std::auto_ptr<QGraphicsItem> getThumbnail(
		PageInfo const& page_info, int page_num);
	
	std::auto_ptr<LabelGroup> getLabelGroup(PageInfo const& page_info);
	
	std::auto_ptr<CompositeItem> getCompositeItem(
		PageInfo const& page_info, int page_num);
	
	void commitSceneRect();
	
	static int const SPACING = 10;
	ThumbnailSequence& m_rOwner;
	QSizeF m_maxLogicalThumbSize;
	Container m_items;
	ItemsById& m_itemsById;
	ItemsInOrder& m_itemsInOrder;
	CompositeItem* m_pSelectedItem;
	IntrusivePtr<ThumbnailFactory> m_ptrFactory;
	QGraphicsScene m_graphicsScene;
	QRectF m_sceneRect;
	int m_syntheticSelectionScope;
};


template<typename Base>
class ThumbnailSequence::NoSelectionItem : public Base
{
public:
	virtual void paint(QPainter* painter,
		QStyleOptionGraphicsItem const* option, QWidget *widget);
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


class ThumbnailSequence::LabelGroup : public NoSelectionItem<QGraphicsItemGroup>
{
public:
	LabelGroup(std::auto_ptr<QGraphicsSimpleTextItem> label);
	
	LabelGroup(
		std::auto_ptr<QGraphicsSimpleTextItem> label,
		std::auto_ptr<QGraphicsPixmapItem> pixmap);
	
	QGraphicsSimpleTextItem* label() { return m_pLabel; }
private:
	QGraphicsSimpleTextItem* m_pLabel;
};


class ThumbnailSequence::CompositeItem : public QGraphicsItemGroup
{
public:
	CompositeItem(
		ThumbnailSequence::Impl& owner,
		PageInfo const& page_info,
		std::auto_ptr<QGraphicsItem> thumbnail,
		std::auto_ptr<LabelGroup> label_group);
	
	void updateSceneRect(QRectF& scene_rect);
	
	virtual QRectF boundingRect() const;
	
	virtual void paint(QPainter* painter,
		QStyleOptionGraphicsItem const* option, QWidget *widget);
protected:
	virtual QVariant itemChange(GraphicsItemChange change, QVariant const& value);
	
	virtual void contextMenuEvent(QGraphicsSceneContextMenuEvent* event);
	
	virtual void mousePressEvent(QGraphicsSceneMouseEvent* event);
private:
	ThumbnailSequence::Impl& m_rOwner;
	QGraphicsItem* m_pThumb;
	QGraphicsSimpleTextItem* m_pLabel;
	PageInfo m_pageInfo;
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
ThumbnailSequence::reset(PageSequenceSnapshot const& pages)
{
	m_ptrImpl->reset(pages);
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
ThumbnailSequence::setCurrentThumbnail(PageId const& page_id)
{
	m_ptrImpl->setCurrentThumbnail(page_id);
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
ThumbnailSequence::currentItemSceneRect() const
{
	return m_ptrImpl->currentItemSceneRect();
}

void
ThumbnailSequence::emitPageSelected(
	PageInfo const& page_info, CompositeItem const* composite,
	bool const by_user, bool const was_already_selected)
{
	QRectF const thumb_rect(
		composite->mapToScene(composite->boundingRect()).boundingRect()
	);
	emit pageSelected(page_info, thumb_rect, by_user, was_already_selected);
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
	m_pSelectedItem(0),
	m_syntheticSelectionScope(0)
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
ThumbnailSequence::Impl::reset(PageSequenceSnapshot const& pages)
{
	ScopedIncDec<int> const scope_manager(m_syntheticSelectionScope);
	
	clear();
	
	size_t const num_pages = pages.numPages();
	if (num_pages == 0) {
		return;
	}
	
	PageId const cur_page(pages.curPage().id());
	CompositeItem* cur_item = 0;
	double offset = 0;
	
	for (size_t i = 0; i < num_pages; ++i) {
		PageInfo const& page_info(pages.pageAt(i));
		
		std::auto_ptr<CompositeItem> composite(
			getCompositeItem(page_info, i)
		);
		composite->setPos(0.0, offset);
		composite->updateSceneRect(m_sceneRect);
		
		offset += composite->boundingRect().height() + SPACING;
		
		if (page_info.id() == cur_page) {
			cur_item = composite.get();
		}
		
		m_itemsInOrder.push_back(Item(page_info, i, composite.get()));
		m_graphicsScene.addItem(composite.release());
	}
	
	commitSceneRect();
	
	if (cur_item) {
		assert(!cur_item->isSelected()); // Because it was just created.
		cur_item->setSelected(true); // This will cause a callback.
	}
}

void
ThumbnailSequence::Impl::invalidateThumbnail(PageId const& page_id)
{
	ItemsById::iterator const id_it(m_itemsById.find(page_id));
	if (id_it != m_itemsById.end()) {
		setThumbnail(
			id_it,
			getCompositeItem(id_it->pageInfo, id_it->pageNum)
		);
	}
}

void
ThumbnailSequence::Impl::invalidateAllThumbnails()
{
	ScopedIncDec<int> const scope_manager(m_syntheticSelectionScope);
	
	m_sceneRect = QRectF(0.0, 0.0, 0.0, 0.0);
	double offset = 0;
	
	CompositeItem* to_be_selected = 0;
	
	ItemsInOrder::iterator ord_it(m_itemsInOrder.begin());
	ItemsInOrder::iterator const ord_end(m_itemsInOrder.end());
	for (; ord_it != ord_end; ++ord_it) {
		std::auto_ptr<CompositeItem> composite(
			getCompositeItem(ord_it->pageInfo, ord_it->pageNum)
		);
		
		CompositeItem* const old_composite = ord_it->composite;
		CompositeItem* const new_composite = composite.get();
		if (old_composite->isSelected()) {
			to_be_selected = new_composite;
		}
		
		new_composite->setPos(0.0, offset);
		new_composite->updateSceneRect(m_sceneRect);
		
		offset += new_composite->boundingRect().height() + SPACING;
		
		if (m_pSelectedItem == old_composite) {
			m_pSelectedItem = 0;
		}
		delete old_composite;
		
		ord_it->composite = new_composite;
		m_graphicsScene.addItem(composite.release());
	}

	commitSceneRect();
	
	if (to_be_selected) {
		assert(!to_be_selected->isSelected()); // Because it was just created.
		to_be_selected->setSelected(true); // This will cause a callback.
	}
}

void
ThumbnailSequence::Impl::setCurrentThumbnail(PageId const& page_id)
{
	ItemsById::iterator const id_it(m_itemsById.find(page_id));
	if (id_it == m_itemsById.end()) {
		return;
	}
	
	bool const was_already_selected = (id_it->composite == m_pSelectedItem);
	if (!was_already_selected) {
		assert(!id_it->composite->isSelected());
		ScopedIncDec<int> const scope_manager(m_syntheticSelectionScope);
		id_it->composite->setSelected(true); // This will cause a callback.
		return;
	}
	
	bool const by_user = false;
	m_rOwner.emitPageSelected(
		id_it->pageInfo, id_it->composite, by_user, was_already_selected
	);
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
		getCompositeItem(page_info, page_num)
	);
	composite->setPos(0.0, offset);
	composite->updateSceneRect(m_sceneRect);
	
	QPointF const pos_delta(0.0, composite->boundingRect().height() + SPACING);
	
	m_itemsInOrder.insert(ord_it, Item(page_info, page_num, composite.get()));
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
	ScopedIncDec<int> const scope_manager(m_syntheticSelectionScope);
	
	QPointF const pos_delta(
		0.0, -(id_it->composite->boundingRect().height() + SPACING)
	);
	
	if (m_pSelectedItem == id_it->composite) {
		m_pSelectedItem = 0;
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

QRectF
ThumbnailSequence::Impl::currentItemSceneRect() const
{
	if (!m_pSelectedItem) {
		return QRectF();
	}
	
	return m_pSelectedItem->mapToScene(
		m_pSelectedItem->boundingRect()
	).boundingRect();
}

void
ThumbnailSequence::Impl::itemSelected(
	PageInfo const& page_info, CompositeItem* const item)
{
	bool const was_already_selected = (m_pSelectedItem == item);
	if (!was_already_selected) {
		if (m_pSelectedItem) {
			m_pSelectedItem->setSelected(false);
		}
		m_pSelectedItem = item;
	}
	
	bool const by_user = !m_syntheticSelectionScope;
	m_rOwner.emitPageSelected(
		page_info, item, by_user, was_already_selected
	);
}

void
ThumbnailSequence::Impl::contextMenuRequested(
	PageInfo const& page_info, QPoint const& screen_pos, bool selected)
{
	m_rOwner.emitContextMenuRequested(page_info, screen_pos, selected);
}

void
ThumbnailSequence::Impl::setThumbnail(
	ItemsById::iterator const& id_it, std::auto_ptr<CompositeItem> composite)
{
	ScopedIncDec<int> const scope_manager(m_syntheticSelectionScope);
	
	CompositeItem* const old_composite = id_it->composite;
	CompositeItem* const new_composite = composite.get();
	QSizeF const old_size(old_composite->boundingRect().size());
	QSizeF const new_size(new_composite->boundingRect().size());
	bool const old_selected = old_composite->isSelected();
	
	new_composite->setPos(old_composite->pos());
	composite->updateSceneRect(m_sceneRect);
	
	QPointF const pos_delta(0.0, new_size.height() - old_size.height());
	
	if (m_pSelectedItem == old_composite) {
		m_pSelectedItem = 0;
	}
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
	
	if (old_selected) {
		assert(!new_composite->isSelected()); // Because it was just created.
		new_composite->setSelected(true); // This will cause a callback.
	}
}

void
ThumbnailSequence::Impl::clear()
{
	m_pSelectedItem = 0;
	
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
	
	std::auto_ptr<QGraphicsSimpleTextItem> text_item(
		new NoSelectionItem<QGraphicsSimpleTextItem>()
	);
	text_item->setText(text);
	QSizeF const text_item_size(text_item->boundingRect().size());
	
	char const* pixmap_resource = 0;
	switch (page_id.subPage()) {
		case PageId::SINGLE_PAGE:
			return std::auto_ptr<LabelGroup>(new LabelGroup(text_item));
		case PageId::LEFT_PAGE:
			pixmap_resource = ":/icons/left_page_thumb.png";
			break;
		case PageId::RIGHT_PAGE:
			pixmap_resource = ":/icons/right_page_thumb.png";
			break;
	}
	
	QPixmap const pixmap(pixmap_resource);
	int const label_pixmap_spacing = 5;
	std::auto_ptr<QGraphicsPixmapItem> pixmap_item(
		new NoSelectionItem<QGraphicsPixmapItem>()
	);
	pixmap_item->setPixmap(pixmap);
	if (text_item_size.height() >= pixmap.height()) {
		pixmap_item->setPos(
			text_item_size.width() + label_pixmap_spacing,
			0.5 * (text_item_size.height() - pixmap.height())
		);
	} else {
		pixmap_item->setPos(
			text_item_size.width() + label_pixmap_spacing, 0.0
		);
		text_item->setPos(
			0.0, 0.5 * (pixmap.height() - text_item_size.height())
		);
	}
	
	return std::auto_ptr<LabelGroup>(new LabelGroup(text_item, pixmap_item));
}

std::auto_ptr<ThumbnailSequence::CompositeItem>
ThumbnailSequence::Impl::getCompositeItem(
	PageInfo const& page_info, int const page_num)
{
	std::auto_ptr<QGraphicsItem> thumb(getThumbnail(page_info, page_num));
	std::auto_ptr<LabelGroup> label_group(getLabelGroup(page_info));
	return std::auto_ptr<CompositeItem>(
		new CompositeItem(*this, page_info, thumb, label_group)
	);
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


/*=================== ThumbnailSequence::NoSelectionItem ===================*/

template<typename Base>
void
ThumbnailSequence::NoSelectionItem<Base>::paint(
	QPainter* painter, QStyleOptionGraphicsItem const* option, QWidget *widget)
{
	if (option->state & QStyle::State_Selected) {
		QStyleOptionGraphicsItem new_opt(*option);
		new_opt.state &= ~QStyle::State_Selected;
		Base::paint(painter, &new_opt, widget);
	} else {
		Base::paint(painter, option, widget);
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
	std::auto_ptr<QGraphicsSimpleTextItem> label)
:	m_pLabel(label.get())
{
	addToGroup(label.release());
}

ThumbnailSequence::LabelGroup::LabelGroup(
	std::auto_ptr<QGraphicsSimpleTextItem> label,
	std::auto_ptr<QGraphicsPixmapItem> pixmap)
:	m_pLabel(label.get())
{
	addToGroup(label.release());
	addToGroup(pixmap.release());
}


/*==================== ThumbnailSequence::CompositeItem =====================*/

ThumbnailSequence::CompositeItem::CompositeItem(
	ThumbnailSequence::Impl& owner, PageInfo const& page_info,
	std::auto_ptr<QGraphicsItem> thumbnail,
	std::auto_ptr<LabelGroup> label_group)
:	m_rOwner(owner),
	m_pThumb(thumbnail.get()),
	m_pLabel(label_group->label()),
	m_pageInfo(page_info)
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
	
	setFlag(QGraphicsItem::ItemIsSelectable);
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
	if (option->state & QStyle::State_Selected) {
		painter->fillRect(boundingRect(), QApplication::palette().highlight());
	}
}

QVariant
ThumbnailSequence::CompositeItem::itemChange(
	GraphicsItemChange const change, QVariant const& value)
{
	if (change == QGraphicsItem::ItemSelectedChange) {
		QPalette const palette(QApplication::palette());
		if (value.toBool()) {
			m_pLabel->setBrush(palette.highlightedText());
			m_rOwner.itemSelected(m_pageInfo, this);
		} else {
			m_pLabel->setBrush(palette.text());
		}
	}
	return value;
}

void
ThumbnailSequence::CompositeItem::mousePressEvent(
	QGraphicsSceneMouseEvent* const event)
{
	// We won't receive the itemChange() even if we are already selected.
	bool const force_selection = (event->button() == Qt::LeftButton && isSelected());
	
	QGraphicsItemGroup::mousePressEvent(event);
	
	// For some reason, if we don't do this, right click will result
	// in the current item being de-selected.
	event->accept();
	
	if (force_selection) {
		m_rOwner.itemSelected(m_pageInfo, this);
	}
}

void
ThumbnailSequence::CompositeItem::contextMenuEvent(
	QGraphicsSceneContextMenuEvent* const event)
{
	event->accept(); // Prevent it from propagating further.
	m_rOwner.contextMenuRequested(m_pageInfo, event->screenPos(), isSelected());
}
