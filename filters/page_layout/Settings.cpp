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

#include "Settings.h"
#include "PageId.h"
#include "PageSequence.h"
#include "Params.h"
#include "Margins.h"
#include "Alignment.h"
#include <QSizeF>
#include <QMutex>
#include <QMutexLocker>
#include <boost/multi_index_container.hpp>
#include <boost/multi_index/ordered_index.hpp>
#include <boost/multi_index/member.hpp>
#include <boost/multi_index/mem_fun.hpp>
#include <boost/multi_index/composite_key.hpp>
#include <algorithm>
#include <functional> // for std::greater<>
#include <stddef.h>

using namespace ::boost;
using namespace ::boost::multi_index;

namespace page_layout
{

class Settings::Item
{
public:
	PageId pageId;
	Margins hardMarginsMM;
	QSizeF contentSizeMM;
	Alignment alignment;
	
	Item(PageId const& page_id, Margins const& hard_margins_mm,
		QSizeF const& content_size_mm, Alignment const& alignment);
	
	double hardWidthMM() const;
	
	double hardHeightMM() const;
	
	double influenceHardWidthMM() const;
	
	double influenceHardHeightMM() const;
	
	bool alignedWithOthers() const { return !alignment.isNull(); }
};


class Settings::ModifyMargins
{
public:
	ModifyMargins(Margins const& margins_mm) : m_marginsMM(margins_mm) {}
	
	void operator()(Item& item) {
		item.hardMarginsMM = m_marginsMM;
	}
private:
	Margins m_marginsMM;
};


class Settings::ModifyAlignment
{
public:
	ModifyAlignment(Alignment const& alignment) : m_alignment(alignment) {}
	
	void operator()(Item& item) {
		item.alignment = m_alignment;
	}
private:
	Alignment m_alignment;
};


class Settings::ModifyContentSize
{
public:
	ModifyContentSize(QSizeF const& content_size_mm)
	: m_contentSizeMM(content_size_mm) {}
	
	void operator()(Item& item) {
		item.contentSizeMM = m_contentSizeMM;
	}
private:
	QSizeF m_contentSizeMM;
};


class Settings::Impl
{
public:
	Impl();
	
	~Impl();
	
	void clear();
	
	bool checkEverythingDefined(
		PageSequenceSnapshot const& pages, PageId const* ignore) const;
	
	std::auto_ptr<Params> getPageParams(PageId const& page_id) const;
	
	void setPageParams(PageId const& page_id, Params const& params);
	
	Params updateContentSizeAndGetParams(
		PageId const& page_id, QSizeF const& content_size_mm,
		QSizeF* agg_hard_size_before, QSizeF* agg_hard_size_after);
	
	Margins getHardMarginsMM(PageId const& page_id) const;
	
	void setHardMarginsMM(PageId const& page_id, Margins const& margins_mm);
	
	Alignment getPageAlignment(PageId const& page_id) const;
	
	void setPageAlignment(PageId const& page_id, Alignment const& alignment);
	
	AggregateSizeChanged setContentSizeMM(
		PageId const& page_id, QSizeF const& content_size_mm);
	
	void invalidateContentSize(PageId const& page_id);
	
	QSizeF getAggregateHardSizeMM() const;
	
	QSizeF getAggregateHardSizeMMLocked() const;
	
	QSizeF getAggregateHardSizeMM(
		PageId const& page_id, QSizeF const& hard_size_mm,
		Alignment const& alignment) const;
	
	PageId findWidestPage() const;
	
	PageId findTallestPage() const;
private:
	class DescWidthTag;
	class DescHeightTag;
	
	typedef multi_index_container<
		Item,
		indexed_by<
			ordered_unique<member<Item, PageId, &Item::pageId> >,
			ordered_non_unique<
				tag<DescWidthTag>,
				// ORDER BY alignedWithOthers DESC, hardWidthMM DESC
				composite_key<
					Item,
					const_mem_fun<Item, bool, &Item::alignedWithOthers>,
					const_mem_fun<Item, double, &Item::hardWidthMM>
				>,
				composite_key_compare<
					std::greater<bool>,
					std::greater<double>
				>
			>,
			ordered_non_unique<
				tag<DescHeightTag>,
				// ORDER BY alignedWithOthers DESC, hardHeightMM DESC
				composite_key<
					Item,
					const_mem_fun<Item, bool, &Item::alignedWithOthers>,
					const_mem_fun<Item, double, &Item::hardHeightMM>
				>,
				composite_key_compare<
					std::greater<bool>,
					std::greater<double>
				>
			>
		>
	> Container;
	
