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

#include "Task.h"
#include "TaskStatus.h"
#include "Filter.h"
#include "OptionsWidget.h"
#include "Settings.h"
#include "LayoutType.h"
#include "ProjectPages.h"
#include "PageInfo.h"
#include "PageId.h"
#include "PageLayoutEstimator.h"
#include "PageLayout.h"
#include "Dependencies.h"
#include "Params.h"
#include "FilterData.h"
#include "ImageMetadata.h"
#include "Dpm.h"
#include "Dpi.h"
#include "OrthogonalRotation.h"
#include "ImageTransformation.h"
#include "filters/deskew/Task.h"
#include "ImageView.h"
#include "FilterUiInterface.h"
#include "DebugImages.h"
#include <QImage>
#include <QObject>
#include <QDebug>
#include <memory>
#include <assert.h>

namespace page_split
{

using imageproc::BinaryThreshold;

class Task::UiUpdater : public FilterResult
{
public:
	UiUpdater(IntrusivePtr<Filter> const& filter,
		IntrusivePtr<ProjectPages> const& pages,
		std::auto_ptr<DebugImages> dbg_img,
		QImage const& image, PageInfo const& page_info,
		ImageTransformation const& xform,
		OptionsWidget::UiData const& ui_data,
		bool batch_processing);
	
	virtual void updateUI(FilterUiInterface* ui);
	
