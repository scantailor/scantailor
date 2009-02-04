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
#include "TaskStatus.h"
#include "Filter.h"
#include "OptionsWidget.h"
#include "Settings.h"
#include "Rule.h"
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

namespace page_split
{

using imageproc::BinaryThreshold;

class Task::UiUpdater : public FilterResult
{
public:
	UiUpdater(IntrusivePtr<Filter> const& filter,
		std::auto_ptr<DebugImages> dbg_img,
		QImage const& image, ImageId const& image_id,
		ImageTransformation const& xform,
		OptionsWidget::UiData const& ui_data,
		bool batch_processing);
	
	virtual void updateUI(FilterUiInterface* ui);
	
	virtual IntrusivePtr<AbstractFilter> filter() { return m_ptrFilter; }
private:
	IntrusivePtr<Filter> m_ptrFilter;
	std::auto_ptr<DebugImages> m_ptrDbg;
	QImage m_image;
	QImage m_downscaledImage;
	ImageId m_imageId;
	ImageTransformation m_xform;
	OptionsWidget::UiData m_uiData;
	bool m_batchProcessing;
};


Task::Task(
	IntrusivePtr<Filter> const& filter,
	IntrusivePtr<Settings> const& settings,
	IntrusivePtr<PageSequence> const& page_sequence,
	IntrusivePtr<deskew::Task> const& next_task,
	ImageId const& image_id,
	bool const batch_processing, bool const debug)
:	m_ptrFilter(filter),
	m_ptrSettings(settings),
	m_ptrPageSequence(page_sequence),
	m_ptrNextTask(next_task),
	m_imageId(image_id),
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
	
	OrthogonalRotation const pre_rotation(data.xform().preRotation());
	Dependencies const deps(data.origImage().size(), pre_rotation);
	
	OptionsWidget::UiData ui_data;
	ui_data.setDependencies(deps);
	
	Settings::Record record(m_ptrSettings->getPageRecord(m_imageId));
	
	for (;;) {
		Params const* const params = record.params();
		
		if (!params || !params->dependencies().matches(deps)) {
			PageLayout const layout(
				PageLayoutEstimator::estimatePageLayout(
					record.rule().layoutType(),
					data.grayImage(), data.xform(),
					data.bwThreshold(), m_ptrDbg.get()
				)
			);
			
			status.throwIfCancelled();
			
			Params const new_params(layout, deps, MODE_AUTO);
			Settings::UpdateAction update;
			update.setParams(new_params);
			bool conflict = false;
			record = m_ptrSettings->conditionalUpdate(
				m_imageId, update, &conflict
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
		}
		break;
	}
	
	PageLayout const& layout = record.params()->pageLayout();
	ui_data.setLayoutTypeAutoDetected(
		record.rule().layoutType() == Rule::AUTO_DETECT
	);
	ui_data.setPageLayout(layout);
	ui_data.setSplitLineMode(record.params()->splitLineMode());
	
	m_ptrPageSequence->setLogicalPagesInImage(
		m_imageId, layout.numSubPages()
	);
	
	if (m_ptrNextTask) {
		return m_ptrNextTask->process(status, data, layout);
	} else {
		return FilterResultPtr(
			new UiUpdater(
				m_ptrFilter, m_ptrDbg, data.origImage(), m_imageId,
				data.xform(), ui_data, m_batchProcessing
			)
		);
	}
}


/*============================ Task::UiUpdater =========================*/

Task::UiUpdater::UiUpdater(
	IntrusivePtr<Filter> const& filter,
	std::auto_ptr<DebugImages> dbg_img,
	QImage const& image, ImageId const& image_id,
	ImageTransformation const& xform,
	OptionsWidget::UiData const& ui_data,
	bool const batch_processing)
:	m_ptrFilter(filter),
	m_ptrDbg(dbg_img),
	m_image(image),
	m_downscaledImage(ImageView::createDownscaledImage(image)),
	m_imageId(image_id),
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
	
	ui->invalidateThumbnail(PageId(m_imageId));
	
	if (m_batchProcessing) {
		return;
	}
	
	ImageView* view = new ImageView(
		m_image, m_downscaledImage,
		m_xform, m_uiData.pageLayout()
	);
	ui->setImageWidget(view, ui->TRANSFER_OWNERSHIP, m_ptrDbg.get());
	
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
