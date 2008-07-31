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

#include "MainWindow.h.moc"
#include "WorkerThread.h"
#include "PageSequence.h"
#include "ThumbnailSequence.h"
#include "PageInfo.h"
#include "ImageId.h"
#include "FilterOptionsWidget.h"
#include "ErrorWidget.h"
#include "DebugImages.h"
#include "BasicImageView.h"
#include "ProjectWriter.h"
#include "ProjectReader.h"
#include "ThumbnailPixmapCache.h"
#include "ThumbnailFactory.h"
#include "ContentBoxAggregator.h"
#include "PageParamsAggregator.h"
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
#include "LoadFileTask.h"
#include "CompositeCacheDrivenTask.h"
#include "ScopedIncDec.h"
#include <boost/foreach.hpp>
#include <QLineF>
#include <QStackedLayout>
#include <QLayoutItem>
#include <QAbstractListModel>
#include <QFileInfo>
#include <QDir>
#include <QString>
#include <QVariant>
#include <QModelIndex>
#include <QFileDialog>
#include <QMessageBox>
#include <QPalette>
#include <QDebug>
#include <algorithm>
#include <stddef.h>
#include <assert.h>

class MainWindow::FilterListModel : public QAbstractListModel
{
	DECLARE_NON_COPYABLE(FilterListModel)
public:
	FilterListModel(IntrusivePtr<PageSequence> const& page_sequence);
	
	virtual ~FilterListModel();
	
	std::vector<FilterPtr> const& filters() const { return m_filters; }
	
	FilterPtr const& getFilter(int filter_idx) const;
	
	int getFilterIndex(FilterPtr const& filter) const;
	
	BackgroundTaskPtr createCompositeTask(
		PageInfo const& page, int page_num,
		QString const& out_dir, int last_filter_idx,
		ThumbnailPixmapCache& thumbnail_cache,
		bool batch_processing, bool debug);
	
	IntrusivePtr<CompositeCacheDrivenTask>
	createCompositeCacheDrivenTask(int last_filter_idx);
	
	int getSelectContentFilterIdx() const { return m_selectContentFilterIdx; }
	
	int getPageLayoutFilterIdx() const { return m_pageLayoutFilterIdx; }
	
	virtual int rowCount(QModelIndex const& parent) const;
	
	virtual QVariant data(QModelIndex const& index, int role) const;
private:
	IntrusivePtr<fix_orientation::Filter> m_ptrFixOrientationFilter;
	IntrusivePtr<page_split::Filter> m_ptrPageSplitFilter;
	IntrusivePtr<deskew::Filter> m_ptrDeskewFilter;
	IntrusivePtr<select_content::Filter> m_ptrSelectContentFilter;
	IntrusivePtr<page_layout::Filter> m_ptrPageLayoutFilter;
	IntrusivePtr<output::Filter> m_ptrOutputFilter;
	std::vector<FilterPtr> m_filters;
	int m_selectContentFilterIdx;
	int m_pageLayoutFilterIdx;
};


MainWindow::MainWindow(
	std::vector<ImageFileInfo> const& files, QString const& out_dir)
:	m_ptrPages(new PageSequence(files, PageSequence::AUTO_PAGES)),
	m_frozenPages(PageSequence::IMAGE_VIEW),
	m_outDir(out_dir),
	m_ptrThumbnailCache(createThumbnailCache()),
	m_ptrWorkerThread(new WorkerThread),
	m_curFilter(0),
	m_ignoreSelectionChanges(0),
	m_debug(false),
	m_projectModified(true),
	m_batchProcessing(false)
{
	m_ptrFilterListModel.reset(new FilterListModel(m_ptrPages));
	
	construct();
}

MainWindow::MainWindow(
	QString const& project_file, ProjectReader const& project_reader)
