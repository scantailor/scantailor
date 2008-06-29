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

#include "ContentBoxAggregator.h"
#include "CompositeCacheDrivenTask.h"
#include "PageSequence.h"
#include "PageInfo.h"
#include "ImageTransformation.h"
#include "Dpi.h"
#include "imageproc/Constants.h"
#include "filter_dc/ContentBoxCollector.h"
#include <QTransform>
#include <QLineF>
#include <QRectF>
#include <QSizeF>
#include <QDebug>
#include <stddef.h>
#include <math.h>

class ContentBoxAggregator::Collector : public ContentBoxCollector
{
public:
	Collector() : m_sizeSet(false) {}
	
	virtual void processContentBox(
		ImageTransformation const& xform, QRectF const& content_box);
	
	QSizeF const* sizeMM() const { return m_sizeSet ? &m_sizeMM : 0; }
private:
	QSizeF m_sizeMM;
	bool m_sizeSet;
};


ContentBoxAggregator::ContentBoxAggregator(
	IntrusivePtr<CompositeCacheDrivenTask> const& task)
:	m_ptrTask(task),
	m_haveUndefinedItems(false)
{
}

ContentBoxAggregator::~ContentBoxAggregator()
{
}

void
ContentBoxAggregator::aggregate(PageSequence const& pages)
{
	QSizeF max_size_mm;
	bool have_undefined = false;
	
	PageSequenceSnapshot snapshot(pages.snapshot(PageSequence::PAGE_VIEW));
	size_t const num_pages = snapshot.numPages();
	
	for (size_t i = 0; i < num_pages; ++i) {
		PageInfo const& page_info = snapshot.pageAt(i);
		Collector collector;
		m_ptrTask->process(page_info, &collector);
		if (QSizeF const* size = collector.sizeMM()) {
			max_size_mm = size->expandedTo(max_size_mm);
		} else {
			have_undefined = true;
		}
	}
	
	m_maxContentBoxMM = max_size_mm;
	m_haveUndefinedItems = have_undefined;
}

double
ContentBoxAggregator::lineLength(QLineF const& line, Dpi const& dpi)
{
	double const dx = (line.p1().x() - line.p2().x()) / dpi.horizontal();
	double const dy = (line.p1().y() - line.p2().y()) / dpi.vertical();
	
	return sqrt(dx * dx + dy * dy);
}


/*==================== ContentBoxAggregator::Collector =====================*/

void
ContentBoxAggregator::Collector::processContentBox(
	ImageTransformation const& xform, QRectF const& content_box)
{
	QLineF const top_line(content_box.topLeft(), content_box.topRight());
	QLineF const left_line(content_box.topLeft(), content_box.bottomLeft());
	QLineF const top_line_phys(xform.transformBack().map(top_line));
	QLineF const left_line_phys(xform.transformBack().map(left_line));
	
	Dpi const orig_dpi(xform.origDpi());
	double const width_inch = lineLength(top_line_phys, orig_dpi);
	double const height_inch = lineLength(left_line_phys, orig_dpi);
	double const width_mm = width_inch * imageproc::constants::INCH2MM;
	double const height_mm = height_inch * imageproc::constants::INCH2MM;
	
	m_sizeMM = QSizeF(width_mm, height_mm);
	m_sizeSet = true;
}
