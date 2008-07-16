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

#include "PageParamsAggregator.h"
#include "CompositeCacheDrivenTask.h"
#include "PageSequence.h"
#include "PageInfo.h"
#include "ImageTransformation.h"
#include "filter_dc/PageLayoutParamsCollector.h"
#include "filters/page_layout/Params.h"
#include <QTransform>
#include <QLineF>
#include <QRectF>
#include <QSizeF>
#include <QDebug>
#include <memory>
#include <stddef.h>
#include <math.h>

using page_layout::Params;

class PageParamsAggregator::Collector : public PageLayoutParamsCollector
{
public:
	virtual void processPageParams(Params const& params);
	
	std::auto_ptr<Params> retrieveParams() { return m_ptrParams; }
private:
	std::auto_ptr<Params> m_ptrParams;
};


PageParamsAggregator::PageParamsAggregator(
	IntrusivePtr<CompositeCacheDrivenTask> const& task)
:	m_ptrTask(task),
	m_haveUndefinedItems(false)
{
}

PageParamsAggregator::~PageParamsAggregator()
{
}

void
PageParamsAggregator::aggregate(PageSequence const& pages)
{
	bool have_undefined = false;
	
	PageSequenceSnapshot snapshot(pages.snapshot(PageSequence::PAGE_VIEW));
	size_t const num_pages = snapshot.numPages();
	
	for (size_t i = 0; i < num_pages; ++i) {
		PageInfo const& page_info = snapshot.pageAt(i);
		Collector collector;
		m_ptrTask->process(page_info, &collector);
		// TODO: do some kind of aggregation
		if (!collector.retrieveParams().get()) {
			have_undefined = true;
		}
	}
	
	m_haveUndefinedItems = have_undefined;
}


/*==================== PageParamsAggregator::Collector =====================*/

void
PageParamsAggregator::Collector::processPageParams(Params const& params)
{
	m_ptrParams.reset(new Params(params));
}