:	m_ptrPages(project_reader.pages()),
	m_frozenPages(PageSequence::IMAGE_VIEW),
	m_outDir(project_reader.outputDirectory()),
	m_projectFile(project_file),
	m_ptrThumbnailCache(createThumbnailCache()),
	m_ptrWorkerThread(new WorkerThread),
	m_curFilter(0),
	m_ignoreSelectionChanges(0),
	m_debug(false),
	m_projectModified(false),
	m_batchProcessing(false)
{
	m_ptrFilterListModel.reset(new FilterListModel(m_ptrPages));
	project_reader.readFilterSettings(m_ptrFilterListModel->filters());
	
	construct();
}

MainWindow::~MainWindow()
{
	if (m_ptrCurTask) {
		m_ptrCurTask->cancel();
		m_ptrCurTask.reset();
	}
	m_ptrWorkerThread->shutdown();
	
	removeWidgetsFromLayout(m_pImageFrameLayout, false);
	removeWidgetsFromLayout(m_pOptionsFrameLayout, false);
	m_ptrTabbedDebugImages->clear();
}

std::auto_ptr<ThumbnailPixmapCache>
MainWindow::createThumbnailCache()
{
	QSize const max_pixmap_size(200, 200);
	
	return std::auto_ptr<ThumbnailPixmapCache>(
		new ThumbnailPixmapCache(
			m_outDir+"/cache/thumbs", max_pixmap_size, 40, 5
		)
	);
}

void
MainWindow::construct()
{
	// This needs to horizontally fit into thumbView.
	m_maxLogicalThumbSize = QSize(250, 160);
	m_ptrThumbSequence.reset(new ThumbnailSequence(m_maxLogicalThumbSize));
	
	setupUi(this);
	m_ptrTabbedDebugImages.reset(new QTabWidget);
	actionStopBatchProcessing->setEnabled(false);
	thumbView->setBackgroundBrush(palette().color(QPalette::Window));
	
	m_ptrThumbSequence->attachView(thumbView);
	
	{
		IntrusivePtr<CompositeCacheDrivenTask> const task(
			m_ptrFilterListModel->createCompositeCacheDrivenTask(
				m_ptrFilterListModel->getSelectContentFilterIdx()
			)
		);
		m_ptrContentBoxAggregator.reset(new ContentBoxAggregator(task));
	}
	{
		IntrusivePtr<CompositeCacheDrivenTask> const task(
			m_ptrFilterListModel->createCompositeCacheDrivenTask(
				m_ptrFilterListModel->getPageLayoutFilterIdx()
			)
		);
		m_ptrPageParamsAggregator.reset(new PageParamsAggregator(task));
	}
	
	addAction(actionNextPage);
	addAction(actionPrevPage);
	
	m_debug = actionDebug->isChecked();
	m_pImageFrameLayout = new QStackedLayout(imageViewFrame);
	m_pOptionsFrameLayout = new QStackedLayout(filterOptions);
	
	filterList->setModel(m_ptrFilterListModel.get());
	filterList->selectionModel()->select(
		m_ptrFilterListModel->index(0, 0),
		QItemSelectionModel::SelectCurrent
	);
	
	resetPageAndThumbSequences();
	
	connect(
		filterList->selectionModel(),
		SIGNAL(selectionChanged(QItemSelection const&, QItemSelection const&)),
		this, SLOT(filterSelectionChanged(QItemSelection const&))
	);
	
	connect(
		m_ptrWorkerThread.get(),
		SIGNAL(taskResult(BackgroundTaskPtr const&, FilterResultPtr const&)),
		this, SLOT(filterResult(BackgroundTaskPtr const&, FilterResultPtr const&))
	);
	
	connect(
		m_ptrThumbSequence.get(),
		SIGNAL(pageSelected(PageInfo const&, QRectF const&, bool, bool)),
		this, SLOT(pageSelected(PageInfo const&, QRectF const&, bool, bool))
	);
	
	connect(actionNextPage, SIGNAL(triggered(bool)), this, SLOT(nextPage()));
	connect(actionPrevPage, SIGNAL(triggered(bool)), this, SLOT(prevPage()));
	
	connect(
		actionStartBatchProcessing, SIGNAL(triggered(bool)),
		this, SLOT(startBatchProcessing())
	);
	connect(
		actionStopBatchProcessing, SIGNAL(triggered(bool)),
		this, SLOT(stopBatchProcessing())
	);
	
	connect(
		actionDebug, SIGNAL(toggled(bool)),
		this, SLOT(debugToggled(bool))
	);
	
	connect(
		actionSaveProject, SIGNAL(triggered(bool)),
		this, SLOT(saveProjectTriggered())
	);
	connect(
		actionSaveProjectAs, SIGNAL(triggered(bool)),
		this, SLOT(saveProjectAsTriggered())
	);
	
	connect(
		m_ptrPages.get(), SIGNAL(modified()),
		this, SLOT(pageSequenceModified()), Qt::QueuedConnection
	);
	
	updateWindowTitle();
	loadImage();
}

