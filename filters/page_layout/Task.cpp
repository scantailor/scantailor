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

#include "Task.h"
#include "Filter.h"
#include "OptionsWidget.h"
#include "FilterUiInterface.h"
#include "TaskStatus.h"
#include "FilterData.h"
#include "BasicImageView.h"

namespace page_layout
{

class Task::UiUpdater : public FilterResult
{
public:
	UiUpdater(IntrusivePtr<Filter> const& filter,
		PageId const& page_id,
		QImage const& image,
		ImageTransformation const& xform, bool batch);
	
	virtual void updateUI(FilterUiInterface* ui);
	
	virtual IntrusivePtr<AbstractFilter> filter() { return m_ptrFilter; }
private:
	IntrusivePtr<Filter> m_ptrFilter;
	PageId m_pageId;
	QImage m_image;
	ImageTransformation m_xform;
	bool m_batchProcessing;
};


Task::Task(IntrusivePtr<Filter> const& filter,
	PageId const& page_id, bool batch, bool debug)
:	m_ptrFilter(filter),
	m_pageId(page_id),
	m_batchProcessing(batch)
{
}

Task::~Task()
{
}

FilterResultPtr
Task::process(TaskStatus const& status, FilterData const& data)
{
	status.throwIfCancelled();
	
	return FilterResultPtr(
		new UiUpdater(
			m_ptrFilter, m_pageId, data.image(),
			data.xform(), m_batchProcessing
		)
	);
}


/*============================ Task::UiUpdater ==========================*/

Task::UiUpdater::UiUpdater(
	IntrusivePtr<Filter> const& filter, PageId const& page_id,
	QImage const& image, ImageTransformation const& xform, bool const batch)
:	m_ptrFilter(filter),
	m_pageId(page_id),
	m_image(image),
	m_xform(xform),
	m_batchProcessing(batch)
{
}

void
Task::UiUpdater::updateUI(FilterUiInterface* ui)
{
	// This function is executed from the GUI thread.
	
	OptionsWidget* const opt_widget = m_ptrFilter->optionsWidget();
	//opt_widget->postUpdateUI(m_uiData);
	ui->setOptionsWidget(opt_widget, ui->KEEP_OWNERSHIP);
	
	ui->invalidateThumbnail(m_pageId);
	
	if (m_batchProcessing) {
		return;
	}
	
	BasicImageView* view = new BasicImageView(m_image);
	ui->setImageWidget(view, ui->TRANSFER_OWNERSHIP);
}

} // namespace page_layout