	virtual IntrusivePtr<AbstractFilter> filter() { return m_ptrFilter; }
private:
	IntrusivePtr<Filter> m_ptrFilter;
	IntrusivePtr<ProjectPages> m_ptrPages;
	std::auto_ptr<DebugImages> m_ptrDbg;
	QImage m_image;
	QImage m_downscaledImage;
	PageInfo m_pageInfo;
	ImageTransformation m_xform;
	OptionsWidget::UiData m_uiData;
	bool m_batchProcessing;
};

static ProjectPages::LayoutType toPageLayoutType(PageLayout const& layout)
{
	switch (layout.type()) {
		case PageLayout::SINGLE_PAGE_UNCUT:
		case PageLayout::SINGLE_PAGE_CUT:
			return ProjectPages::ONE_PAGE_LAYOUT;
		case PageLayout::TWO_PAGES:
			return ProjectPages::TWO_PAGE_LAYOUT;
	}

	assert(!"Unreachable");
	return ProjectPages::ONE_PAGE_LAYOUT;
}

Task::Task(
	IntrusivePtr<Filter> const& filter,
	IntrusivePtr<Settings> const& settings,
	IntrusivePtr<ProjectPages> const& pages,
	IntrusivePtr<deskew::Task> const& next_task,
	PageInfo const& page_info,
	bool const batch_processing, bool const debug)
:	m_ptrFilter(filter),
	m_ptrSettings(settings),
	m_ptrPages(pages),
	m_ptrNextTask(next_task),
	m_pageInfo(page_info),
	m_batchProcessing(batch_processing)
{
	if (debug) {
		m_ptrDbg.reset(new DebugImages);
	}
}

Task::~Task()
{
}

FilterResultPtr
Task::process(TaskStatus const& status, FilterData const& data)
{
	status.throwIfCancelled();
	
	Settings::Record record(m_ptrSettings->getPageRecord(m_pageInfo.imageId()));
	
	OrthogonalRotation const pre_rotation(data.xform().preRotation());
	Dependencies const deps(
		data.origImage().size(), pre_rotation,
		record.combinedLayoutType()
	);
	
	OptionsWidget::UiData ui_data;
	ui_data.setDependencies(deps);

	for (;;) {
		Params const* const params = record.params();
		
		PageLayout new_layout;
		
		if (!params || !deps.compatibleWith(*params)) {
			new_layout = PageLayoutEstimator::estimatePageLayout(
				record.combinedLayoutType(),
				data.grayImage(), data.xform(),
				data.bwThreshold(), m_ptrDbg.get()
			);
			status.throwIfCancelled();
		} else if (params->pageLayout().uncutOutline().isEmpty()) {
			// Backwards compatibility with versions < 0.9.9
			new_layout = params->pageLayout();
			new_layout.setUncutOutline(data.xform().resultingRect());
		} else {
			break;
		}
			
		Params const new_params(new_layout, deps, MODE_AUTO);
		Settings::UpdateAction update;
		update.setParams(new_params);

#ifndef NDEBUG
		{
			Settings::Record updated_record(record);
			updated_record.update(update);
			assert(!updated_record.hasLayoutTypeConflict());
			// This assert effectively verifies that PageLayoutEstimator::estimatePageLayout()
			// returned a layout with of a type consistent with the requested one.
			// If it didn't, it's a bug which will in fact cause a dead loop.
		}
#endif

		bool conflict = false;
		record = m_ptrSettings->conditionalUpdate(
			m_pageInfo.imageId(), update, &conflict
		);
		if (conflict && !record.params()) {
			// If there was a conflict, it means
			// the record was updated by another
			// thread somewhere between getPageRecord()
			// and conditionalUpdate().  If that
			// external update didn't leave page
			// parameters clear, we are just going
			// to use it's data, otherwise we need
			// to process this page again for the
			// new layout type.
			continue;
		}
		
		break;
	}
	
	PageLayout const& layout = record.params()->pageLayout();
	ui_data.setLayoutTypeAutoDetected(
		record.combinedLayoutType() == AUTO_LAYOUT_TYPE
	);
	ui_data.setPageLayout(layout);
	ui_data.setSplitLineMode(record.params()->splitLineMode());
	
	m_ptrPages->setLayoutTypeFor(m_pageInfo.imageId(), toPageLayoutType(layout));
	
	if (m_ptrNextTask) {
		ImageTransformation new_xform(data.xform());
		new_xform.setPreCropArea(layout.pageOutline(m_pageInfo.id().subPage()));
		return m_ptrNextTask->process(status, FilterData(data, new_xform));
	} else {
		return FilterResultPtr(
			new UiUpdater(
				m_ptrFilter, m_ptrPages, m_ptrDbg, data.origImage(),
				m_pageInfo, data.xform(), ui_data, m_batchProcessing
			)
		);
	}
}


/*============================ Task::UiUpdater =========================*/

Task::UiUpdater::UiUpdater(
	IntrusivePtr<Filter> const& filter,
	IntrusivePtr<ProjectPages> const& pages,
	std::auto_ptr<DebugImages> dbg_img,
	QImage const& image, PageInfo const& page_info,
	ImageTransformation const& xform,
	OptionsWidget::UiData const& ui_data,
	bool const batch_processing)
:	m_ptrFilter(filter),
	m_ptrPages(pages),
	m_ptrDbg(dbg_img),
	m_image(image),
	m_downscaledImage(ImageView::createDownscaledImage(image)),
	m_pageInfo(page_info),
	m_xform(xform),
	m_uiData(ui_data),
	m_batchProcessing(batch_processing)
{
}

void
Task::UiUpdater::updateUI(FilterUiInterface* ui)
{
	// This function is executed from the GUI thread.
	
	OptionsWidget* const opt_widget = m_ptrFilter->optionsWidget();
	opt_widget->postUpdateUI(m_uiData);
	ui->setOptionsWidget(opt_widget, ui->KEEP_OWNERSHIP);
	
	ui->invalidateThumbnail(m_pageInfo.id());
	
	if (m_batchProcessing) {
		return;
	}
	
	ImageView* view = new ImageView(
		m_image, m_downscaledImage, m_xform, m_uiData.pageLayout(),
		m_ptrPages, m_pageInfo.imageId(),
		m_pageInfo.leftHalfRemoved(), m_pageInfo.rightHalfRemoved()
	);
	ui->setImageWidget(view, ui->TRANSFER_OWNERSHIP, m_ptrDbg.get());
	
	QObject::connect(
		view, SIGNAL(invalidateThumbnail(PageInfo const&)),
		opt_widget, SIGNAL(invalidateThumbnail(PageInfo const&))
	);
	QObject::connect(
		view, SIGNAL(pageLayoutSetLocally(PageLayout const&)),
		opt_widget, SLOT(pageLayoutSetExternally(PageLayout const&))
	);
	QObject::connect(
		opt_widget, SIGNAL(pageLayoutSetLocally(PageLayout const&)),
		view, SLOT(pageLayoutSetExternally(PageLayout const&))
	);
}

} // namespace page_split