void
MainWindow::resetPageAndThumbSequences()
{
	updateFrozenPages();
	
	IntrusivePtr<CompositeCacheDrivenTask> const task(
		m_ptrFilterListModel->createCompositeCacheDrivenTask(m_curFilter)
	);
	
	m_ptrThumbSequence->setThumbnailFactory(
		IntrusivePtr<ThumbnailFactory>(
			new ThumbnailFactory(
				*m_ptrThumbnailCache, m_maxLogicalThumbSize, task
			)
		)
	);
	
	m_ptrThumbSequence->reset(m_frozenPages);
}

void
MainWindow::setOptionsWidget(FilterOptionsWidget* widget, Ownership const ownership)
{
	if (m_ptrOptionsWidget != widget) {
		removeWidgetsFromLayout(m_pOptionsFrameLayout, false);
	}
	
	// Delete the old widget we were owning, if any.
	m_optionsWidgetCleanup.clear();
	
	if (ownership == TRANSFER_OWNERSHIP) {
		m_optionsWidgetCleanup.add(widget);
	}
	
	if (m_ptrOptionsWidget == widget) {
		return;
	}
	
	if (m_ptrOptionsWidget) {
		disconnect(
			m_ptrOptionsWidget, SIGNAL(reloadRequested()),
			this, SLOT(reloadRequested())
		);
		disconnect(
			m_ptrOptionsWidget, SIGNAL(invalidateThumbnail(PageId const&)),
			this, SLOT(invalidateThumbnailSlot(PageId const&))
		);
		disconnect(
			m_ptrOptionsWidget, SIGNAL(invalidateAllThumbnails()),
			this, SLOT(invalidateAllThumbnailsSlot())
		);
		disconnect(
			m_ptrOptionsWidget, SIGNAL(goToPage(PageId const&)),
			this, SLOT(goToPage(PageId const&))
		);
	}
	
	m_pOptionsFrameLayout->addWidget(widget);
	m_ptrOptionsWidget = widget;
	
	connect(widget, SIGNAL(reloadRequested()), this, SLOT(reloadRequested()));
	connect(
		widget, SIGNAL(invalidateThumbnail(PageId const&)),
		this, SLOT(invalidateThumbnailSlot(PageId const&))
	);
	connect(
		widget, SIGNAL(invalidateAllThumbnails()),
		this, SLOT(invalidateAllThumbnailsSlot())
	);
	connect(
		widget, SIGNAL(goToPage(PageId const&)),
		this, SLOT(goToPage(PageId const&))
	);
}

void
MainWindow::setImageWidget(
	QWidget* widget, Ownership const ownership, DebugImages const* debug_images)
{
	removeWidgetsFromLayout(m_pImageFrameLayout, false);
	
	m_ptrTabbedDebugImages->clear();
	
	// Delete the old widget we were owning, if any.
	m_imageWidgetCleanup.clear();
	
	if (ownership == TRANSFER_OWNERSHIP) {
		m_imageWidgetCleanup.add(widget);
	}
	
	if (!debug_images || debug_images->items().empty()) {
		m_pImageFrameLayout->addWidget(widget);
	} else {
		m_ptrTabbedDebugImages->addTab(widget, "Main");
		BOOST_FOREACH (DebugImages::Item const& item, debug_images->items()) {
			QWidget* widget = new BasicImageView(item.image());
			m_imageWidgetCleanup.add(widget);
			m_ptrTabbedDebugImages->addTab(widget, item.label());
		}
		m_pImageFrameLayout->addWidget(m_ptrTabbedDebugImages.get());
	}
}

