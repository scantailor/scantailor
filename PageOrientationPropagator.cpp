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

#include "PageOrientationPropagator.h"
#include "CompositeCacheDrivenTask.h"
#include "OrthogonalRotation.h"
#include "ProjectPages.h"
#include "PageSequence.h"
#include "PageView.h"
#include "PageInfo.h"
#include "filters/page_split/Filter.h"
#include "filter_dc/PageOrientationCollector.h"

class PageOrientationPropagator::Collector : public PageOrientationCollector
{
public:
	virtual void process(OrthogonalRotation const& orientation) {
		m_orientation = orientation;
	}
	
	OrthogonalRotation const& orientation() const { return m_orientation; }
private:
	OrthogonalRotation m_orientation;
};


PageOrientationPropagator::PageOrientationPropagator(
	IntrusivePtr<page_split::Filter> const& page_split_filter,
	IntrusivePtr<CompositeCacheDrivenTask> const& task)
:	m_ptrPageSplitFilter(page_split_filter),
	m_ptrTask(task)
{
}

PageOrientationPropagator::~PageOrientationPropagator()
{
}

void
PageOrientationPropagator::propagate(ProjectPages const& pages)
{
	PageSequence const sequence(pages.toPageSequence(PAGE_VIEW));
	size_t const num_pages = sequence.numPages();
	
	for (size_t i = 0; i < num_pages; ++i) {
		PageInfo const& page_info = sequence.pageAt(i);
		Collector collector;
		m_ptrTask->process(page_info, &collector);
		m_ptrPageSplitFilter->pageOrientationUpdate(
			page_info.imageId(), collector.orientation()
		);
	}
}
