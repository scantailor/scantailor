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
#include "Filter.h"
#include "FilterData.h"
#include "DebugImages.h"
#include "OptionsWidget.h"
#include "AutoManualMode.h"
#include "Dependencies.h"
#include "Params.h"
#include "Settings.h"
#include "TaskStatus.h"
#include "ContentBoxFinder.h"
#include "PageFinder.h"
#include "FilterUiInterface.h"
#include "ImageView.h"
#include "OrthogonalRotation.h"
#include "ImageTransformation.h"
#include "PhysSizeCalc.h"
#include "filters/page_layout/Task.h"
#include <QObject>
#include <QTransform>
#include <QDebug>

#include <iostream>

namespace select_content
{

class Task::UiUpdater : public FilterResult
{
public:
	UiUpdater(IntrusivePtr<Filter> const& filter,
		PageId const& page_id,
		std::auto_ptr<DebugImages> dbg,
		QImage const& image,
		ImageTransformation const& xform,
		OptionsWidget::UiData const& ui_data, bool batch);
	
	virtual void updateUI(FilterUiInterface* ui);
	
	virtual IntrusivePtr<AbstractFilter> filter() { return m_ptrFilter; }
private:
	IntrusivePtr<Filter> m_ptrFilter;
	PageId m_pageId;
	std::auto_ptr<DebugImages> m_ptrDbg;
	QImage m_image;
	QImage m_downscaledImage;
	ImageTransformation m_xform;
	OptionsWidget::UiData m_uiData;
	bool m_batchProcessing;
};


Task::Task(IntrusivePtr<Filter> const& filter,
	IntrusivePtr<page_layout::Task> const& next_task,
	IntrusivePtr<Settings> const& settings,
	PageId const& page_id, bool const batch, bool const debug)
:	m_ptrFilter(filter),
	m_ptrNextTask(next_task),
	m_ptrSettings(settings),
	m_pageId(page_id),
	m_batchProcessing(batch)
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
	
	Dependencies const deps(data.xform().resultingPreCropArea());

	OptionsWidget::UiData ui_data;
	ui_data.setSizeCalc(PhysSizeCalc(data.xform()));

	std::auto_ptr<Params> params(m_ptrSettings->getPageParams(m_pageId));
	Params new_params(deps);

	if (params.get())
	{
		/*
		new_params.setPageDetect(params->isPageDetectionEnabled());
		new_params.setFineTuneCorners(params->isFineTuningEnabled());
		new_params.setPageBorders(params->pageBorders());
		new_params.setContentDetect(params->isContentDetectionEnabled());
		new_params.setMode(params->mode());
		new_params.setContentRect(params->contentRect());
		new_params.setPageRect(params->pageRect());
*/
		new_params = *params;
		new_params.setDependencies(deps);
		if (!params->dependencies().matches(deps)) {
			goto create_new_content;
		}
	}
	else
	{
create_new_content:
		QRectF page_rect(data.xform().resultingRect());
		QRectF content_rect(page_rect);

		if (new_params.isPageDetectionEnabled()) {
			//std::cout << "PageFinder" << std::endl;
			page_rect = PageFinder::findPageBox(status, data, new_params.isFineTuningEnabled(), m_ptrSettings->pageDetectionBox(), m_ptrSettings->pageDetectionTolerance(), new_params.pageBorders(), m_ptrDbg.get());
		}

		if (new_params.isContentDetectionEnabled() && new_params.mode() == MODE_AUTO) {
			//std::cout << "ContentBoxFinder" << std::endl;
			content_rect = ContentBoxFinder::findContentBox(status, data, page_rect, m_ptrDbg.get());
		} else if (new_params.isContentDetectionEnabled() && new_params.mode() == MODE_MANUAL && new_params.contentRect().isValid()) {
			content_rect = new_params.contentRect();
		} else {
			content_rect = page_rect;
		}

		new_params.setPageRect(page_rect);
		new_params.setContentRect(content_rect);
	}

	ui_data.setContentRect(new_params.contentRect());
	ui_data.setPageRect(new_params.pageRect());
	ui_data.setDependencies(deps);
	ui_data.setMode(new_params.mode());
	ui_data.setContentDetection(new_params.isContentDetectionEnabled());
	ui_data.setPageDetection(new_params.isPageDetectionEnabled());
	ui_data.setFineTuneCorners(new_params.isFineTuningEnabled());
	ui_data.setPageBorders(new_params.pageBorders());

	new_params.setContentSizeMM(ui_data.contentSizeMM());