void
MainWindow::invalidateThumbnail(PageId const& page_id)
{
	m_ptrThumbSequence->invalidateThumbnail(page_id);
}

void
MainWindow::invalidateAllThumbnails()
{
	m_ptrThumbSequence->invalidateAllThumbnails();
}

void
MainWindow::invalidateThumbnailSlot(PageId const& page_id)
{
	invalidateThumbnail(page_id);
}

void
MainWindow::invalidateAllThumbnailsSlot()
{
	m_ptrThumbSequence->invalidateAllThumbnails();
}

void
MainWindow::nextPage()
{
	if (m_batchProcessing) {
		return;
	}
	
	int page_num = 0;
	PageInfo const next_page(
		m_ptrPages->setNextPage(m_frozenPages.view(), &page_num)
	);
	m_ptrThumbSequence->setCurrentThumbnail(next_page.id());
	loadImage(next_page, page_num);
}

void
MainWindow::prevPage()
{
	if (m_batchProcessing) {
		return;
	}
	
	int page_num = 0;
	PageInfo const prev_page(
		m_ptrPages->setPrevPage(m_frozenPages.view(), &page_num)
	);
	m_ptrThumbSequence->setCurrentThumbnail(prev_page.id());
	loadImage(prev_page, page_num);
}

void
MainWindow::goToPage(PageId const& page_id)
{
	m_ptrPages->setCurPage(page_id);
	m_ptrThumbSequence->setCurrentThumbnail(page_id);
	loadImage();
}

void
MainWindow::pageSelected(
	PageInfo const& page_info, QRectF const& thumb_rect,
	bool const by_user, bool const was_already_selected)
{
	thumbView->ensureVisible(thumb_rect, 0, 0);
	
	if (by_user) {
		m_ptrPages->setCurPage(page_info.id());
		if (m_batchProcessing) {
			stopBatchProcessing();
		} else {
			loadImage();
		}
	}
}

void
MainWindow::filterSelectionChanged(QItemSelection const& selected)
{
	if (m_ignoreSelectionChanges) {
		return;
	}
	
	if (selected.empty()) {
		return;
	}
	
	bool const was_below_select_content = isBelowSelectContent(m_curFilter);
	m_curFilter = selected.front().top();
	bool const now_below_select_content = isBelowSelectContent(m_curFilter);
	
	if (!was_below_select_content && now_below_select_content) {
		// IMPORTANT: this needs to go before resetting thumbnails,
		// because it may affect them.  The reason it may affect
		// thumbnails is that besides collecting page parameters,
		// this task propagates content rectangles from
		// "Select Content" to "Page Layout" filters, which may
		// change the aggregate page size.
		m_ptrPageParamsAggregator->aggregate(*m_ptrPages);
	}
	
	resetPageAndThumbSequences();
	/*
	if (!was_below_select_content && now_below_select_content) {
		m_ptrContentBoxAggregator->aggregate(*m_ptrPages);
	}
	*/
	updateBatchProcessingActions();
	
	loadImage();
}

void
MainWindow::reloadRequested()
{
	loadImage();
}

void
MainWindow::startBatchProcessing()
{
	if (m_ptrCurTask) {
		// The current task is being cancelled because
		// it's probably not a batch processing task.
		m_ptrCurTask->cancel();
		m_ptrCurTask.reset();
	}
	
	m_batchProcessing = true;
	updateBatchProcessingActions();
	
	splitter->widget(0)->setVisible(false);
	
	PageInfo const page_info(m_frozenPages.pageAt(0));
	m_ptrPages->setCurPage(page_info.id());
	m_ptrThumbSequence->setCurrentThumbnail(page_info.id());
	loadImage();
}

