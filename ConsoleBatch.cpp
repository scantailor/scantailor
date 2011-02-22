/*
    Scan Tailor - Interactive post-processing tool for scanned pages.
    Copyright (C) 2007-2009  Joseph Artsimovich <joseph_a@mail.ru>

    ConsoleBatch - Batch processing scanned pages from command line.
    Copyright (C) 2011 Petr Kovar <pejuko@gmail.com>

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

#include <vector>
#include <iostream>
#include <assert.h>

#include "ProjectPages.h"
#include "PageSelectionAccessor.h"
#include "StageSequence.h"
#include "ProcessingTaskQueue.h"
#include "FileNameDisambiguator.h"
#include "OutputFileNameGenerator.h"
#include "ImageInfo.h"
#include "ImageFileInfo.h"
#include "PageInfo.h"
#include "PageSequence.h"
#include "ImageId.h"
#include "ThumbnailPixmapCache.h"
#include "LoadFileTask.h"

#include "filters/fix_orientation/Settings.h"
#include "filters/fix_orientation/Filter.h"
#include "filters/fix_orientation/Task.h"
#include "filters/fix_orientation/CacheDrivenTask.h"
#include "filters/page_split/Filter.h"
#include "filters/page_split/Task.h"
#include "filters/page_split/CacheDrivenTask.h"
#include "filters/deskew/Settings.h"
#include "filters/deskew/Filter.h"
#include "filters/deskew/Task.h"
#include "filters/deskew/CacheDrivenTask.h"
#include "filters/select_content/Settings.h"
#include "filters/select_content/Filter.h"
#include "filters/select_content/Task.h"
#include "filters/select_content/CacheDrivenTask.h"
#include "filters/page_layout/Settings.h"
#include "filters/page_layout/Filter.h"
#include "filters/page_layout/Task.h"
#include "filters/page_layout/CacheDrivenTask.h"
#include "filters/output/Settings.h"
#include "filters/output/Params.h"
#include "filters/output/Filter.h"
#include "filters/output/Task.h"
#include "filters/output/CacheDrivenTask.h"

#include "OrthogonalRotation.h"
#include "ConsoleBatch.h"
#include "CommandLine.h"


ConsoleBatch::ConsoleBatch()
:   batch(true), debug(true),
	disambiguator(new FileNameDisambiguator)
{}

/*
ConsoleBatch::~ConsoleBatch()
{
	delete(disambiguator);
}
*/

BackgroundTaskPtr
ConsoleBatch::createCompositeTask(
		StageSequence const* m_ptrStages,
		IntrusivePtr<ThumbnailPixmapCache> const m_ptrThumbnailCache,
		OutputFileNameGenerator const& m_outFileNameGen,
		PageInfo const& page,
		IntrusivePtr<ProjectPages> const m_ptrPages,
		int const last_filter_idx)
{
	IntrusivePtr<fix_orientation::Task> fix_orientation_task;
	IntrusivePtr<page_split::Task> page_split_task;
	IntrusivePtr<deskew::Task> deskew_task;
	IntrusivePtr<select_content::Task> select_content_task;
	IntrusivePtr<page_layout::Task> page_layout_task;
	IntrusivePtr<output::Task> output_task;
	
	if (batch) {
		debug = false;
	}

	if (last_filter_idx >= m_ptrStages->outputFilterIdx()) {
		output_task = m_ptrStages->outputFilter()->createTask(
			page.id(), m_ptrThumbnailCache, m_outFileNameGen, batch, debug
		);
		debug = false;
	}
	if (last_filter_idx >= m_ptrStages->pageLayoutFilterIdx()) {
		page_layout_task = m_ptrStages->pageLayoutFilter()->createTask(
			page.id(), output_task, batch, debug
		);
		debug = false;
	}
	if (last_filter_idx >= m_ptrStages->selectContentFilterIdx()) {
		select_content_task = m_ptrStages->selectContentFilter()->createTask(
			page.id(), page_layout_task, batch, debug
		);
		debug = false;
	}
	if (last_filter_idx >= m_ptrStages->deskewFilterIdx()) {
		deskew_task = m_ptrStages->deskewFilter()->createTask(
			page.id(), select_content_task, batch, debug
		);
		debug = false;
	}
	if (last_filter_idx >= m_ptrStages->pageSplitFilterIdx()) {
		page_split_task = m_ptrStages->pageSplitFilter()->createTask(
			page, deskew_task, batch, debug
		);
		debug = false;
	}
	if (last_filter_idx >= m_ptrStages->fixOrientationFilterIdx()) {
		fix_orientation_task = m_ptrStages->fixOrientationFilter()->createTask(
			page.id(), page_split_task, batch
		);
		debug = false;
	}
	assert(fix_orientation_task);
	
	return BackgroundTaskPtr(
		new LoadFileTask(
			BackgroundTask::BATCH,
			page, m_ptrThumbnailCache, m_ptrPages, fix_orientation_task
		)
	);
}