	typedef Container::index<DescWidthTag>::type DescWidthOrder;
	typedef Container::index<DescHeightTag>::type DescHeightOrder;
	
	mutable QMutex m_mutex;
	Container m_items;
	DescWidthOrder& m_descWidthOrder;
	DescHeightOrder& m_descHeightOrder;
	QSizeF const m_invalidSize;
	Margins const m_defaultHardMarginsMM;
	Alignment const m_defaultAlignment;
};


/*=============================== Settings ==================================*/

Settings::Settings()
:	m_ptrImpl(new Impl())
{
}

Settings::~Settings()
{
}

void
Settings::clear()
{
	return m_ptrImpl->clear();
}

bool
Settings::checkEverythingDefined(
	PageSequenceSnapshot const& pages, PageId const* ignore) const
{
	return m_ptrImpl->checkEverythingDefined(pages, ignore);
}

std::auto_ptr<Params>
Settings::getPageParams(PageId const& page_id) const
{
	return m_ptrImpl->getPageParams(page_id);
}

void
Settings::setPageParams(PageId const& page_id, Params const& params)
{
	return m_ptrImpl->setPageParams(page_id, params);
}

Params
Settings::updateContentSizeAndGetParams(
	PageId const& page_id, QSizeF const& content_size_mm,
	QSizeF* agg_hard_size_before, QSizeF* agg_hard_size_after)
{
	return m_ptrImpl->updateContentSizeAndGetParams(
		page_id, content_size_mm,
		agg_hard_size_before, agg_hard_size_after
	);
}

Margins
Settings::getHardMarginsMM(PageId const& page_id) const
{
	return m_ptrImpl->getHardMarginsMM(page_id);
}

void
Settings::setHardMarginsMM(PageId const& page_id, Margins const& margins_mm)
{
	m_ptrImpl->setHardMarginsMM(page_id, margins_mm);
}

Alignment
Settings::getPageAlignment(PageId const& page_id) const
{
	return m_ptrImpl->getPageAlignment(page_id);
}

void
Settings::setPageAlignment(PageId const& page_id, Alignment const& alignment)
{
	m_ptrImpl->setPageAlignment(page_id, alignment);
}

Settings::AggregateSizeChanged
Settings::setContentSizeMM(
	PageId const& page_id, QSizeF const& content_size_mm)
{
	return m_ptrImpl->setContentSizeMM(page_id, content_size_mm);
}

void
Settings::invalidateContentSize(PageId const& page_id)
{
	return m_ptrImpl->invalidateContentSize(page_id);
}

QSizeF
Settings::getAggregateHardSizeMM() const
{
	return m_ptrImpl->getAggregateHardSizeMM();
}

QSizeF
Settings::getAggregateHardSizeMM(
	PageId const& page_id, QSizeF const& hard_size_mm,
	Alignment const& alignment) const
{
	return m_ptrImpl->getAggregateHardSizeMM(page_id, hard_size_mm, alignment);
}

PageId
Settings::findWidestPage() const
{
	return m_ptrImpl->findWidestPage();
}

PageId
Settings::findTallestPage() const
{
	return m_ptrImpl->findTallestPage();
}


/*============================== Settings::Item =============================*/

Settings::Item::Item(
	PageId const& page_id, Margins const& hard_margins_mm,
	QSizeF const& content_size_mm, Alignment const& align)
:	pageId(page_id),
	hardMarginsMM(hard_margins_mm),
	contentSizeMM(content_size_mm),
	alignment(align)
{
}

double
Settings::Item::hardWidthMM() const
{
	return contentSizeMM.width() + hardMarginsMM.left() + hardMarginsMM.right();
}

double
Settings::Item::hardHeightMM() const
{
	return contentSizeMM.height() + hardMarginsMM.top() + hardMarginsMM.bottom();
}

double
Settings::Item::influenceHardWidthMM() const
{
	return alignment.isNull() ? 0.0 : hardWidthMM();
}

double
Settings::Item::influenceHardHeightMM() const
{
	return alignment.isNull() ? 0.0 : hardHeightMM();
}


/*============================= Settings::Impl ==============================*/

Settings::Impl::Impl()
:	m_items(),
	m_descWidthOrder(m_items.get<DescWidthTag>()),
	m_descHeightOrder(m_items.get<DescHeightTag>()),
	m_invalidSize(),
	m_defaultHardMarginsMM(10.0, 5.0, 10.0, 5.0),
	m_defaultAlignment(Alignment::TOP, Alignment::HCENTER)
{
}

Settings::Impl::~Impl()
{
}

void
Settings::Impl::clear()
{
	QMutexLocker const locker(&m_mutex);
	m_items.clear();
}

bool
Settings::Impl::checkEverythingDefined(
	PageSequenceSnapshot const& pages, PageId const* ignore) const
{
	QMutexLocker const locker(&m_mutex);
	
	size_t const num_pages = pages.numPages();
	for (size_t i = 0; i < num_pages; ++i) {
		PageInfo const& page_info = pages.pageAt(i);
		if (ignore && *ignore == page_info.id()) {
			continue;
		}
		Container::iterator const it(m_items.find(page_info.id()));
		if (it == m_items.end() || !it->contentSizeMM.isValid()) {
			return false;
		}
	}
	
	return true;
}

std::auto_ptr<Params>
Settings::Impl::getPageParams(PageId const& page_id) const
{
	QMutexLocker const locker(&m_mutex);
	
	Container::iterator const it(m_items.find(page_id));
	if (it == m_items.end()) {
		return std::auto_ptr<Params>();
	}
	
	return std::auto_ptr<Params>(
		new Params(it->hardMarginsMM, it->contentSizeMM, it->alignment)
	);
}

void
Settings::Impl::setPageParams(PageId const& page_id, Params const& params)
{
	QMutexLocker const locker(&m_mutex);
	
	Item const new_item(
		page_id, params.hardMarginsMM(),
		params.contentSizeMM(), params.alignment()
	);
	
	Container::iterator const it(m_items.lower_bound(page_id));
	if (it == m_items.end() || page_id < it->pageId) {
		m_items.insert(it, new_item);
	} else {
		m_items.replace(it, new_item);
	}
}

Params
Settings::Impl::updateContentSizeAndGetParams(
	PageId const& page_id, QSizeF const& content_size_mm,
	QSizeF* agg_hard_size_before, QSizeF* agg_hard_size_after)
{
	QMutexLocker const locker(&m_mutex);
	
	if (agg_hard_size_before) {
		*agg_hard_size_before = getAggregateHardSizeMMLocked();
	}
	
	Container::iterator const it(m_items.lower_bound(page_id));
	Container::iterator item_it(it);
	if (it == m_items.end() || page_id < it->pageId) {
		Item const item(
			page_id, m_defaultHardMarginsMM,
			content_size_mm, m_defaultAlignment
		);
		item_it = m_items.insert(it, item);
	} else {
		m_items.modify(it, ModifyContentSize(content_size_mm));
	}
	
	if (agg_hard_size_after) {
		*agg_hard_size_after = getAggregateHardSizeMMLocked();
	}
	
	return Params(
		item_it->hardMarginsMM,
		item_it->contentSizeMM, item_it->alignment
	);
}

Margins
Settings::Impl::getHardMarginsMM(PageId const& page_id) const
{
	QMutexLocker const locker(&m_mutex);
	
	Container::iterator const it(m_items.find(page_id));
	if (it == m_items.end()) {
		return m_defaultHardMarginsMM;
	} else {
		return it->hardMarginsMM;
	}
}

void
Settings::Impl::setHardMarginsMM(
	PageId const& page_id, Margins const& margins_mm)
{
	QMutexLocker const locker(&m_mutex);
	
	Container::iterator const it(m_items.lower_bound(page_id));
	if (it == m_items.end() || page_id < it->pageId) {
		Item const item(
			page_id, margins_mm, m_invalidSize, m_defaultAlignment
		);
		m_items.insert(it, item);
	} else {
		m_items.modify(it, ModifyMargins(margins_mm));
	}
}

Alignment
Settings::Impl::getPageAlignment(PageId const& page_id) const
{
	QMutexLocker const locker(&m_mutex);
	
	Container::iterator const it(m_items.find(page_id));
	if (it == m_items.end()) {
		return m_defaultAlignment;
	} else {
		return it->alignment;
	}
}

void
Settings::Impl::setPageAlignment(
	PageId const& page_id, Alignment const& alignment)
{
	QMutexLocker const locker(&m_mutex);
	
	Container::iterator const it(m_items.lower_bound(page_id));
	if (it == m_items.end() || page_id < it->pageId) {
		Item const item(
			page_id, m_defaultHardMarginsMM, m_invalidSize, alignment
		);
		m_items.insert(it, item);
	} else {
		m_items.modify(it, ModifyAlignment(alignment));
	}
}

Settings::AggregateSizeChanged
Settings::Impl::setContentSizeMM(
	PageId const& page_id, QSizeF const& content_size_mm)
{
	QMutexLocker const locker(&m_mutex);
	
	QSizeF const agg_size_before(getAggregateHardSizeMMLocked());
	
	Container::iterator const it(m_items.lower_bound(page_id));
	if (it == m_items.end() || page_id < it->pageId) {
		Item const item(
			page_id, m_defaultHardMarginsMM,
			content_size_mm, m_defaultAlignment
		);
		m_items.insert(it, item);
	} else {
		m_items.modify(it, ModifyContentSize(content_size_mm));
	}
	
	QSizeF const agg_size_after(getAggregateHardSizeMMLocked());
	if (agg_size_before == agg_size_after) {
		return AGGREGATE_SIZE_UNCHANGED;
	} else {
		return AGGREGATE_SIZE_CHANGED;
	}
}

void
Settings::Impl::invalidateContentSize(PageId const& page_id)
{
	QMutexLocker const locker(&m_mutex);
	
	Container::iterator const it(m_items.find(page_id));
	if (it != m_items.end()) {
		m_items.modify(it, ModifyContentSize(m_invalidSize));
	}
}

QSizeF
Settings::Impl::getAggregateHardSizeMM() const
{
	QMutexLocker const locker(&m_mutex);
	return getAggregateHardSizeMMLocked();
}

QSizeF
Settings::Impl::getAggregateHardSizeMMLocked() const
{
	if (m_items.empty()) {
		return QSizeF(0.0, 0.0);
	}
	
	Item const& max_width_item = *m_descWidthOrder.begin();
	Item const& max_height_item = *m_descHeightOrder.begin();
	
	double const width = max_width_item.influenceHardWidthMM();
	double const height = max_height_item.influenceHardHeightMM();
	
	return QSizeF(width, height);
}

QSizeF
Settings::Impl::getAggregateHardSizeMM(
	PageId const& page_id, QSizeF const& hard_size_mm,
	Alignment const& alignment) const
{
	if (alignment.isNull()) {
		return getAggregateHardSizeMM();
	}
	
	QMutexLocker const locker(&m_mutex);
	
	if (m_items.empty()) {
		return QSizeF(0.0, 0.0);
	}
	
	double width = 0.0;
	
	{
		DescWidthOrder::iterator it(m_descWidthOrder.begin());
		if (it->pageId != page_id) {
			width = it->influenceHardWidthMM();
		} else {
			++it;
			if (it == m_descWidthOrder.end()) {
				width = hard_size_mm.width();
			} else {
				width = std::max(
					hard_size_mm.width(), it->influenceHardWidthMM()
				);
			}
		}
	}
	
	double height = 0.0;
	
	{
		DescHeightOrder::iterator it(m_descHeightOrder.begin());
		if (it->pageId != page_id) {
			height = it->influenceHardHeightMM();
		} else {
			++it;
			if (it == m_descHeightOrder.end()) {
				height = hard_size_mm.height();
			} else {
				height = std::max(
					hard_size_mm.height(), it->influenceHardHeightMM()
				);
			}
		}
	}
	
	return QSizeF(width, height);
}

PageId
Settings::Impl::findWidestPage() const
{
	QMutexLocker const locker(&m_mutex);
	
	if (m_items.empty()) {
		return PageId();
	}
	
	return m_descWidthOrder.begin()->pageId;
}

PageId
Settings::Impl::findTallestPage() const
{
	QMutexLocker const locker(&m_mutex);
	
	if (m_items.empty()) {
		return PageId();
	}
	
	return m_descHeightOrder.begin()->pageId;
}

} // namespace page_layout