void
MainWindow::stopBatchProcessing()
{
	if (!m_batchProcessing) {
		return;
	}
	
	if (m_ptrCurTask) {
		// The current task is being cancelled because
		// it's probably a batch processing task.
		m_ptrCurTask->cancel();
		m_ptrCurTask.reset();
	}
	
	splitter->widget(0)->setVisible(true);
	
	m_batchProcessing = false;
	updateBatchProcessingActions();
	
	// The necessity to explicitly select a thumbnail comes from the fact
	// that during batch processing we select not the thumbnail that is
	// currently being processed, but the one that has been processed
	// before that.
	int page_num = 0;
	PageInfo const page_info(m_ptrPages->curPage(m_frozenPages.view(), &page_num));
	m_ptrThumbSequence->setCurrentThumbnail(page_info.id());
	loadImage(page_info, page_num);
}

void
MainWindow::filterResult(BackgroundTaskPtr const& task, FilterResultPtr const& result)
{
	if (task != m_ptrCurTask || task->isCancelled()) {
		// The page sequence may still have changed.
		updateFrozenPages();
		return;
	}
	
	if (!m_batchProcessing) {
		if (!result->filter()) {
			// Error loading file.  No special action is necessary.
		} else if (result->filter() != m_ptrFilterListModel->getFilter(m_curFilter)) {
			// Error from one of the previous filters.
			int const idx = m_ptrFilterListModel->getFilterIndex(result->filter());
			assert(idx >= 0);
			m_curFilter = idx;
			
			ScopedIncDec<int> selection_guard(m_ignoreSelectionChanges);
			filterList->selectionModel()->select(
				m_ptrFilterListModel->index(idx, 0),
				QItemSelectionModel::SelectCurrent
			);
		}
	}
	
	updateFrozenPages(); // Must be done after m_curFilter is modified.
	result->updateUI(this);
	
	if (m_batchProcessing) {
		// During batch processing we prefer to select the thumbnail
		// after it has been processed, not before.  Otherwise selected
		// thumbnail will always be an unprocessed one, with the
		// question mark on it.
		m_ptrThumbSequence->setCurrentThumbnail(m_frozenPages.curPage().id());
		
		if (m_frozenPages.curPageIdx() == m_frozenPages.numPages() - 1) {
			stopBatchProcessing();
		} else {
			int page_num = 0;
			PageInfo const page_info(
				m_ptrPages->setNextPage(m_frozenPages.view(), &page_num)
			);
			loadImage(page_info, page_num);
		}
	}
}

void
MainWindow::debugToggled(bool const enabled)
{
	m_debug = enabled;
}

void
MainWindow::saveProjectTriggered()
{
	if (m_projectFile.isEmpty()) {
		saveProjectAsTriggered();
		return;
	}
	
	if (saveProjectWithFeedback(m_projectFile)) {
		m_projectModified = false;
		updateWindowTitle();
	}
}

void
MainWindow::saveProjectAsTriggered()
{
	QDir dir(m_outDir);
	if (!m_projectFile.isEmpty()) {
		dir = QFileInfo(m_projectFile).absoluteDir();
	}
	QString project_file(
		QFileDialog::getSaveFileName(
			this, QString(), dir.absolutePath(),
			tr("Scan Tailor Projects")+" (*.ScanTailor)"
		)
	);
	if (!project_file.endsWith(".ScanTailor", Qt::CaseInsensitive)) {
		project_file += ".ScanTailor";
	}
	
	if (saveProjectWithFeedback(project_file)) {
		m_projectFile = project_file;
		m_projectModified = false;
		updateWindowTitle();
	}
}

void
MainWindow::pageSequenceModified()
{
	m_projectModified = true;
	updateWindowTitle();
}

void
MainWindow::removeWidgetsFromLayout(QLayout* layout, bool delete_widgets)
{
	QLayoutItem *child;
	while ((child = layout->takeAt(0))) {
		if (delete_widgets) {
			delete child->widget();
		}
		delete child;
	}
}

void
MainWindow::updateBatchProcessingActions()
{
#if 0
	bool const ok = !isBelowSelectContent()
		|| !m_ptrContentBoxAggregator->haveUndefinedItems();
#else
	bool const ok = true;
#endif
	actionStartBatchProcessing->setEnabled(ok && !m_batchProcessing);
	actionStopBatchProcessing->setEnabled(ok && m_batchProcessing);
}

