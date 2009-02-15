/*
    Scan Tailor - Interactive post-processing tool for scanned pages.
    Copyright (C) 2007-2009  Joseph Artsimovich <joseph_a@mail.ru>

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

#include "config.h"
#include "MainWindow.h.moc"
#include "NewOpenProjectPanel.h"
#include "RecentProjects.h"
#include "WorkerThread.h"
#include "PageSequence.h"
#include "ThumbnailSequence.h"
#include "PageInfo.h"
#include "ImageId.h"
#include "Utils.h"
#include "FilterOptionsWidget.h"
#include "ErrorWidget.h"
#include "DebugImages.h"
#include "BasicImageView.h"
#include "ProjectWriter.h"
#include "ProjectReader.h"
#include "ThumbnailPixmapCache.h"
#include "ThumbnailFactory.h"
#include "ContentBoxPropagator.h"
#include "ProjectCreationContext.h"
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
#include <QWidget>
#include <QCloseEvent>
#include <QStackedLayout>
#include <QGridLayout>
#include <QLayoutItem>
#include <QAbstractListModel>
#include <QScrollBar>
#include <QFileInfo>
#include <QFile>
#include <QDir>
#include <QString>
#include <QByteArray>
#include <QVariant>
#include <QModelIndex>
#include <QFileDialog>
#include <QMessageBox>
#include <QPalette>
#include <QSettings>
#include <QDomDocument>
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
		IntrusivePtr<PageSequence> const& page_sequence,
		bool batch_processing, bool debug);
	
	IntrusivePtr<CompositeCacheDrivenTask>
	createCompositeCacheDrivenTask(QString const& out_dir, int last_filter_idx);
	
	IntrusivePtr<page_layout::Filter> const& getPageLayoutFilter() const {
		return m_ptrPageLayoutFilter;
	}
	
	int getSelectContentFilterIdx() const { return m_selectContentFilterIdx; }
	
	int getPageLayoutFilterIdx() const { return m_pageLayoutFilterIdx; }
	
	int getOutputFilterIdx() const { return m_outputFilterIdx; }
	
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
	int m_outputFilterIdx;
};


MainWindow::MainWindow()
:	m_ptrPages(new PageSequence),
	m_ptrWorkerThread(new WorkerThread),
	m_curFilter(0),
	m_ignoreSelectionChanges(0),
	m_debug(false),
	m_batchProcessing(false),
	m_closing(false)
{
	m_ptrFilterListModel.reset(new FilterListModel(m_ptrPages));
	
	m_maxLogicalThumbSize = QSize(250, 160);
	m_ptrThumbSequence.reset(new ThumbnailSequence(m_maxLogicalThumbSize));
	
	setupUi(this);
	
	setupThumbView(); // Expects m_ptrThumbSequence to be initialized.
	
	m_ptrTabbedDebugImages.reset(new QTabWidget);
	actionStopBatchProcessing->setEnabled(false);
	
	m_debug = actionDebug->isChecked();
	m_pImageFrameLayout = new QStackedLayout(imageViewFrame);
	m_pOptionsFrameLayout = new QStackedLayout(filterOptions);
	
	filterList->setModel(m_ptrFilterListModel.get());
	filterList->selectionModel()->select(
		m_ptrFilterListModel->index(0, 0),
		QItemSelectionModel::SelectCurrent
	);
	
	addAction(actionNextPage);
	addAction(actionPrevPage);
	addAction(actionPrevPageQ);
	addAction(actionNextPageW);
	
	connect(actionPrevPage, SIGNAL(triggered(bool)), this, SLOT(prevPage()));
	connect(actionNextPage, SIGNAL(triggered(bool)), this, SLOT(nextPage()));
	connect(actionPrevPageQ, SIGNAL(triggered(bool)), this, SLOT(prevPage()));
	connect(actionNextPageW, SIGNAL(triggered(bool)), this, SLOT(nextPage()));
	
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
	
	connect(
		thumbView->verticalScrollBar(), SIGNAL(sliderMoved(int)),
		this, SLOT(thumbViewScrolled())
	);
	connect(
		thumbView->verticalScrollBar(), SIGNAL(valueChanged(int)),
		this, SLOT(thumbViewScrolled())
	);
	connect(
		focusButton, SIGNAL(clicked(bool)),
		this, SLOT(thumbViewFocusToggled(bool))
	);
	
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
		actionNewProject, SIGNAL(triggered(bool)),
		this, SLOT(newProject())
	);
	connect(
		actionOpenProject, SIGNAL(triggered(bool)),
		this, SLOT(openProject())
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
		actionCloseProject, SIGNAL(triggered(bool)),
		this, SLOT(closeProject())
	);
	connect(
		actionQuit, SIGNAL(triggered(bool)),
		this, SLOT(close())
	);
	
	updateProjectActions();
	updateBatchProcessingActions();
	updateWindowTitle();
	updateMainArea();
}


MainWindow::~MainWindow()
{
	cancelOngoingTask();
	m_ptrWorkerThread->shutdown();
	
	removeWidgetsFromLayout(m_pImageFrameLayout, false);
	removeWidgetsFromLayout(m_pOptionsFrameLayout, false);
	m_ptrTabbedDebugImages->clear();
}

void
MainWindow::cancelOngoingTask()
{
	if (m_ptrCurTask) {
		m_ptrCurTask->cancel();
		m_ptrCurTask.reset();
	}
}

void
MainWindow::switchToNewProject(
	IntrusivePtr<PageSequence> const& pages,
	QString const& out_dir, QString const& project_file_path,
	ProjectReader const* project_reader)
{
	m_ptrPages = pages;
	m_outDir = out_dir;
	m_projectFile = project_file_path;
	
	if (m_batchProcessing) {
		splitter->widget(0)->setVisible(true);
		m_batchProcessing = false;
	}
	
	cancelOngoingTask();
	
	// Recreate the filter list model, and let filters load
	m_ptrFilterListModel.reset(new FilterListModel(m_ptrPages));
	if (project_reader) {
		project_reader->readFilterSettings(
			m_ptrFilterListModel->filters()
		);
	}
	
	// Connect the filter list model to the view and select
	// the first item.
	{
		ScopedIncDec<int> guard(m_ignoreSelectionChanges);
		filterList->setModel(m_ptrFilterListModel.get());
		filterList->selectionModel()->select(
			m_ptrFilterListModel->index(0, 0),
			QItemSelectionModel::SelectCurrent
		);
		m_curFilter = 0;
		
		// Setting a data model also implicitly sets a new
		// selection model, so we have to reconnect to it.
		connect(
			filterList->selectionModel(),
			SIGNAL(selectionChanged(QItemSelection const&, QItemSelection const&)),
			this, SLOT(filterSelectionChanged(QItemSelection const&))
		);
	}
	
	m_ptrContentBoxPropagator.reset(
		new ContentBoxPropagator(
			m_ptrFilterListModel->getPageLayoutFilter(),
			m_ptrFilterListModel->createCompositeCacheDrivenTask(
				m_outDir,
				m_ptrFilterListModel->getSelectContentFilterIdx()
			)
		)
	);
	
	// Thumbnails are stored relative to the output directory,
	// so recreate the thumbnail cache.
	if (out_dir.isEmpty()) {
		m_ptrThumbnailCache.reset();
	} else {
		m_ptrThumbnailCache = createThumbnailCache();
	}
	resetThumbSequence();
	
	updateProjectActions();
	updateBatchProcessingActions();
	updateWindowTitle();
	updateMainArea();
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
MainWindow::showNewOpenProjectPanel()
{
	std::auto_ptr<QWidget> outer_widget(new QWidget);
	QGridLayout* layout = new QGridLayout(outer_widget.get());
	outer_widget->setLayout(layout);
	
	NewOpenProjectPanel* nop = new NewOpenProjectPanel(outer_widget.get());
	
	// We use asynchronous connections because otherwise we
	// would be deleting a widget from its event handler, which
	// Qt doesn't like.
	connect(
		nop, SIGNAL(newProject()),
		this, SLOT(newProject()),
		Qt::QueuedConnection
	);
	connect(
		nop, SIGNAL(openProject()),
		this, SLOT(openProject()),
		Qt::QueuedConnection
	);
	connect(
		nop, SIGNAL(openRecentProject(QString const&)),
		this, SLOT(openProject(QString const&)),
		Qt::QueuedConnection
	);
	
	layout->addWidget(nop, 1, 1);
	layout->setColumnStretch(0, 1);
	layout->setColumnStretch(2, 1);
	layout->setRowStretch(0, 1);
	layout->setRowStretch(2, 1);
	setImageWidget(outer_widget.release(), TRANSFER_OWNERSHIP);
}

void
MainWindow::setupThumbView()
{
	QSize const outer_before(thumbView->size());
	QSize const inner_before(thumbView->viewport()->size());
	
	QSize const inner_after(m_maxLogicalThumbSize.toSize());
	QSize const outer_after(
		inner_after.width() + outer_before.width() - inner_before.width(),
		inner_after.height() + outer_before.height() - inner_before.height()
	);
	
	thumbView->setMinimumSize(outer_after);
	
	thumbView->setBackgroundBrush(palette().color(QPalette::Window));
	m_ptrThumbSequence->attachView(thumbView);
}

void
MainWindow::closeEvent(QCloseEvent* const event)
{
	if (m_closing) {
		event->accept();
	} else {
		event->ignore();
		startTimer(0);
	}
}

void
MainWindow::timerEvent(QTimerEvent* const event)
{
	// We only use the timer event for delayed closing of the window.
	killTimer(event->timerId());
	
	if (closeProjectInteractive()) {
		m_closing = true;
		close();
	}
}

MainWindow::SavePromptResult
MainWindow::promptProjectSave()
{
	QMessageBox::StandardButton const res = QMessageBox::question(
		 this, tr("Save Project"), tr("Save this project?"),
		 QMessageBox::Save|QMessageBox::Discard|QMessageBox::Cancel,
		 QMessageBox::Save
	);
	
	switch (res) {
		case QMessageBox::Save:
			return SAVE;
		case QMessageBox::Discard:
			return DONT_SAVE;
		default:
			return CANCEL;
	}
}

bool
MainWindow::compareFiles(QString const& fpath1, QString const& fpath2)
{
	QFile file1(fpath1);
	QFile file2(fpath2);
	
	if (!file1.open(QIODevice::ReadOnly)) {
		return false;
	}
	if (!file2.open(QIODevice::ReadOnly)) {
		return false;
	}
	
	if (!file1.isSequential() && !file2.isSequential()) {
		if (file1.size() != file2.size()) {
			return false;
		}
	}
	
	int const chunk_size = 4096;
	for (;;) {
		QByteArray const chunk1(file1.read(chunk_size));
		QByteArray const chunk2(file2.read(chunk_size));
		if (chunk1.size() != chunk2.size()) {
			return false;
		} else if (chunk1.size() == 0) {
			return true;
		}
	}
}

void
MainWindow::resetThumbSequence()
{
	if (m_ptrThumbnailCache.get()) {
		IntrusivePtr<CompositeCacheDrivenTask> const task(
			m_ptrFilterListModel->createCompositeCacheDrivenTask(
				m_outDir, m_curFilter
			)
		);
		
		m_ptrThumbSequence->setThumbnailFactory(
			IntrusivePtr<ThumbnailFactory>(
				new ThumbnailFactory(
					*m_ptrThumbnailCache,
					m_maxLogicalThumbSize, task
				)
			)
		);
	}
	
	m_ptrThumbSequence->reset(m_ptrPages->snapshot(getCurrentView()));
	
	if (!m_ptrThumbnailCache.get()) {
		// Empty project.
		assert(m_ptrPages->numImages() == 0);
		m_ptrThumbSequence->setThumbnailFactory(
			IntrusivePtr<ThumbnailFactory>()
		);
	}
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
	
	// We use an asynchronous connection here, because the slot
	// will probably delete the options panel, which could be
	// responsible for the emission of this signal.  Qt doesn't
	// like when we delete an object while it's emitting a singal.
	connect(
		widget, SIGNAL(reloadRequested()),
		this, SLOT(reloadRequested()), Qt::QueuedConnection
	);
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
	QWidget* widget, Ownership const ownership,
	DebugImages* debug_images)
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
		BOOST_FOREACH (DebugImages::Item& item, debug_images->items()) {
			QWidget* widget = new BasicImageView(item.image());
			m_imageWidgetCleanup.add(widget);
			m_ptrTabbedDebugImages->addTab(widget, item.label());
			item.image() = QImage(); // Save memory.
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
		m_ptrPages->setNextPage(getCurrentView(), &page_num)
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
		m_ptrPages->setPrevPage(getCurrentView(), &page_num)
	);
	m_ptrThumbSequence->setCurrentThumbnail(prev_page.id());
	loadImage(prev_page, page_num);
}

void
MainWindow::goToPage(PageId const& page_id)
{
	m_ptrPages->setCurPage(page_id);
	m_ptrThumbSequence->setCurrentThumbnail(page_id);
	updateMainArea();
}

void
MainWindow::pageSelected(
	PageInfo const& page_info, QRectF const& thumb_rect,
	bool const by_user, bool const was_already_selected)
{
	if (by_user || focusButton->isChecked()) {
		thumbView->ensureVisible(thumb_rect, 0, 0);
	}
	
	if (by_user) {
		m_ptrPages->setCurPage(page_info.id());
		if (m_batchProcessing) {
			stopBatchProcessing();
		} else {
			updateMainArea();
		}
	}
}

void
MainWindow::thumbViewFocusToggled(bool const checked)
{
	QRectF const rect(m_ptrThumbSequence->currentItemSceneRect());
	if (rect.isNull()) {
		// No current item.
		return;
	}
	
	if (checked) {
		thumbView->ensureVisible(rect, 0, 0);
	}
}

void
MainWindow::thumbViewScrolled()
{
	QRectF const rect(m_ptrThumbSequence->currentItemSceneRect());
	if (rect.isNull()) {
		// No current item.
		return;
	}
	
	QRectF const viewport_rect(thumbView->viewport()->rect());
	QRectF const viewport_item_rect(
		thumbView->viewportTransform().mapRect(rect)
	);
	
	double const intersection_threshold = 0.5;
	if (viewport_item_rect.top() >= viewport_rect.top() &&
			viewport_item_rect.top() + viewport_item_rect.height()
			* intersection_threshold <= viewport_rect.bottom()) {
		// Item is visible.
	} else if (viewport_item_rect.bottom() <= viewport_rect.bottom() &&
			viewport_item_rect.bottom() - viewport_item_rect.height()
			* intersection_threshold >= viewport_rect.top()) {
		// Item is visible.
	} else {
		focusButton->setChecked(false);
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
	
	cancelOngoingTask();
	
	bool const was_below_select_content = isBelowSelectContent(m_curFilter);
	m_curFilter = selected.front().top();
	bool const now_below_select_content = isBelowSelectContent(m_curFilter);
	
	if (!was_below_select_content && now_below_select_content) {
		// IMPORTANT: this needs to go before resetting thumbnails,
		// because it may affect them.
		if (m_ptrContentBoxPropagator.get()) {
			m_ptrContentBoxPropagator->propagate(*m_ptrPages);
		} // Otherwise probably no project is loaded.
	}
	
	focusButton->setChecked(true); // Should go before resetThumbSequence().
	resetThumbSequence();
	
	updateBatchProcessingActions();
	updateMainArea();
}

void
MainWindow::reloadRequested()
{
	updateMainArea();
}

void
MainWindow::startBatchProcessing()
{
	// The current task is being cancelled because
	// it's probably not a batch processing task.
	cancelOngoingTask();
	
	m_batchProcessing = true;
	updateBatchProcessingActions();
	
	focusButton->setChecked(true);
	splitter->widget(0)->setVisible(false);
	
	PageInfo const page_info(m_ptrPages->setFirstPage(getCurrentView()));
	m_ptrThumbSequence->setCurrentThumbnail(page_info.id());
	updateMainArea();
}

void
MainWindow::stopBatchProcessing()
{
	if (!m_batchProcessing) {
		return;
	}
	
	// The current task is being cancelled because
	// it's probably a batch processing task.
	cancelOngoingTask();
	
	splitter->widget(0)->setVisible(true);
	
	m_batchProcessing = false;
	updateBatchProcessingActions();
	
	// The necessity to explicitly select a thumbnail comes from the fact
	// that during batch processing we select not the thumbnail that is
	// currently being processed, but the one that has been processed
	// before that.
	int page_num = 0;
	PageInfo const page_info(m_ptrPages->curPage(getCurrentView(), &page_num));
	m_ptrThumbSequence->setCurrentThumbnail(page_info.id());
	loadImage(page_info, page_num);
}

void
MainWindow::filterResult(BackgroundTaskPtr const& task, FilterResultPtr const& result)
{
	if (task != m_ptrCurTask || task->isCancelled()) {
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
	
	result->updateUI(this);
	
	if (m_batchProcessing) {
		// During batch processing we prefer to select the thumbnail
		// after it has been processed, not before.  Otherwise selected
		// thumbnail will always be an unprocessed one, with the
		// question mark on it.
		PageInfo const cur_page(m_ptrPages->curPage(getCurrentView()));
		m_ptrThumbSequence->setCurrentThumbnail(cur_page.id());
		
		int page_num = 0;
		PageInfo const next_page(
			m_ptrPages->setNextPage(getCurrentView(), &page_num)
		);
		
		if (next_page.id() == cur_page.id()) {
			m_ptrPages->setFirstPage(getCurrentView());
			stopBatchProcessing(); // This will call loadImage().
		} else {
			loadImage(next_page, page_num);
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
		updateWindowTitle();
	}
}

void
MainWindow::saveProjectAsTriggered()
{
	QString project_dir;
	if (!m_projectFile.isEmpty()) {
		project_dir = QFileInfo(m_projectFile).absolutePath();
	} else {
		QSettings settings;
		project_dir = settings.value("project/lastDir").toString();
	}
	
	QString project_file(
		QFileDialog::getSaveFileName(
			this, QString(), project_dir,
			tr("Scan Tailor Projects")+" (*.ScanTailor)"
		)
	);
	if (project_file.isEmpty()) {
		return;
	}
	
	if (!project_file.endsWith(".ScanTailor", Qt::CaseInsensitive)) {
		project_file += ".ScanTailor";
	}
	
	if (saveProjectWithFeedback(project_file)) {
		m_projectFile = project_file;
		updateWindowTitle();
		
		QSettings settings;
		settings.setValue(
			"project/lastDir",
			QFileInfo(m_projectFile).absolutePath()
		);
		
		RecentProjects rp;
		rp.read();
		rp.setMostRecent(m_projectFile);
		rp.write();
	}
}

void
MainWindow::newProject()
{
	if (!closeProjectInteractive()) {
		return;
	}
	
	// It will delete itself when it's done.
	ProjectCreationContext* context = new ProjectCreationContext(this);
	connect(
		context, SIGNAL(done(ProjectCreationContext*)),
		this, SLOT(newProjectCreated(ProjectCreationContext*))
	);
}

void
MainWindow::newProjectCreated(ProjectCreationContext* context)
{
	IntrusivePtr<PageSequence> pages(
		new PageSequence(context->files(), PageSequence::AUTO_PAGES)
	);
	switchToNewProject(pages, context->outDir());
}

void
MainWindow::openProject()
{
	if (!closeProjectInteractive()) {
		return;
	}
	
	QSettings settings;
	QString const project_dir(settings.value("project/lastDir").toString());
	
	QString const project_file(
		QFileDialog::getOpenFileName(
			0, tr("Open Project"),
			project_dir,
			tr("Scan Tailor Projects")+" (*.ScanTailor)"
		)
	);
	if (project_file.isEmpty()) {
		// Cancelled by user.
		return;
	}
	
	openProject(project_file);
}

void
MainWindow::openProject(QString const& project_file)
{
	QFile file(project_file);
	if (!file.open(QIODevice::ReadOnly)) {
		QMessageBox::warning(
			0, tr("Error"),
			tr("Unable to open the project file.")
		);
		return;
	}
	
	QDomDocument doc;
	if (!doc.setContent(&file)) {
		QMessageBox::warning(
			0, tr("Error"),
			tr("The project file is broken.")
		);
		return;
	}
	
	file.close();
	
	ProjectReader reader(doc);
	if (!reader.success()) {
		QMessageBox::warning(
			0, tr("Error"),
			tr("Unable to interpret the project file.")
		);
		return;
	}
	
	RecentProjects rp;
	rp.read();
	rp.setMostRecent(project_file);
	rp.write();
	
	QSettings settings;
	settings.setValue(
		"project/lastDir",
		QFileInfo(project_file).absolutePath()
	);
	
	switchToNewProject(
		reader.pages(), reader.outputDirectory(),
		project_file, &reader
	);
}

void
MainWindow::closeProject()
{
	closeProjectInteractive();
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
MainWindow::updateProjectActions()
{
	bool const loaded = isProjectLoaded();
	actionSaveProject->setEnabled(loaded);
	actionSaveProjectAs->setEnabled(loaded);
}

void
MainWindow::updateBatchProcessingActions()
{
	bool const ok = m_ptrPages->numImages() != 0 &&
		(!isOutputFilter() ||
		m_ptrFilterListModel->getPageLayoutFilter()
		->checkReadyForOutput(*m_ptrPages));
	
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

bool
MainWindow::isOutputFilter() const
{
	return isOutputFilter(m_curFilter);
}

bool
MainWindow::isOutputFilter(int const filter_idx) const
{
	return (filter_idx == m_ptrFilterListModel->getOutputFilterIdx());
}

PageSequence::View
MainWindow::getCurrentView() const
{
	return m_ptrFilterListModel->getFilter(m_curFilter)->getView();
}

void
MainWindow::updateMainArea()
{
	if (m_ptrPages->numImages() == 0) {
		showNewOpenProjectPanel();
	} else {
		int page_num = 0;
		PageInfo const page_info(
			m_ptrPages->curPage(getCurrentView(), &page_num)
		);
		loadImage(page_info, page_num);
	}
}

void
MainWindow::loadImage(PageInfo const& page, int const page_num)
{
	removeWidgetsFromLayout(m_pImageFrameLayout, false);
	m_ptrTabbedDebugImages->clear();
	m_imageWidgetCleanup.clear();
	
	if (isOutputFilter() &&
			!m_ptrFilterListModel->getPageLayoutFilter()
			->checkReadyForOutput(*m_ptrPages, &page.id())) {
		QString const err_text(
			tr("Output is not yet possible, as the final size"
			" of pages is not yet known.\nTo determine it,"
			" run batch processing at \"Select Content\" or"
			" \"Page Layout\".")
		);
		setOptionsWidget(new FilterOptionsWidget, TRANSFER_OWNERSHIP);
		setImageWidget(new ErrorWidget(err_text), TRANSFER_OWNERSHIP);
		return;
	}
	
	m_ptrFilterListModel->getFilter(m_curFilter)->preUpdateUI(this, page.id());
	
	cancelOngoingTask();
	
	assert(m_ptrThumbnailCache.get());
	m_ptrCurTask = m_ptrFilterListModel->createCompositeTask(
		page, page_num, m_outDir, m_curFilter,
		*m_ptrThumbnailCache, m_ptrPages,
		m_batchProcessing, m_debug
	);
	if (m_ptrCurTask) {
		m_ptrWorkerThread->performTask(m_ptrCurTask);
	}
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
	QString const version(QString::fromUtf8(VERSION));
	setWindowTitle(tr("%1 - Scan Tailor %2").arg(project_name, version));
}

/**
 * \brief Closes the currently project, prompting to save it if necessary.
 *
 * \return true if the project was closed, false if the user cancelled the process.
 */