// process the image vector **images** and save output to **output_dir**
void
ConsoleBatch::process(std::vector<ImageFileInfo> const& images, QString const& output_dir, Qt::LayoutDirection const layout)
{
	IntrusivePtr<ProjectPages> pages(new ProjectPages(images, ProjectPages::AUTO_PAGES, layout));
	PageSelectionAccessor const accessor(0);
	StageSequence* stages = setup(pages, accessor);
	IntrusivePtr<ThumbnailPixmapCache> thumbnail_cache = IntrusivePtr<ThumbnailPixmapCache>(new ThumbnailPixmapCache(output_dir+"/cache/thumbs", QSize(200,200), 40, 5));
	OutputFileNameGenerator out_filename_gen(disambiguator, output_dir, pages->layoutDirection());

	CommandLine cli;

	// it should be enough to run last two stages
	PageSequence page_sequence = pages->toPageSequence(IMAGE_VIEW);
	for (int j=4; j<stages->count(); j++) {
		if (cli["verbose"] == "true")
			std::cout << "Filter: " << j << "\n";
		for (unsigned i=0; i<page_sequence.numPages(); i++) {
			PageInfo page = page_sequence.pageAt(i);
			if (cli["verbose"] == "true")
				std::cout << "\tProcessing: " << page.imageId().filePath().toAscii().constData() << "\n";
			BackgroundTaskPtr bgTask = createCompositeTask(stages, thumbnail_cache, out_filename_gen, page, pages, j);
			(*bgTask)();
		}
	}
}

