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

#include "MainWindow.h"
#include "WorkerThread.h"
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
#include <QSettings>
#include <Qt>
#include <QDebug>
#include <vector>
#include <assert.h>

#include "filters/fix_orientation/Filter.h"
#include "filters/fix_orientation/Task.h"
#include "filters/fix_orientation/CacheDrivenTask.h"
#include "filters/page_split/Filter.h"
#include "filters/page_split/Task.h"
#include "filters/page_split/CacheDrivenTask.h"
#include "filters/deskew/Filter.h"
#include "filters/deskew/Task.h"
#include "filters/deskew/CacheDrivenTask.h"
#include "filters/select_content/Filter.h"
#include "filters/select_content/Task.h"
#include "filters/select_content/CacheDrivenTask.h"
#include "filters/page_layout/Filter.h"
#include "filters/page_layout/Task.h"
#include "filters/page_layout/CacheDrivenTask.h"
#include "filters/output/Filter.h"
#include "filters/output/Task.h"
#include "filters/output/CacheDrivenTask.h"

#include "ConsoleBatch.h"
#include "ConsoleBatch.h.moc"


ConsoleBatch::ConsoleBatch(MainWindow* main_w)
:	batch(true), debug(true), main_wnd(main_w),
	m_ptrWorkerThread(new WorkerThread()),
	m_ptrBatchQueue(new ProcessingTaskQueue(ProcessingTaskQueue::SEQUENTIAL_ORDER)),
	disambiguator(new FileNameDisambiguator)
{
	connect(
		m_ptrWorkerThread.get(),
		SIGNAL(taskResult(BackgroundTaskPtr const&, FilterResultPtr const&)),
		this, SLOT(filterResult(BackgroundTaskPtr const&, FilterResultPtr const&))
	);
};


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
			batch ? BackgroundTask::BATCH : BackgroundTask::INTERACTIVE,
			page, m_ptrThumbnailCache, m_ptrPages, fix_orientation_task
		)
	);
}


// process the image vector **images** and save output to **output_dir**
void
ConsoleBatch::process(std::vector<ImageFileInfo> const& images, QString const& output_dir, Qt::LayoutDirection const layout)
{
	IntrusivePtr<ProjectPages>         pages(new ProjectPages(images, ProjectPages::AUTO_PAGES, layout));
	StageSequence*                     stages          = new StageSequence(pages, PageSelectionAccessor(main_wnd));
	IntrusivePtr<ThumbnailPixmapCache> thumbnail_cache = IntrusivePtr<ThumbnailPixmapCache>(new ThumbnailPixmapCache(output_dir+"/cache/thumbs", QSize(200,200), 40, 5));
	OutputFileNameGenerator            out_filename_gen(disambiguator, output_dir, pages->layoutDirection());

	// Create batch queue
	PageSequence page_sequence = pages->toPageSequence(IMAGE_VIEW);
	for (int j=0; j<stages->count(); j++) {
		for (unsigned i=0; i<page_sequence.numPages(); i++) {
			PageInfo page = page_sequence.pageAt(i);
			BackgroundTaskPtr bgTask = createCompositeTask(stages, thumbnail_cache, out_filename_gen, page, pages, j);
			m_ptrBatchQueue->addProcessingTask(page, bgTask);
		}
	}

	// Start processing the first image
	BackgroundTaskPtr const task(m_ptrBatchQueue->takeForProcessing());
	m_ptrWorkerThread->performTask(task);
}


// remove finished task from queue and schedule new one
void
ConsoleBatch::filterResult(BackgroundTaskPtr const& task, FilterResultPtr const& result)
{
	m_ptrBatchQueue->processingFinished(task);

	PageInfo const page(m_ptrBatchQueue->selectedPage());
	if (!page.isNull()) {
		printf("Finished: %s\n", page.imageId().filePath().toAscii().constData());
	}

	if (isBatchProcessingInProgress()) {
		// if all images were processed, shutdown
		if (m_ptrBatchQueue->allProcessed()) {
			stopBatchProcessing();
			return;
		}

		// schedule new one
		BackgroundTaskPtr const task(m_ptrBatchQueue->takeForProcessing());
		if (task) {
			m_ptrWorkerThread->performTask(task);
		}
	}
}


bool
ConsoleBatch::isBatchProcessingInProgress() const
{
	return m_ptrBatchQueue.get() != 0;
}


// shutdown
void
ConsoleBatch::stopBatchProcessing()
{
	if (!isBatchProcessingInProgress()) {
		return;
	}
	
	printf("Stopping...\n");

	m_ptrBatchQueue->cancelAndClear();
	m_ptrBatchQueue.reset();

	main_wnd->close();
}