bool
MainWindow::isBelowSelectContent() const
{
	return isBelowSelectContent(m_curFilter);
}

bool
MainWindow::isBelowSelectContent(int const filter_idx) const
{
	return (filter_idx > m_ptrFilterListModel->getSelectContentFilterIdx());
}

void
MainWindow::loadImage()
{
	int page_num = 0;
	PageInfo const page_info(
		m_ptrPages->curPage(m_frozenPages.view(), &page_num)
	);
	loadImage(page_info, page_num);
}

void
MainWindow::loadImage(PageInfo const& page, int const page_num)
{
	removeWidgetsFromLayout(m_pImageFrameLayout, false);
	m_ptrTabbedDebugImages->clear();
	m_imageWidgetCleanup.clear();
#if 0
	if (isBelowSelectContent() && m_ptrContentBoxAggregator->haveUndefinedItems()) {
		QString const err_text(
			tr("You have to process all pages with the"
			" \"Select Content\" filter first.")
		);
		setOptionsWidget(new FilterOptionsWidget, TRANSFER_OWNERSHIP);
		setImageWidget(new ErrorWidget(err_text), TRANSFER_OWNERSHIP);
		return;
	}
#endif
	
	m_ptrFilterListModel->getFilter(m_curFilter)->preUpdateUI(this, page.id());
	
	if (m_ptrCurTask) {
		m_ptrCurTask->cancel();
		m_ptrCurTask.reset();
	}
	m_ptrCurTask = m_ptrFilterListModel->createCompositeTask(
		page, page_num, m_outDir, m_curFilter, *m_ptrThumbnailCache,
		m_batchProcessing, m_debug
	);
	if (m_ptrCurTask) {
		m_ptrWorkerThread->performTask(m_ptrCurTask);
	}
}

void
MainWindow::updateFrozenPages()
{
	FilterPtr const& filter = m_ptrFilterListModel->getFilter(m_curFilter);
	m_frozenPages = m_ptrPages->snapshot(filter->getView());
}

void
MainWindow::updateWindowTitle()
{
	QString project_name;
	if (m_projectFile.isEmpty()) {
		project_name = tr("Unnamed");
	} else {
		project_name = QFileInfo(m_projectFile).baseName();
	}
	
	// We don't include "[modified]" if m_projectModified is true,
	// because very innocent things like moving to a next page will
	// in fact mark the project as modified.
	setWindowTitle(tr("%1 - Scan Tailor").arg(project_name));
}

bool
MainWindow::saveProjectWithFeedback(QString const& project_file)
{
	ProjectWriter writer(m_outDir, m_ptrPages);
	
	if (!writer.write(project_file, m_ptrFilterListModel->filters())) {
		QMessageBox::warning(
			this, tr("Error"),
			tr("Error saving the project file!")
		);
		return false;
	}
	
	return true;
}


/*====================== MainWindow::FilterListModel =====================*/

MainWindow::FilterListModel::FilterListModel(
	IntrusivePtr<PageSequence> const& page_sequence)
:	m_ptrFixOrientationFilter(new fix_orientation::Filter(page_sequence)),
	m_ptrPageSplitFilter(new page_split::Filter(page_sequence)),
	m_ptrDeskewFilter(new deskew::Filter()),
	m_ptrSelectContentFilter(new select_content::Filter()),
	m_ptrPageLayoutFilter(new page_layout::Filter(page_sequence)),
	m_ptrOutputFilter(new output::Filter())
{
	m_filters.push_back(m_ptrFixOrientationFilter);
	m_filters.push_back(m_ptrPageSplitFilter);
	m_filters.push_back(m_ptrDeskewFilter);
	
	m_selectContentFilterIdx = m_filters.size();
	m_filters.push_back(m_ptrSelectContentFilter);
	
	m_pageLayoutFilterIdx = m_filters.size();
	m_filters.push_back(m_ptrPageLayoutFilter);
	
	m_filters.push_back(m_ptrOutputFilter);
}

