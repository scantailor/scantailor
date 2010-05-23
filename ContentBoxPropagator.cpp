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

#include "ContentBoxPropagator.h"
#include "CompositeCacheDrivenTask.h"
#include "ProjectPages.h"
#include "PageSequence.h"
#include "PageInfo.h"
#include "ImageTransformation.h"
#include "filters/page_layout/Filter.h"
#include "filter_dc/ContentBoxCollector.h"
#include <QRectF>

class ContentBoxPropagator::Collector : public ContentBoxCollector
{
public:
	Collector();
	
	virtual void process(
		ImageTransformation const& xform,
		QRectF const& content_rect);
	
	bool collected() const { return m_collected; }
	
	ImageTransformation const& xform() const { return m_xform; }
	
	QRectF const& contentRect() const { return m_contentRect; }
private:
	ImageTransformation m_xform;
	QRectF m_contentRect;
	bool m_collected;
};


ContentBoxPropagator::ContentBoxPropagator(
	IntrusivePtr<page_layout::Filter> const& page_layout_filter,
	IntrusivePtr<CompositeCacheDrivenTask> const& task)
:	m_ptrPageLayoutFilter(page_layout_filter),
	m_ptrTask(task)
{
}

ContentBoxPropagator::~ContentBoxPropagator()
{
}

void
ContentBoxPropagator::propagate(ProjectPages const& pages)
{
	PageSequence const sequence(pages.toPageSequence(PAGE_VIEW));
	size_t const num_pages = sequence.numPages();
	
	for (size_t i = 0; i < num_pages; ++i) {
		PageInfo const& page_info = sequence.pageAt(i);
		Collector collector;
		m_ptrTask->process(page_info, &collector);
		if (collector.collected()) {
			m_ptrPageLayoutFilter->setContentBox(
				page_info.id(), collector.xform(),
				collector.contentRect()
			);
		} else {
			m_ptrPageLayoutFilter->invalidateContentBox(page_info.id());
		}
	}
}


/*=================== ContentBoxPropagator::Collector ====================*/

ContentBoxPropagator::Collector::Collector()
:	m_xform(QRectF(0, 0, 1, 1), Dpi(300, 300)),
	m_collected(false)
{
}

void
ContentBoxPropagator::Collector::process(
	ImageTransformation const& xform, QRectF const& content_rect)
{
	m_xform = xform;
	m_contentRect = content_rect;
	m_collected = true;
}
