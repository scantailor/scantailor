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

#include "version.h"
#include "MainWindow.h.moc"
#include "NewOpenProjectPanel.h"
#include "RecentProjects.h"
#include "WorkerThread.h"
#include "PageSequence.h"
#include "StageSequence.h"
#include "ThumbnailSequence.h"
#include "ImageInfo.h"
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
#include "SkinnedButton.h"
#include "ProcessingIndicationWidget.h"
#include "ImageMetadataLoader.h"
#include "OrthogonalRotation.h"
#include "FixDpiSinglePageDialog.h"
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
#include "ui_RemoveFileDialog.h"
#include <boost/foreach.hpp>
#include <boost/lambda/lambda.hpp>
#include <boost/lambda/bind.hpp>
#include <QLineF>
#include <QWidget>
#include <QDialog>
#include <QCloseEvent>
#include <QStackedLayout>
#include <QGridLayout>
#include <QLayoutItem>
#include <QScrollBar>
#include <QPushButton>
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
#include <QSortFilterProxyModel>
#include <QFileSystemModel>
#include <QFileInfo>
#include <QDebug>
#include <algorithm>
#include <stddef.h>
#include <math.h>
#include <assert.h>

MainWindow::MainWindow()
:	m_ptrPages(new PageSequence),
	m_ptrStages(new StageSequence(m_ptrPages)),
	m_ptrWorkerThread(new WorkerThread),
	m_curFilter(0),
	m_ignoreSelectionChanges(0),
	m_debug(false),
	m_batchProcessing(false),
	m_closing(false)
{
	m_maxLogicalThumbSize = QSize(250, 160);
	m_ptrThumbSequence.reset(new ThumbnailSequence(m_maxLogicalThumbSize));
	
	setupUi(this);
	m_ptrBatchProcessingWidget = createBatchProcessingWidget();
	m_ptrProcessingIndicationWidget.reset(new ProcessingIndicationWidget);
	
	filterList->setStages(m_ptrStages);
	filterList->selectRow(0);
	
	setupThumbView(); // Expects m_ptrThumbSequence to be initialized.
	
	m_ptrTabbedDebugImages.reset(new QTabWidget);
	
	m_debug = actionDebug->isChecked();
	m_pImageFrameLayout = new QStackedLayout(imageViewFrame);
	m_pOptionsFrameLayout = new QStackedLayout(filterOptions);
	
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
		filterList, SIGNAL(launchBatchProcessing()),
		this, SLOT(startBatchProcessing())
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
		m_ptrThumbSequence.get(),
		SIGNAL(contextMenuRequested(PageInfo const&, QPoint const&, bool)),
		this, SLOT(contextMenuRequested(PageInfo const&, QPoint const&, bool))
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
	updateWindowTitle();
	updateMainArea();

	QSettings settings;
	if (settings.value("mainWindow/maximized") == false) {
		QVariant const geom(
			settings.value("mainWindow/nonMaximizedGeometry")
		);
		if (!restoreGeometry(geom.toByteArray())) {
			resize(1014, 689); // A sensible value.
		}
	}
}