bool
MainWindow::closeProjectInteractive()
{
	if (!isProjectLoaded()) {
		return true;
	}
	
	if (m_projectFile.isEmpty()) {
		switch (promptProjectSave()) {
			case SAVE:
				saveProjectTriggered();
				// fall through
			case DONT_SAVE:
				break;
			case CANCEL:
				return false;
		}
		closeProjectWithoutSaving();
		return true;
	}
	
	QFileInfo const project_file(m_projectFile);
	QFileInfo const backup_file(
		project_file.absoluteDir(),
		QString::fromAscii("Backup.")+project_file.fileName()
	);
	QString const backup_file_path(backup_file.absoluteFilePath());
	
	ProjectWriter writer(m_outDir, m_ptrPages);
	
	if (!writer.write(backup_file_path, m_ptrFilterListModel->filters())) {
		// Backup file could not be written???
		QFile::remove(backup_file_path);
		switch (promptProjectSave()) {
			case SAVE:
				saveProjectTriggered();
				// fall through
			case DONT_SAVE:
				break;
			case CANCEL:
				return false;
		}
		closeProjectWithoutSaving();
		return true;
	}
	
	if (compareFiles(m_projectFile, backup_file_path)) {
		// The project hasn't really changed.
		QFile::remove(backup_file_path);
		closeProjectWithoutSaving();
		return true;
	}
	
	switch (promptProjectSave()) {
		case SAVE:
			if (!Utils::overwritingRename(
					backup_file_path, m_projectFile)) {
				QMessageBox::warning(
					this, tr("Error"),
					tr("Error saving the project file!")
				);
				return false;
			}
			// fall through
		case DONT_SAVE:
			QFile::remove(backup_file_path);
			break;
		case CANCEL:
			return false;
	}
	
	closeProjectWithoutSaving();
	return true;
}
	