StageSequence*
ConsoleBatch::setup(IntrusivePtr<ProjectPages> pages, PageSelectionAccessor const& accessor)
{
	StageSequence* stages = new StageSequence(pages, 0);
	std::set<PageId> allPages = pages->toPageSequence(IMAGE_VIEW).selectAll();

	IntrusivePtr<fix_orientation::Filter> fix_orientation = stages->fixOrientationFilter(); 
	IntrusivePtr<page_split::Filter> page_split = stages->pageSplitFilter(); 
	IntrusivePtr<deskew::Filter> deskew = stages->deskewFilter(); 
	IntrusivePtr<select_content::Filter> select_content = stages->selectContentFilter(); 
	IntrusivePtr<page_layout::Filter> page_layout = stages->pageLayoutFilter(); 
	IntrusivePtr<output::Filter> output = stages->outputFilter(); 

	CommandLine cli;

	for (std::set<PageId>::iterator i=allPages.begin(); i!=allPages.end(); i++) {
		PageId page = *i;

		OrthogonalRotation rotation;
		// FIX ORIENTATION FILTER
		if (cli["orientation"] != "") {
			if (cli["orientation"] == "left") {
				rotation.prevClockwiseDirection();
			} else if (cli["orientation"] == "right") {
				rotation.nextClockwiseDirection();
			} else if (cli["orientation"] == "upsidedown") {
				rotation.nextClockwiseDirection();
				rotation.nextClockwiseDirection();
			}
			fix_orientation->getSettings()->applyRotation(page.imageId(), rotation);
		}
	
		// DESKEW FILTER
		if (cli["rotate"] != "" || cli["deskew"] == "manual") {
			double angle = 0.0;
			if (cli["rotate"] != "")
				angle = cli["rotate"].toDouble();
			deskew::Dependencies deps(QPolygonF(), rotation);
			deskew::Params params(angle, deps, MODE_MANUAL);
			deskew->getSettings()->setPageParams(page, params);
		}
	
		// SELECT CONTENT FILTER
		if (cli["content-box"] != "") {
			QRegExp rx("([\\d\\.]+)x([\\d\\.]+):([\\d\\.]+)x([\\d\\.]+)");
			if (rx.exactMatch(cli["content-box"])) {
				QRectF rect(rx.cap(1).toFloat(), rx.cap(2).toFloat(), rx.cap(3).toFloat(), rx.cap(4).toFloat());
				QSizeF size_mm(rx.cap(3).toFloat(), rx.cap(4).toFloat());
				select_content::Dependencies deps;
				select_content::Params params(rect, size_mm, deps, MODE_MANUAL);
				select_content->getSettings()->setPageParams(page, params);
			} else {
				std::cout << ("invalid --content-box=" + cli["content-box"] + "\n").toAscii().constData();
				exit(2);
			}
		}

		// PAGE LAYOUT FILTER
		page_layout::Alignment alignment = cli.alignment();
		if (cli["match-layout-tolerance"] != "") {
			QImage img(page.imageId().filePath());
			float imgAspectRatio = float(img.width()) / float(img.height());
			float tolerance = cli["match-layout-tolerance"].toFloat();
			float max_width = 0.0;
			float max_height = 0.0;
			std::vector<float> diffs;
			for (std::set<PageId>::iterator pi=allPages.begin(); pi!=allPages.end(); pi++) {
				ImageId pimageId = pi->imageId();
				QImage pimg(pimageId.filePath());
				float pimgAspectRatio = float(pimg.width()) / float(pimg.height());
				float diff = imgAspectRatio - pimgAspectRatio;
				if (diff < 0.0) diff *= -1;
				diffs.push_back(diff);
			}
			unsigned bad_diffs = 0;
			for (unsigned j=0; j<diffs.size(); j++) {
				if (diffs[j] > tolerance)
					bad_diffs += 1;
			}
			if (bad_diffs > (diffs.size()/2)) {
				alignment.setNull(true);
			}
		}
		page_layout->getSettings()->setHardMarginsMM(page, cli.margins());
		page_layout->getSettings()->setPageAlignment(page, alignment);

		// OUTPUT FILTER
		output::Params params(output->getSettings()->getParams(page));
		Dpi outputDpi = cli.outputDpi();
		params.setOutputDpi(outputDpi);

		output::ColorParams colorParams = params.colorParams();
		colorParams.setColorMode(cli.colorMode());

		output::ColorGrayscaleOptions cgo;
		if (cli["white-margins"] == "true")
			cgo.setWhiteMargins(true);
		if (cli["normalize-illumination"] == "true")
			cgo.setNormalizeIllumination(true);
		colorParams.setColorGrayscaleOptions(cgo);

		output::BlackWhiteOptions bwo;
		if (cli["threshold"] != "")
			bwo.setThresholdAdjustment(cli["threshold"].toInt());
		colorParams.setBlackWhiteOptions(bwo);

		params.setColorParams(colorParams);

		if (cli["despeckle"] != "")
			params.setDespeckleLevel(output::despeckleLevelFromString(cli["despeckle"]));

		if (cli["dewarping"] != "")
			params.setDewarpingMode(output::DewarpingMode(cli["dewarping"]));
		if (cli["depth-perception"] != "")
			params.setDepthPerception(output::DepthPerception(cli["depth-perception"]));

		output->getSettings()->setParams(page, params);
	}

	return stages;
}