MainWindow::~MainWindow()
{
	cancelOngoingTask();
	m_ptrWorkerThread->shutdown();
	
	removeWidgetsFromLayout(m_pImageFrameLayout);
	removeWidgetsFromLayout(m_pOptionsFrameLayout);
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
	stopBatchProcessing(CLEAR_MAIN_AREA);
	
	cancelOngoingTask();
	
	m_ptrPages = pages;
	m_outDir = out_dir;
	m_projectFile = project_file_path;
	
	// Recreate the stages and load their state.
	m_ptrStages.reset(new StageSequence(pages));
	if (project_reader) {
		project_reader->readFilterSettings(
			m_ptrStages->filters()
		);
	}
	
	// Connect the filter list model to the view and select
	// the first item.
	{
		ScopedIncDec<int> guard(m_ignoreSelectionChanges);
		filterList->setStages(m_ptrStages);
		filterList->selectRow(0);
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
			m_ptrStages->pageLayoutFilter(),
			createCompositeCacheDrivenTask(
				m_ptrStages->selectContentFilterIdx()
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
	
	removeFilterOptionsWidget();
	updateProjectActions();
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
	
	filterList->setBatchProcessingPossible(false);
}

std::auto_ptr<QWidget>
MainWindow::createBatchProcessingWidget()
{
	std::auto_ptr<QWidget> outer_widget(new QWidget);
	QGridLayout* layout = new QGridLayout(outer_widget.get());
	outer_widget->setLayout(layout);
	
	SkinnedButton* btn = new SkinnedButton(
		":/icons/stop-big.png",
		":/icons/stop-big-hovered.png",
		":/icons/stop-big-pressed.png",
		outer_widget.get()
	);
	btn->setStatusTip(tr("Stop batch processing"));
	
	layout->addWidget(btn, 1, 1);
	layout->setColumnStretch(0, 1);
	layout->setColumnStretch(2, 1);
	layout->setRowStretch(0, 1);
	layout->setRowStretch(2, 1);
	
	connect(
		btn, SIGNAL(clicked()),
		this, SLOT(stopBatchProcessing())
	);
	
	return outer_widget;
}

void
MainWindow::setupThumbView()
{
	int const sb = thumbView->style()->pixelMetric(QStyle::PM_ScrollBarExtent);
	int inner_width = thumbView->maximumViewportSize().width() - sb;
	if (thumbView->style()->styleHint(QStyle::SH_ScrollView_FrameOnlyAroundContents, 0, thumbView)) {
		inner_width -= thumbView->frameWidth() * 2;
	}
	int const delta_x = thumbView->size().width() - inner_width;
	thumbView->setMinimumWidth((int)ceil(m_maxLogicalThumbSize.width() + delta_x));
	
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
		QSettings settings;
		settings.setValue("mainWindow/maximized", isMaximized());
		if (!isMaximized()) {
			settings.setValue(
				"mainWindow/nonMaximizedGeometry", saveGeometry()
			);
		}
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
			createCompositeCacheDrivenTask(m_curFilter)
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
	if (m_batchProcessing) {
		if (ownership == TRANSFER_OWNERSHIP) {
			delete widget;
		}
		return;
	}
	
	if (m_ptrOptionsWidget != widget) {
		removeWidgetsFromLayout(m_pOptionsFrameLayout);
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
	if (m_batchProcessing) {
		if (ownership == TRANSFER_OWNERSHIP) {
			delete widget;
		}
		return;
	}
	
	removeImageWidget();
	
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
MainWindow::removeImageWidget()
{
	removeWidgetsFromLayout(m_pImageFrameLayout);
	
	m_ptrTabbedDebugImages->clear();
	
	// Delete the old widget we were owning, if any.
	m_imageWidgetCleanup.clear();
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
	if (m_batchProcessing || !isProjectLoaded()) {
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
	if (m_batchProcessing || !isProjectLoaded()) {
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
	focusButton->setChecked(true);
	m_ptrPages->setCurPage(page_id);
	
	// This will result in pageSelected() being called
	// with by_user == false.
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
MainWindow::contextMenuRequested(
	PageInfo const& page_info_, QPoint const& screen_pos, bool selected)
{
	// Make a copy to prevent it from being invalidated.
	PageInfo const page_info(page_info_);
	
	if (!selected) {
		goToPage(page_info.id());
	}
	
	if (getCurrentView() != PageSequence::IMAGE_VIEW) {
		// We don't support adding / removing of pages - only of images.
		return;
	}
	
	QMenu menu;
	QAction* ins_before = menu.addAction(
		QIcon(":/icons/insert-before-16.png"), tr("Insert before ...")
	);
	QAction* ins_after = menu.addAction(
		QIcon(":/icons/insert-after-16.png"), tr("Insert after ...")
	);
	menu.addSeparator();
	QAction* remove = menu.addAction(
		QIcon(":/icons/user-trash.png"), tr("Remove from project ...")
	);
	
	QAction* action = menu.exec(screen_pos);
	if (action == ins_before) {
		showInsertFileDialog(BEFORE, page_info.imageId());
	} else if (action == ins_after) {
		showInsertFileDialog(AFTER, page_info.imageId());
	} else if (action == remove) {
		showRemoveFileDialog(page_info);
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
	if (m_batchProcessing || !isProjectLoaded()) {
		return;
	}
	
	// Must be done before setting m_batchProcessing to true.
	setImageWidget(m_ptrBatchProcessingWidget.get(), KEEP_OWNERSHIP);
	
	m_batchProcessing = true;
	
	focusButton->setChecked(true);
	
	removeFilterOptionsWidget();
	filterList->setBatchProcessingInProgress(true);
	filterList->setEnabled(false);
	
	// In batch processing mode we highlight the page that
	// was just processed, while processing the next one.
	// Keep in mind that next one will have a question mark
	// on it until it's fully processed.
	
	if (!m_ptrCurTask) {
		// If the task for the current page has already finished, then the
		// current page is already processed, so move to the next one.
		m_ptrPages->setNextPage(getCurrentView());
	} else {
		cancelOngoingTask();
	}
	
	updateMainArea();
}

void
MainWindow::stopBatchProcessing(MainAreaAction main_area)
{
	if (!m_batchProcessing) {
		return;
	}
	
	// The current task is being cancelled because
	// it's probably a batch processing task.
	cancelOngoingTask();
	
	m_batchProcessing = false;
	filterList->setBatchProcessingInProgress(false);
	filterList->setEnabled(true);
	
	// The necessity to explicitly select a thumbnail comes from the fact
	// that during batch processing we select not the thumbnail that is
	// currently being processed, but the one that has been processed
	// before that.
	int page_num = 0;
	PageInfo const page(m_ptrPages->curPage(getCurrentView(), &page_num));
	m_ptrThumbSequence->setCurrentThumbnail(page.id());
	
	switch (main_area) {
		case LOAD_IMAGE:
			loadImage(page, page_num);
			break;
		case CLEAR_MAIN_AREA:
			removeImageWidget();
			break;
	}
}

void
MainWindow::filterResult(BackgroundTaskPtr const& task, FilterResultPtr const& result)
{
	if (task != m_ptrCurTask || task->isCancelled()) {
		return;
	}
	
	m_ptrCurTask.reset(); // We check if a task is in progress in startBatchProcessing().
	
	if (!m_batchProcessing) {
		if (!result->filter()) {
			// Error loading file.  No special action is necessary.
		} else if (result->filter() != m_ptrStages->filterAt(m_curFilter)) {
			// Error from one of the previous filters.
			int const idx = m_ptrStages->findFilter(result->filter());
			assert(idx >= 0);
			m_curFilter = idx;
			
			ScopedIncDec<int> selection_guard(m_ignoreSelectionChanges);
			filterList->selectRow(idx);
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
		new PageSequence(
			context->files(), PageSequence::AUTO_PAGES,
			context->layoutDirection()
		)
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

/**
 * Note: the removed widgets are not deleted.
 */
void
MainWindow::removeWidgetsFromLayout(QLayout* layout)
{
	QLayoutItem *child;
	while ((child = layout->takeAt(0))) {
		delete child;
	}
}

void
MainWindow::removeFilterOptionsWidget()
{
	removeWidgetsFromLayout(m_pOptionsFrameLayout);
	
	// Delete the old widget we were owning, if any.
	m_optionsWidgetCleanup.clear();
	
	m_ptrOptionsWidget = 0;
}

void
MainWindow::updateProjectActions()
{
	bool const loaded = isProjectLoaded();
	actionSaveProject->setEnabled(loaded);
	actionSaveProjectAs->setEnabled(loaded);
}

bool
MainWindow::isBelowSelectContent() const
{
	return isBelowSelectContent(m_curFilter);
}

bool
MainWindow::isBelowSelectContent(int const filter_idx) const
{
	return (filter_idx > m_ptrStages->selectContentFilterIdx());
}

bool
MainWindow::isOutputFilter() const
{
	return isOutputFilter(m_curFilter);
}

bool
MainWindow::isOutputFilter(int const filter_idx) const
{
	return (filter_idx == m_ptrStages->outputFilterIdx());
}

PageSequence::View
MainWindow::getCurrentView() const
{
	return m_ptrStages->filterAt(m_curFilter)->getView();
}

void
MainWindow::updateMainArea()
{
	filterList->setBatchProcessingPossible(true);
	// Both showNewOpenProjectPanel() and loadImage()
	// can set it to false though.
	
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

bool
MainWindow::checkReadyForOutput(PageId const* ignore) const
{
	return m_ptrStages->pageLayoutFilter()->checkReadyForOutput(
		*m_ptrPages, ignore
	);
}

void
MainWindow::loadImage(PageInfo const& page, int const page_num)
{
	cancelOngoingTask();
	
	if (isOutputFilter() && !checkReadyForOutput(&page.id())) {
		filterList->setBatchProcessingPossible(false);
		
		// Switch to the first page - the user will need
		// to process all pages in batch mode.
		PageInfo const first_page(
			m_ptrPages->setFirstPage(getCurrentView())
		);
		m_ptrThumbSequence->setCurrentThumbnail(first_page.id());
		
		QString const err_text(
			tr("Output is not yet possible, as the final size"
			" of pages is not yet known.\nTo determine it,"
			" run batch processing at \"Select Content\" or"
			" \"Page Layout\".")
		);
		
		removeFilterOptionsWidget();
		setImageWidget(new ErrorWidget(err_text), TRANSFER_OWNERSHIP);
		return;
	}
	
	if (!m_batchProcessing) {
		if (m_pImageFrameLayout->indexOf(m_ptrProcessingIndicationWidget.get()) != -1) {
			m_ptrProcessingIndicationWidget->processingRestartedEffect();
		}
		setImageWidget(m_ptrProcessingIndicationWidget.get(), KEEP_OWNERSHIP);
		m_ptrStages->filterAt(m_curFilter)->preUpdateUI(this, page.id());
	}
	
	assert(m_ptrThumbnailCache.get());
	m_ptrCurTask = createCompositeTask(page, page_num, m_curFilter);
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
	
	if (!writer.write(backup_file_path, m_ptrStages->filters())) {
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
	
	if (!writer.write(project_file, m_ptrStages->filters())) {
		QMessageBox::warning(
			this, tr("Error"),
			tr("Error saving the project file!")
		);
		return false;
	}
	
	return true;
}

void
MainWindow::showInsertFileDialog(BeforeOrAfter before_or_after, ImageId const& existing)
{
	// We need to filter out files already in project.
	class ProxyModel : public QSortFilterProxyModel
	{
	public:
		ProxyModel(PageSequence const& pages) {
			setDynamicSortFilter(true);
			
			PageSequenceSnapshot const snapshot(pages.snapshot(PageSequence::IMAGE_VIEW));
			unsigned const count = snapshot.numPages();
			for (unsigned i = 0; i < count; ++i) {
				PageInfo const& page = snapshot.pageAt(i);
				m_inProjectFiles.push_back(QFileInfo(page.imageId().filePath()));
			}
		}
	protected:
		virtual bool filterAcceptsRow(int source_row, QModelIndex const& source_parent) const {
			QModelIndex const idx(source_parent.child(source_row, 0));
			QVariant const data(idx.data(QFileSystemModel::FilePathRole));
			if (data.isNull()) {
				return true;
			}
			return !m_inProjectFiles.contains(QFileInfo(data.toString()));
		}
		
		virtual bool lessThan(QModelIndex const& left, QModelIndex const& right) const {
			return left.row() < right.row();
		}
	private:
		QFileInfoList m_inProjectFiles;
	};
	
	std::auto_ptr<QFileDialog> dialog(
		new QFileDialog(
			this, tr("File to insert"),
			QFileInfo(existing.filePath()).absolutePath()
		)
	);
	dialog->setFileMode(QFileDialog::ExistingFile);
	dialog->setProxyModel(new ProxyModel(*m_ptrPages));
	dialog->setNameFilter(tr("Images not in project (%1)").arg("*.png *.tiff *.tif *.jpeg *.jpg"));
	
	dialog->exec();
	
	QStringList const files(dialog->selectedFiles());
	if (files.size() != 1) {
		assert(files.empty());
		return;
	}
	
	using namespace boost::lambda;
	
	QString const file(files.front());
	ImageId const image_id(file, 0);
	
	std::vector<ImageMetadata> metadata_list;
	ImageMetadataLoader::Status const status = ImageMetadataLoader::load(
		file, bind(&std::vector<ImageMetadata>::push_back, var(metadata_list), _1)
	);
	if (status != ImageMetadataLoader::LOADED) {
		QMessageBox::warning(
			0, tr("Error"),
			tr("Error opening the image file.")
		);
		return;
	}
	
	bool const is_multipage_file = metadata_list.size() > 1;
	ImageMetadata& metadata = metadata_list.front();
	
	if (metadata.dpi().isNull()) {
		std::auto_ptr<FixDpiSinglePageDialog> dpi_dialog(
			new FixDpiSinglePageDialog(
				image_id, metadata.dpi(), is_multipage_file, this
			)
		);
		if (dpi_dialog->exec() != QDialog::Accepted) {
			return;
		}
		metadata.setDpi(dpi_dialog->dpi());
	}
	
	// This has to be done after metadata.setDpi() call above.
	int const num_sub_pages = PageSequence::adviseNumberOfLogicalPages(
		metadata, OrthogonalRotation()
	);
	ImageInfo const image_info(
		image_id, metadata, is_multipage_file, num_sub_pages
	);
	insertImage(image_info, before_or_after, existing);
}

void
MainWindow::showRemoveFileDialog(PageInfo const& page_info)
{
	QString const file_path(page_info.imageId().filePath());
	QString const fname(QFileInfo(file_path).fileName());
	
	std::auto_ptr<QDialog> dialog(new QDialog(this));
	Ui::RemoveFileDialog ui;
	ui.setupUi(dialog.get());
	
	QString image_name(fname);
	if (page_info.isMultiPageFile()) {
		image_name = tr("%1 (page %2)").arg(fname).arg(page_info.imageId().page() + 1);
	}
	ui.text->setText(ui.text->text().arg(image_name));
	
	ui.deleteFromDisk->setEnabled(!page_info.isMultiPageFile());
	
	QPushButton* remove_btn = ui.buttonBox->button(QDialogButtonBox::Ok);
	remove_btn->setText(tr("Remove"));
	
	dialog->setWindowModality(Qt::WindowModal);
	if (dialog->exec() != QDialog::Accepted) {
		return;
	}
	
	removeFromProject(page_info.imageId());
	if (ui.deleteFromDisk->isChecked()) {
		if (!QFile::remove(file_path)) {
			QMessageBox::warning(
				0, tr("Error"),
				tr("Unable to delete file:\n%1").arg(file_path)
			);
		}
	}
}

void
MainWindow::insertImage(ImageInfo const& new_image,
	BeforeOrAfter before_or_after, ImageId const& existing)
{
	assert(getCurrentView() == PageSequence::IMAGE_VIEW);

	m_ptrPages->insertImage(new_image, before_or_after, existing);
	PageInfo const page_info(
		PageId(new_image.id(), PageId::SINGLE_PAGE), new_image.metadata(),
		new_image.isMultiPageFile(), new_image.numSubPages()
	);
	m_ptrThumbSequence->insert(
		page_info, before_or_after, PageId(existing, PageId::SINGLE_PAGE)
	);
}

void
MainWindow::removeFromProject(ImageId image_id)
{
	// Note that image_id is passed by value intentionally.
	// We need a local copy, because a reference might come
	// directly from m_ptrPages, and so will be invalidated
	// by m_ptrPages->removeImage().
	
	m_ptrPages->removeImage(image_id);
	m_ptrThumbSequence->remove(image_id);
	m_ptrThumbSequence->setCurrentThumbnail(
		m_ptrPages->curPage(getCurrentView()).id()
	);
	updateMainArea();
}

BackgroundTaskPtr
MainWindow::createCompositeTask(
	PageInfo const& page, int const page_num, int const last_filter_idx)
{
	IntrusivePtr<fix_orientation::Task> fix_orientation_task;
	IntrusivePtr<page_split::Task> page_split_task;
	IntrusivePtr<deskew::Task> deskew_task;
	IntrusivePtr<select_content::Task> select_content_task;
	IntrusivePtr<page_layout::Task> page_layout_task;
	IntrusivePtr<output::Task> output_task;
	
	bool debug = m_debug;
	if (m_batchProcessing) {
		debug = false;
	}
	
	if (last_filter_idx >= m_ptrStages->outputFilterIdx()) {
		output_task = m_ptrStages->outputFilter()->createTask(
			page.id(), page_num, m_outDir,
			*m_ptrThumbnailCache, m_batchProcessing, debug
		);
		debug = false;
	}
	if (last_filter_idx >= m_ptrStages->pageLayoutFilterIdx()) {
		page_layout_task = m_ptrStages->pageLayoutFilter()->createTask(
			page.id(), output_task, m_batchProcessing, debug
		);
		debug = false;
	}
	if (last_filter_idx >= m_ptrStages->selectContentFilterIdx()) {
		select_content_task = m_ptrStages->selectContentFilter()->createTask(
			page.id(), page_layout_task, m_batchProcessing, debug
		);
		debug = false;
	}
	if (last_filter_idx >= m_ptrStages->deskewFilterIdx()) {
		deskew_task = m_ptrStages->deskewFilter()->createTask(
			page.id(), select_content_task, m_batchProcessing, debug
		);
		debug = false;
	}
	if (last_filter_idx >= m_ptrStages->pageSplitFilterIdx()) {
		page_split_task = m_ptrStages->pageSplitFilter()->createTask(
			page.id(), deskew_task, m_batchProcessing, debug
		);
		debug = false;
	}
	if (last_filter_idx >= m_ptrStages->fixOrientationFilterIdx()) {
		fix_orientation_task = m_ptrStages->fixOrientationFilter()->createTask(
			page.id(), page_split_task, m_batchProcessing
		);
		debug = false;
	}
	assert(fix_orientation_task);
	
	return BackgroundTaskPtr(
		new LoadFileTask(
			page, *m_ptrThumbnailCache,
			m_ptrPages, fix_orientation_task
		)
	);
}

IntrusivePtr<CompositeCacheDrivenTask>
MainWindow::createCompositeCacheDrivenTask(int const last_filter_idx)
{
	IntrusivePtr<fix_orientation::CacheDrivenTask> fix_orientation_task;
	IntrusivePtr<page_split::CacheDrivenTask> page_split_task;
	IntrusivePtr<deskew::CacheDrivenTask> deskew_task;
	IntrusivePtr<select_content::CacheDrivenTask> select_content_task;
	IntrusivePtr<page_layout::CacheDrivenTask> page_layout_task;
	IntrusivePtr<output::CacheDrivenTask> output_task;
	
	if (last_filter_idx >= m_ptrStages->outputFilterIdx()) {
		output_task = m_ptrStages->outputFilter()
				->createCacheDrivenTask(m_outDir);
	}
	if (last_filter_idx >= m_ptrStages->pageLayoutFilterIdx()) {
		page_layout_task = m_ptrStages->pageLayoutFilter()
				->createCacheDrivenTask(output_task);
	}
	if (last_filter_idx >= m_ptrStages->selectContentFilterIdx()) {
		select_content_task = m_ptrStages->selectContentFilter()
				->createCacheDrivenTask(page_layout_task);
	}
	if (last_filter_idx >= m_ptrStages->deskewFilterIdx()) {
		deskew_task = m_ptrStages->deskewFilter()
				->createCacheDrivenTask(select_content_task);
	}
	if (last_filter_idx >= m_ptrStages->pageSplitFilterIdx()) {
		page_split_task = m_ptrStages->pageSplitFilter()
				->createCacheDrivenTask(deskew_task);
	}
	if (last_filter_idx >= m_ptrStages->fixOrientationFilterIdx()) {
		fix_orientation_task = m_ptrStages->fixOrientationFilter()
				->createCacheDrivenTask(page_split_task);
	}
	
	assert(fix_orientation_task);
	
	return fix_orientation_task;
}
