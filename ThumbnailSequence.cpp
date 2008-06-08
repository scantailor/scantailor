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

#include "ThumbnailSequence.h"
#include "ThumbnailFactory.h"
#include "PageSequence.h"
#include "PageInfo.h"
#include "PageId.h"
#include "LogicalPageId.h"
#include <boost/multi_index_container.hpp>
#include <boost/multi_index/ordered_index.hpp>
#include <boost/multi_index/sequenced_index.hpp>
#include <boost/multi_index/mem_fun.hpp>
#include <QGraphicsScene>
#include <QGraphicsItem>
#include <QGraphicsView>
#include <QRectF>
#include <QSizeF>
#include <QPointF>
#include <QPainter>
#include <QTransform>
#include <QBrush>
#include <QColor>
#include <QDebug>
#include <stddef.h>
#include <assert.h>

using namespace ::boost;
using namespace ::boost::multi_index;


class ThumbnailSequence::Item
{
public:
	Item(PageInfo const& page_info, QGraphicsItem* thumb, bool tag = false)
	: pageInfo(page_info), thumbnail(thumb), tagged(tag) {}
	
	LogicalPageId const& pageId() const { return pageInfo.id(); }
	
	PageInfo pageInfo;
	mutable QGraphicsItem* thumbnail;
	mutable bool tagged;
};


class ThumbnailSequence::Impl
{
public:
	Impl();
	
	~Impl();
	
	void setThumbnailFactory(IntrusivePtr<ThumbnailFactory> const& factory);
	
	void attachView(QGraphicsView* view);
	
	void syncWith(PageSequenceSnapshot const& pages);
	
	void reset(PageSequenceSnapshot const& pages);
	
	void invalidateThumbnail(PageId const& page_id);
private:
	class ItemsByIdTag;
	class ItemsInOrderTag;
	
	typedef multi_index_container<
		Item,
		indexed_by<
			ordered_unique<
				tag<ItemsByIdTag>,
				const_mem_fun<Item, LogicalPageId const&, &Item::pageId>
			>,
			sequenced<tag<ItemsInOrderTag> >
		>
	> Container;
	
	typedef Container::index<ItemsByIdTag>::type ItemsById;
	typedef Container::index<ItemsInOrderTag>::type ItemsInOrder;
	
	void setThumbnail(
		ItemsById::iterator const& id_it,
		std::auto_ptr<QGraphicsItem> thumb_ptr);
	
	void clear();
	
	void commitSceneRect();
	
	static int const SPACING = 20;
	Container m_items;
	ItemsById& m_itemsById;
	ItemsInOrder& m_itemsInOrder;
	IntrusivePtr<ThumbnailFactory> m_ptrFactory;
	QGraphicsScene m_graphicsScene;
	QRectF m_sceneRect;
};


class ThumbnailSequence::PlaceholderThumb : public QGraphicsItem
{
public:
	PlaceholderThumb();
	
	virtual QRectF boundingRect() const;
	
	virtual void paint(QPainter* painter,
		QStyleOptionGraphicsItem const* option, QWidget *widget);
};


/*============================= ThumbnailSequence ===========================*/