	new_params.computeDeviation(m_ptrSettings->avg());
	m_ptrSettings->setPageParams(m_pageId, new_params);

/*
	if (params.get() && !params->dependencies().matches(deps) && (params->mode() == MODE_AUTO || params->isPageDetectionEnabled())) {
		new_params.setMode(params->mode());
		new_params.setPageDetect(params->isPageDetectionEnabled());
		new_params.setFineTuneCorners(params->isFineTuningEnabled());
		new_params.setContentDetect(params->isContentDetectionEnabled());
        new_params.setPageBorders(params->pageBorders());
	} else if (params.get()) {
	    new_params = *params;
	    new_params.setDependencies(deps);
	}

	QRectF page_rect(data.xform().resultingRect());
	if (new_params.isPageDetectionEnabled()) {
		std::cout << "PageFinder" << std::endl;
		page_rect = PageFinder::findPageBox(status, data, new_params.isFineTuningEnabled(), m_ptrSettings->pageDetectionBox(), m_ptrSettings->pageDetectionTolerance(), new_params.pageBorders(), m_ptrDbg.get());
	}
	new_params.setPageRect(page_rect);

	QRectF content_rect(page_rect);
	if (new_params.isContentDetectionEnabled() && new_params.mode() == MODE_AUTO) {
		content_rect = ContentBoxFinder::findContentBox(status, data, page_rect, m_ptrDbg.get());
	} else if (params.get() && new_params.isContentDetectionEnabled() && new_params.mode() == MODE_MANUAL) {
		std::cout << "params->contentRect()" << std::endl;
		content_rect = params->contentRect();
	}
	new_params.setContentRect(content_rect);

	ui_data.setContentRect(content_rect);
	ui_data.setPageRect(page_rect);
	ui_data.setDependencies(deps);
	ui_data.setMode(new_params.mode());
	ui_data.setContentDetection(new_params.isContentDetectionEnabled());
	ui_data.setPageDetection(new_params.isPageDetectionEnabled());
	ui_data.setFineTuneCorners(new_params.isFineTuningEnabled());
    ui_data.setPageBorders(new_params.pageBorders());

	new_params.setContentSizeMM(ui_data.contentSizeMM());

	new_params.computeDeviation(m_ptrSettings->avg());
	m_ptrSettings->setPageParams(m_pageId, new_params);
*/
	/*
	if (params.get()) {
		ui_data.setContentRect(params->contentRect());
		ui_data.setDependencies(deps);
		ui_data.setMode(params->mode());

		if (!params->dependencies().matches(deps)) {
			QRectF content_rect = ui_data.contentRect();
			QPointF new_center= deps.rotatedPageOutline().boundingRect().center();
			QPointF old_center = params->dependencies().rotatedPageOutline().boundingRect().center();

			content_rect.translate(new_center - old_center);
			ui_data.setContentRect(content_rect);
		}

		if ((params->contentSizeMM().isEmpty() && !params->contentRect().isEmpty()) || !params->dependencies().matches(deps)) {
			// Backwards compatibilty: put the missing data where it belongs.
			Params const new_params(
				ui_data.contentRect(), ui_data.contentSizeMM(),
				deps, params->mode()
			);
			m_ptrSettings->setPageParams(m_pageId, new_params);
		}
	} else {
		QRectF const content_rect(
			ContentBoxFinder::findContentBox(
				status, data, page_rect, m_ptrDbg.get()
			)
		);
		ui_data.setContentRect(content_rect);
		ui_data.setDependencies(deps);
		ui_data.setMode(MODE_AUTO);

		Params const new_params(
			ui_data.contentRect(), ui_data.contentSizeMM(), deps, MODE_AUTO
		);
		m_ptrSettings->setPageParams(m_pageId, new_params);
	}
	*/

	status.throwIfCancelled();
	
	if (m_ptrNextTask) {
		return m_ptrNextTask->process(
			status, FilterData(data, data.xform()),
			ui_data.pageRect(), ui_data.contentRect()
		);
	} else {
		return FilterResultPtr(
			new UiUpdater(
				m_ptrFilter, m_pageId, m_ptrDbg, data.origImage(),
				data.xform(), ui_data, m_batchProcessing
			)
		);
	}
}


/*============================ Task::UiUpdater ==========================*/

Task::UiUpdater::UiUpdater(
	IntrusivePtr<Filter> const& filter, PageId const& page_id,
	std::auto_ptr<DebugImages> dbg, QImage const& image,
	ImageTransformation const& xform, OptionsWidget::UiData const& ui_data,
	bool const batch)
:	m_ptrFilter(filter),
	m_pageId(page_id),
	m_ptrDbg(dbg),
	m_image(image),
	m_downscaledImage(ImageView::createDownscaledImage(image)),
	m_xform(xform),
	m_uiData(ui_data),
	m_batchProcessing(batch)
{
}

void
Task::UiUpdater::updateUI(FilterUiInterface* ui)
{
	// This function is executed from the GUI thread.
	
	if (m_batchProcessing) {
		ui->invalidateThumbnail(m_pageId);
		return;
	}
	
	OptionsWidget* const opt_widget = m_ptrFilter->optionsWidget();
	opt_widget->postUpdateUI(m_uiData);
	ui->setOptionsWidget(opt_widget, ui->KEEP_OWNERSHIP);
	
	ui->invalidateThumbnail(m_pageId);
	
	ImageView* view = new ImageView(
		m_image, m_downscaledImage,
		m_xform, m_uiData.contentRect(), m_uiData.pageRect()
	);
	ui->setImageWidget(view, ui->TRANSFER_OWNERSHIP, m_ptrDbg.get());
	
	QObject::connect(
		view, SIGNAL(manualContentRectSet(QRectF const&)),
		opt_widget, SLOT(manualContentRectSet(QRectF const&))
	);
}

} // namespace select_content
