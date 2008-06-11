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
#include "ImageId.h"
#include "LogicalPageId.h"
#include "Utils.h"
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
#include <QDebug>
#include <stddef.h>
#include <assert.h>

using namespace ::boost;
using namespace ::boost::multi_index;


class ThumbnailSequence::Item
{
public:
	Item(PageInfo const& page_info, CompositeItem* comp_item, bool tag = false)
	: pageInfo(page_info), composite(comp_item), tagged(tag) {}
	
	LogicalPageId const& pageId() const { return pageInfo.id(); }
	
	PageInfo pageInfo;
	mutable CompositeItem* composite;
	mutable bool tagged;
};


class ThumbnailSequence::Impl
{
public:
	Impl();
	
	~Impl();
	
	void setThumbnailFactory(IntrusivePtr<ThumbnailFactory> const& factory);
	
	void attachView(QGraphicsView* view);
	
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
		std::auto_ptr<CompositeItem> composite);
	
	void clear();
	
	std::auto_ptr<QGraphicsItem> getThumbnail(PageInfo const& page_info);
	
	std::auto_ptr<QGraphicsItem> getLabel(PageInfo const& page_info);
	
	std::auto_ptr<CompositeItem> getCompositeItem(PageInfo const& page_info);
	
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
private:
	static QPainterPath m_sCachedPath;
};


class ThumbnailSequence::CompositeItem : public QGraphicsItemGroup
{
public:
	CompositeItem(
		std::auto_ptr<QGraphicsItem> thumbnail,
		std::auto_ptr<QGraphicsItem> label);
	