ThumbnailSequence::ThumbnailSequence()
:	m_ptrImpl(new Impl)
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
ThumbnailSequence::syncWith(PageSequenceSnapshot const& pages)
{
	m_ptrImpl->syncWith(pages);
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


/*======================== ThumbnailSequence::Impl ==========================*/

ThumbnailSequence::Impl::Impl()
:	m_items(),
	m_itemsById(m_items.get<ItemsByIdTag>()),
	m_itemsInOrder(m_items.get<ItemsInOrderTag>())
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
ThumbnailSequence::Impl::syncWith(PageSequenceSnapshot const& pages)
{
	double offset = 0;
	size_t const num_pages = pages.numPages();
	for (size_t i = 0; i < num_pages; ++i) {
		PageInfo const& page_info(pages.pageAt(i));
		ItemsById::iterator const id_it(m_itemsById.find(page_info.id()));
		
		QGraphicsItem* thumb = 0;
		ItemsInOrder::iterator insert_pos(m_itemsInOrder.begin());
		
		if (id_it == m_itemsById.end()) {
			std::auto_ptr<QGraphicsItem> thumb_ptr;
			if (m_ptrFactory.get()) {
				thumb_ptr = m_ptrFactory->get(page_info);
			}
			if (!thumb_ptr.get()) {
				thumb_ptr.reset(new PlaceholderThumb);
			}
			thumb_ptr->setPos(-0.5 * thumb_ptr->boundingRect().width(), offset);
			thumb = thumb_ptr.release();
			m_itemsInOrder.insert(insert_pos, Item(page_info, thumb, true));
			m_graphicsScene.addItem(thumb);
		} else {
			thumb = id_it->thumbnail;
			id_it->tagged = true;
			ItemsInOrder::iterator const ord_it(m_items.project<ItemsInOrderTag>(id_it));
			m_itemsInOrder.relocate(insert_pos, ord_it);
		}
		
		m_sceneRect |= thumb->boundingRect().translated(thumb->pos());
		offset += thumb->boundingRect().height() + SPACING;
	}
	
	ItemsInOrder::iterator ord_it(m_itemsInOrder.begin());
	ItemsInOrder::iterator const ord_end(m_itemsInOrder.end());
	while (ord_it != ord_end) {
		if (ord_it->tagged) {
			ord_it->tagged = false;
			++ord_it;
		} else {
			m_graphicsScene.removeItem(ord_it->thumbnail);
			delete ord_it->thumbnail;
			m_itemsInOrder.erase(ord_it++);
		}
	}
	
	commitSceneRect();
}

void
ThumbnailSequence::Impl::reset(PageSequenceSnapshot const& pages)
{
	clear();
	
	double offset = 0;
	size_t const num_pages = pages.numPages();
	for (size_t i = 0; i < num_pages; ++i) {
		PageInfo const& page_info(pages.pageAt(i));
		std::auto_ptr<QGraphicsItem> thumb_ptr;
		if (m_ptrFactory.get()) {
			thumb_ptr = m_ptrFactory->get(page_info);
		}
		if (!thumb_ptr.get()) {
			thumb_ptr.reset(new PlaceholderThumb);
		}
		thumb_ptr->setPos(-0.5 * thumb_ptr->boundingRect().width(), offset);
		QGraphicsItem* thumb = thumb_ptr.release();
		
		m_itemsInOrder.push_back(Item(page_info, thumb));
		m_graphicsScene.addItem(thumb);
		
		m_sceneRect |= thumb->boundingRect().translated(thumb->pos());
		offset += thumb->boundingRect().height() + SPACING;
	}
	
	commitSceneRect();
}

void
ThumbnailSequence::Impl::invalidateThumbnail(PageId const& page_id)
{
	ItemsById::iterator const id_it(m_itemsById.find(page_id));
	if (id_it == m_itemsById.end()) {
		return;
	}
	
	std::auto_ptr<QGraphicsItem> thumb_ptr;
	if (m_ptrFactory.get()) {
		thumb_ptr = m_ptrFactory->get(id_it->pageInfo);
	}
	if (!thumb_ptr.get()) {
		thumb_ptr.reset(new PlaceholderThumb);
	}
	
	setThumbnail(id_it, thumb_ptr);
}

void
ThumbnailSequence::Impl::setThumbnail(
	ItemsById::iterator const& id_it, std::auto_ptr<QGraphicsItem> thumb_ptr)
{
	QGraphicsItem* old_thumb = id_it->thumbnail;
	QGraphicsItem* new_thumb = thumb_ptr.get();
	new_thumb->setPos(-0.5 * new_thumb->boundingRect().width(), old_thumb->pos().y());
	QRectF const old_rect(old_thumb->boundingRect());
	QRectF const new_rect(new_thumb->boundingRect());
	QPointF const pos_delta(0.0, new_rect.height() - old_rect.height());
	m_graphicsScene.removeItem(old_thumb);
	delete old_thumb;
	if (!pos_delta.isNull()) {
		ItemsInOrder::iterator ord_it(m_items.project<ItemsInOrderTag>(id_it));
		ItemsInOrder::iterator const ord_end(m_itemsInOrder.end());
		++ord_it; // skip the item itself
		for (; ord_it != ord_end; ++ord_it) {
			ord_it->thumbnail->setPos(ord_it->thumbnail->pos() + pos_delta);
		}
	}
	m_graphicsScene.addItem(thumb_ptr.release());
	id_it->thumbnail = new_thumb;
	
	m_sceneRect |= new_rect.translated(new_thumb->pos());
	commitSceneRect();
}

void
ThumbnailSequence::Impl::clear()
{
	ItemsInOrder::iterator it(m_itemsInOrder.begin());
	ItemsInOrder::iterator const end(m_itemsInOrder.end());
	while (it != end) {
		m_graphicsScene.removeItem(it->thumbnail);
		delete it->thumbnail;
		m_itemsInOrder.erase(it++);
	}
	
	assert(m_graphicsScene.items().empty());
	
	m_sceneRect = QRectF(0.0, 0.0, 0.0, 0.0);
	commitSceneRect();
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


/*================== ThumbnailSequence::PlaceholderThumb ====================*/

ThumbnailSequence::PlaceholderThumb::PlaceholderThumb()
{
}

QRectF
ThumbnailSequence::PlaceholderThumb::boundingRect() const
{
	return QRect(0.0, 0.0, 160.0, 160.0); // FIXME: don't hardcode sizes
}

void
ThumbnailSequence::PlaceholderThumb::paint(
	QPainter* painter, QStyleOptionGraphicsItem const* option,
	QWidget *widget)
{
	QSizeF const max_size(boundingRect().size());
	QSizeF const unscaled_size(
		painter->boundingRect(
			boundingRect(), Qt::AlignHCenter|Qt::AlignVCenter, "?"
		).size()
	);
	QSizeF scaled_size(unscaled_size);
	scaled_size.scale(max_size, Qt::KeepAspectRatio);
	double const hscale = scaled_size.width() / unscaled_size.width();
	double const vscale = scaled_size.height() / unscaled_size.height();
	QTransform xform;
	xform.scale(hscale, vscale);
	
	painter->setWorldTransform(xform, true);
	painter->drawText(xform.inverted().mapRect(boundingRect()), Qt::AlignHCenter|Qt::AlignVCenter, "?");
}