MainWindow::FilterListModel::~FilterListModel()
{
}

MainWindow::FilterPtr const&
MainWindow::FilterListModel::getFilter(int const filter_idx) const
{
	return m_filters[filter_idx];
}

int
MainWindow::FilterListModel::getFilterIndex(FilterPtr const& filter) const
{
	size_t const num_filters = m_filters.size();
	for (size_t i = 0; i < num_filters; ++i) {
		if (m_filters[i] == filter) {
			return i;
		}
	}
	
	return -1;
}

BackgroundTaskPtr
MainWindow::FilterListModel::createCompositeTask(
	PageInfo const& page, int const page_num,
	QString const& out_dir, int const last_filter_idx,
	ThumbnailPixmapCache& thumbnail_cache,
	bool const batch_processing, bool const debug)
{
	IntrusivePtr<fix_orientation::Task> fix_orientation_task;
	IntrusivePtr<page_split::Task> page_split_task;
	IntrusivePtr<deskew::Task> deskew_task;
	IntrusivePtr<select_content::Task> select_content_task;
	IntrusivePtr<page_layout::Task> page_layout_task;
	IntrusivePtr<output::Task> output_task;
	
	switch (last_filter_idx) {
	case 5:
		output_task = m_ptrOutputFilter->createTask(
			page.id(), page_num, out_dir, batch_processing, debug
		);
	case 4:
		page_layout_task = m_ptrPageLayoutFilter->createTask(
			page.id(), output_task, batch_processing, debug
		);
	case 3:
		select_content_task = m_ptrSelectContentFilter->createTask(
			page.id(), page_layout_task, batch_processing, debug
		);
	case 2:
		deskew_task = m_ptrDeskewFilter->createTask(
			page.id(), select_content_task, batch_processing, debug
		);
	case 1:
		page_split_task = m_ptrPageSplitFilter->createTask(
			page.id(), deskew_task, batch_processing, debug
		);
	case 0:
		fix_orientation_task = m_ptrFixOrientationFilter->createTask(
			page.id(), page_split_task, batch_processing
		);
	}
	assert(fix_orientation_task);
	
	return BackgroundTaskPtr(
		new LoadFileTask(page, thumbnail_cache, fix_orientation_task)
	);
}

IntrusivePtr<CompositeCacheDrivenTask>
MainWindow::FilterListModel::createCompositeCacheDrivenTask(int const last_filter_idx)
{
	IntrusivePtr<fix_orientation::CacheDrivenTask> fix_orientation_task;
	IntrusivePtr<page_split::CacheDrivenTask> page_split_task;
	IntrusivePtr<deskew::CacheDrivenTask> deskew_task;
	IntrusivePtr<select_content::CacheDrivenTask> select_content_task;
	IntrusivePtr<page_layout::CacheDrivenTask> page_layout_task;
	IntrusivePtr<output::CacheDrivenTask> output_task;
	
	switch (last_filter_idx) {
	case 5:
		output_task = m_ptrOutputFilter->createCacheDrivenTask();
	case 4:
		page_layout_task = m_ptrPageLayoutFilter->createCacheDrivenTask(
			output_task
		);
	case 3:
		select_content_task = m_ptrSelectContentFilter->createCacheDrivenTask(
			page_layout_task
		);
	case 2:
		deskew_task = m_ptrDeskewFilter->createCacheDrivenTask(select_content_task);
	case 1:
		page_split_task = m_ptrPageSplitFilter->createCacheDrivenTask(deskew_task);
	case 0:
		fix_orientation_task = m_ptrFixOrientationFilter->createCacheDrivenTask(
			page_split_task
		);
	}
	assert(fix_orientation_task);
	
	return fix_orientation_task;
}

int
MainWindow::FilterListModel::rowCount(QModelIndex const& parent) const
{
	return m_filters.size();
}

QVariant
MainWindow::FilterListModel::data(QModelIndex const& index, int const role) const
{
	if (role == Qt::DisplayRole) {
		return m_filters[index.row()]->getName();
	}
	return QVariant();
}