void
MainWindow::closeProjectWithoutSaving()
{
	IntrusivePtr<PageSequence> pages(new PageSequence());
	switchToNewProject(pages, QString());
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
	
	m_outputFilterIdx = m_filters.size();
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
	IntrusivePtr<PageSequence> const& page_sequence,
	bool const batch_processing, bool debug)
{
	IntrusivePtr<fix_orientation::Task> fix_orientation_task;
	IntrusivePtr<page_split::Task> page_split_task;
	IntrusivePtr<deskew::Task> deskew_task;
	IntrusivePtr<select_content::Task> select_content_task;
	IntrusivePtr<page_layout::Task> page_layout_task;
	IntrusivePtr<output::Task> output_task;
	
	if (batch_processing) {
		debug = false;
	}
	
	switch (last_filter_idx) {
	case 5:
		output_task = m_ptrOutputFilter->createTask(
			page.id(), page_num, out_dir,
			thumbnail_cache, batch_processing, debug
		);
		debug = false;
	case 4:
		page_layout_task = m_ptrPageLayoutFilter->createTask(
			page.id(), output_task, batch_processing, debug
		);
		debug = false;
	case 3:
		select_content_task = m_ptrSelectContentFilter->createTask(
			page.id(), page_layout_task, batch_processing, debug
		);
		debug = false;
	case 2:
		deskew_task = m_ptrDeskewFilter->createTask(
			page.id(), select_content_task, batch_processing, debug
		);
		debug = false;
	case 1:
		page_split_task = m_ptrPageSplitFilter->createTask(
			page.id(), deskew_task, batch_processing, debug
		);
		debug = false;
	case 0:
		fix_orientation_task = m_ptrFixOrientationFilter->createTask(
			page.id(), page_split_task, batch_processing
		);
		debug = false;
	}
	assert(fix_orientation_task);
	
	return BackgroundTaskPtr(
		new LoadFileTask(
			page, thumbnail_cache,
			page_sequence, fix_orientation_task
		)
	);
}

IntrusivePtr<CompositeCacheDrivenTask>
MainWindow::FilterListModel::createCompositeCacheDrivenTask(
	QString const& out_dir, int const last_filter_idx)
{
	IntrusivePtr<fix_orientation::CacheDrivenTask> fix_orientation_task;
	IntrusivePtr<page_split::CacheDrivenTask> page_split_task;
	IntrusivePtr<deskew::CacheDrivenTask> deskew_task;
	IntrusivePtr<select_content::CacheDrivenTask> select_content_task;
	IntrusivePtr<page_layout::CacheDrivenTask> page_layout_task;
	IntrusivePtr<output::CacheDrivenTask> output_task;
	
	switch (last_filter_idx) {
	case 5:
		output_task = m_ptrOutputFilter->createCacheDrivenTask(out_dir);
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