	void updateSceneRect(QRectF& scene_rect);
private:
	QGraphicsItem* m_pThumb;
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
ThumbnailSequence::Impl::reset(PageSequenceSnapshot const& pages)
{
	clear();
	
	double offset = 0;
	size_t const num_pages = pages.numPages();
	for (size_t i = 0; i < num_pages; ++i) {
		PageInfo const& page_info(pages.pageAt(i));
		
		std::auto_ptr<CompositeItem> composite(getCompositeItem(page_info));
		composite->setPos(0.0, offset);
		composite->updateSceneRect(m_sceneRect);
		
		offset += composite->boundingRect().height() + SPACING;
		
		m_itemsInOrder.push_back(Item(page_info, composite.get()));
		m_graphicsScene.addItem(composite.release());
	}
	
	commitSceneRect();
}

void
ThumbnailSequence::Impl::invalidateThumbnail(PageId const& page_id)
{
	ItemsById::iterator const id_it(m_itemsById.find(page_id));
	if (id_it != m_itemsById.end()) {
		setThumbnail(id_it, getCompositeItem(id_it->pageInfo));
	}
}

void
ThumbnailSequence::Impl::setThumbnail(
	ItemsById::iterator const& id_it, std::auto_ptr<CompositeItem> composite)
{
	QGraphicsItem* const old_composite = id_it->composite;
	CompositeItem* const new_composite = composite.get();
	QSizeF const old_size(old_composite->boundingRect().size());
	QSizeF const new_size(new_composite->boundingRect().size());
	
	new_composite->setPos(old_composite->pos());
	composite->updateSceneRect(m_sceneRect);
	
	QPointF const pos_delta(0.0, new_size.height() - old_size.height());
	
	m_graphicsScene.removeItem(old_composite);
	delete old_composite;
	
	if (!pos_delta.isNull()) {
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
	ItemsInOrder::iterator it(m_itemsInOrder.begin());
	ItemsInOrder::iterator const end(m_itemsInOrder.end());
	while (it != end) {
		m_graphicsScene.removeItem(it->composite);
		delete it->composite;
		m_itemsInOrder.erase(it++);
	}
	
	assert(m_graphicsScene.items().empty());
	
	m_sceneRect = QRectF(0.0, 0.0, 0.0, 0.0);
	commitSceneRect();
}

std::auto_ptr<QGraphicsItem>
ThumbnailSequence::Impl::getThumbnail(PageInfo const& page_info)
{
	std::auto_ptr<QGraphicsItem> thumb;
	
	if (m_ptrFactory.get()) {
		thumb = m_ptrFactory->get(page_info);
	}
	
	if (!thumb.get()) {
		thumb.reset(new PlaceholderThumb);
	}
	
	return thumb;
}

std::auto_ptr<QGraphicsItem>
ThumbnailSequence::Impl::getLabel(PageInfo const& page_info)
{
	LogicalPageId const& page_id = page_info.id();
	QFileInfo const file_info(page_id.imageId().filePath());
	QString const file_name(file_info.fileName());
	int const page_num = page_id.imageId().page();
	
	QString text(file_name);
	if (page_info.isMultiPageFile() || page_num > 0) {
		text = QObject::tr("%1 (page %2)").arg(file_name).arg(page_num + 1);
	}
	
	std::auto_ptr<QGraphicsSimpleTextItem> text_item(new QGraphicsSimpleTextItem);
	text_item->setText(text);
	QSizeF const text_item_size(text_item->boundingRect().size());
	
	char const* pixmap_resource = 0;
	switch (page_id.subPage()) {
		case LogicalPageId::SINGLE_PAGE:
			return std::auto_ptr<QGraphicsItem>(text_item);
		case LogicalPageId::LEFT_PAGE:
			pixmap_resource = ":/icons/left_page_thumb.png";
			break;
		case LogicalPageId::RIGHT_PAGE:
			pixmap_resource = ":/icons/right_page_thumb.png";
			break;
	}
	
	QPixmap pixmap;
	Utils::loadAndCachePixmap(pixmap, pixmap_resource);
	
	int const label_pixmap_spacing = 5;
	std::auto_ptr<QGraphicsPixmapItem> pixmap_item(new QGraphicsPixmapItem(pixmap));
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
	
	std::auto_ptr<QGraphicsItemGroup> group(new QGraphicsItemGroup);
	group->addToGroup(text_item.release());
	group->addToGroup(pixmap_item.release());
	
	return std::auto_ptr<QGraphicsItem>(group);
}

std::auto_ptr<ThumbnailSequence::CompositeItem>
ThumbnailSequence::Impl::getCompositeItem(PageInfo const& page_info)
{
	std::auto_ptr<QGraphicsItem> thumb(getThumbnail(page_info));
	std::auto_ptr<QGraphicsItem> label(getLabel(page_info));
	return std::auto_ptr<CompositeItem>(new CompositeItem(thumb, label));
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

QPainterPath ThumbnailSequence::PlaceholderThumb::m_sCachedPath;

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
	QString const text(QString::fromAscii("?"));
	QRectF const bounding_rect(boundingRect());
	
	// Because painting happens only from the main thread, we don't
	// need to care about concurrent access.
	if (m_sCachedPath.isEmpty()) {
		QFont font(painter->font());
		font.setWeight(QFont::DemiBold);
		font.setStyleStrategy(QFont::ForceOutline);
		m_sCachedPath.addText(0, 0, font, text);
	}
	QRectF const text_rect(m_sCachedPath.boundingRect());
	
	QTransform xform1;
	xform1.translate(-text_rect.left(), -text_rect.top());
	
	QSizeF const unscaled_size(text_rect.size());
	QSizeF scaled_size(unscaled_size);
	scaled_size.scale(bounding_rect.size() * 0.9, Qt::KeepAspectRatio);
	
	double const hscale = scaled_size.width() / unscaled_size.width();
	double const vscale = scaled_size.height() / unscaled_size.height();
	QTransform xform2;
	xform2.scale(hscale, vscale);
	
	// Position the text at the center of our bounding rect.
	QSizeF const translation(bounding_rect.size() * 0.5 - scaled_size * 0.5);
	QTransform xform3;
	xform3.translate(translation.width(), translation.height());
	
	painter->setWorldTransform(xform1 * xform2 * xform3, true);
	painter->setRenderHint(QPainter::Antialiasing);
	
	QPen pen(QColor(0x00, 0x00, 0x00, 60));
	pen.setWidth(2);
	pen.setCosmetic(true);
	painter->setPen(pen);
	
	painter->drawPath(m_sCachedPath);
}


/*==================== ThumbnailSequence::CompositeItem =====================*/

ThumbnailSequence::CompositeItem::CompositeItem(
	std::auto_ptr<QGraphicsItem> thumbnail, std::auto_ptr<QGraphicsItem> label)
:	m_pThumb(thumbnail.get())
{
	QSizeF const thumb_size(thumbnail->boundingRect().size());
	QSizeF const label_size(label->boundingRect().size());
	
	int const thumb_label_spacing = 1;
	thumbnail->setPos(-0.5 * thumb_size.width(), 0.0);
	label->setPos(
		thumbnail->pos().x() + thumb_size.width() - label_size.width(),
		thumb_size.height() + thumb_label_spacing
	);
	
	addToGroup(thumbnail.release());
	addToGroup(label.release());
}

void
ThumbnailSequence::CompositeItem::updateSceneRect(QRectF& scene_rect)
{
	QRectF rect(m_pThumb->boundingRect());
	rect.translate(m_pThumb->pos());
	rect.translate(pos());
	
	scene_rect |= rect;
}

