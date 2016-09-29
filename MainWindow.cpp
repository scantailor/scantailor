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

#include "MainWindow.h"
#include "MainWindow.h.moc"
#include "NewOpenProjectPanel.h"
#include "RecentProjects.h"
#include "WorkerThread.h"
#include "ProjectPages.h"
#include "PageSequence.h"
#include "PageSelectionAccessor.h"
#include "PageSelectionProvider.h"
#include "StageSequence.h"
#include "ThumbnailSequence.h"
#include "PageOrderOption.h"
#include "PageOrderProvider.h"
#include "ProcessingTaskQueue.h"
#include "FileNameDisambiguator.h"
#include "OutputFileNameGenerator.h"
#include "ImageInfo.h"
#include "PageInfo.h"
#include "ImageId.h"
#include "Utils.h"
#include "FilterOptionsWidget.h"
#include "ErrorWidget.h"
#include "AutoRemovingFile.h"
#include "DebugImages.h"
#include "DebugImageView.h"
#include "TabbedDebugImages.h"
#include "BasicImageView.h"
#include "ProjectWriter.h"
#include "ProjectReader.h"
#include "ThumbnailPixmapCache.h"
#include "ThumbnailFactory.h"
#include "ContentBoxPropagator.h"
#include "PageOrientationPropagator.h"
#include "ProjectCreationContext.h"
#include "ProjectOpeningContext.h"
#include "SkinnedButton.h"
#include "SystemLoadWidget.h"
#include "ProcessingIndicationWidget.h"
#include "ImageMetadataLoader.h"
#include "SmartFilenameOrdering.h"
#include "OrthogonalRotation.h"
#include "FixDpiDialog.h"
#include "LoadFilesStatusDialog.h"
#include "SettingsDialog.h"
#include "AbstractRelinker.h"
#include "RelinkingDialog.h"
#include "OutOfMemoryHandler.h"
#include "OutOfMemoryDialog.h"
#include "QtSignalForwarder.h"
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
#include "ui_AboutDialog.h"
#include "ui_RemovePagesDialog.h"
#include "ui_BatchProcessingLowerPanel.h"
#include "config.h"
#include "version.h"
#ifndef Q_MOC_RUN
#include <boost/foreach.hpp>
#include <boost/lambda/lambda.hpp>
#include <boost/lambda/bind.hpp>
#endif
#include <QApplication>
#include <QLineF>
#include <QPointer>
#include <QWidget>
#include <QDialog>
#include <QCloseEvent>
#include <QStackedLayout>
#include <QGridLayout>
#include <QVBoxLayout>
#include <QLayoutItem>
#include <QScrollBar>
#include <QPushButton>
#include <QCheckBox>
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
#include <QStyle>
#include <QSettings>
#include <QDomDocument>
#include <QSortFilterProxyModel>
#include <QFileSystemModel>
#include <QFileInfo>
#include <QResource>
#include <Qt>
#include <QDebug>
#include <algorithm>
#include <vector>
#include <stddef.h>
#include <math.h>
#include <assert.h>

class MainWindow::PageSelectionProviderImpl : public PageSelectionProvider
{
public:
	PageSelectionProviderImpl(MainWindow* wnd) : m_ptrWnd(wnd) {}
	
	virtual PageSequence allPages() const {
		return m_ptrWnd ? m_ptrWnd->allPages() : PageSequence();
	}

	virtual std::set<PageId> selectedPages() const {
		return m_ptrWnd ? m_ptrWnd->selectedPages() : std::set<PageId>();
	}
	
	std::vector<PageRange> selectedRanges() const {
		return m_ptrWnd ? m_ptrWnd->selectedRanges() : std::vector<PageRange>();
	}
private:
	QPointer<MainWindow> m_ptrWnd;
};


MainWindow::MainWindow()
:	m_ptrPages(new ProjectPages),
	m_ptrStages(new StageSequence(m_ptrPages, newPageSelectionAccessor())),
	m_ptrWorkerThread(new WorkerThread),
	m_ptrInteractiveQueue(new ProcessingTaskQueue(ProcessingTaskQueue::RANDOM_ORDER)),
	m_ptrOutOfMemoryDialog(new OutOfMemoryDialog),
	m_curFilter(0),
	m_ignoreSelectionChanges(0),
	m_ignorePageOrderingChanges(0),
	m_debug(false),
	m_closing(false)
{
	m_maxLogicalThumbSize = QSize(250, 160);
	m_ptrThumbSequence.reset(new ThumbnailSequence(m_maxLogicalThumbSize));
	
	setupUi(this);
	sortOptions->setVisible(false);

#if !defined(ENABLE_OPENGL)
	// Right now the only setting is 3D acceleration, so get rid of
	// the whole Settings dialog, if it's inaccessible.
	actionSettings->setVisible(false);
#endif

	createBatchProcessingWidget();
	m_ptrProcessingIndicationWidget.reset(new ProcessingIndicationWidget);
	
	filterList->setStages(m_ptrStages);
	filterList->selectRow(0);
	
	setupThumbView(); // Expects m_ptrThumbSequence to be initialized.
	
	m_ptrTabbedDebugImages.reset(new TabbedDebugImages);
	
	m_debug = actionDebug->isChecked();
	m_pImageFrameLayout = new QStackedLayout(imageViewFrame);
	m_pOptionsFrameLayout = new QStackedLayout(filterOptions);
	
	addAction(actionFirstPage);
	addAction(actionLastPage);
	addAction(actionNextPage);
	addAction(actionPrevPage);
	addAction(actionPrevPageQ);
	addAction(actionNextPageW);

	// Should be enough to save a project.
	OutOfMemoryHandler::instance().allocateEmergencyMemory(3*1024*1024);

	connect(actionFirstPage, SIGNAL(triggered(bool)), SLOT(goFirstPage()));
	connect(actionLastPage, SIGNAL(triggered(bool)), SLOT(goLastPage()));
	connect(actionPrevPage, SIGNAL(triggered(bool)), SLOT(goPrevPage()));
	connect(actionNextPage, SIGNAL(triggered(bool)), SLOT(goNextPage()));
	connect(actionPrevPageQ, SIGNAL(triggered(bool)), this, SLOT(goPrevPage()));
	connect(actionNextPageW, SIGNAL(triggered(bool)), this, SLOT(goNextPage()));
	connect(actionAbout, SIGNAL(triggered(bool)), this, SLOT(showAboutDialog()));
	connect(
		&OutOfMemoryHandler::instance(),
		SIGNAL(outOfMemory()), SLOT(handleOutOfMemorySituation())
	);
	
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
		SIGNAL(newSelectionLeader(PageInfo const&, QRectF const&, ThumbnailSequence::SelectionFlags)),
		this, SLOT(currentPageChanged(PageInfo const&, QRectF const&, ThumbnailSequence::SelectionFlags))
	);
	connect(
		m_ptrThumbSequence.get(),
		SIGNAL(pageContextMenuRequested(PageInfo const&, QPoint const&, bool)),
		this, SLOT(pageContextMenuRequested(PageInfo const&, QPoint const&, bool))
	);
	connect(
		m_ptrThumbSequence.get(),
		SIGNAL(pastLastPageContextMenuRequested(QPoint const&)),
		SLOT(pastLastPageContextMenuRequested(QPoint const&))
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
		sortOptions, SIGNAL(currentIndexChanged(int)),
		this, SLOT(pageOrderingChanged(int))
	);
	
	connect(actionFixDpi, SIGNAL(triggered(bool)), SLOT(fixDpiDialogRequested()));
	connect(actionRelinking, SIGNAL(triggered(bool)), SLOT(showRelinkingDialog()));
	connect(actionDebug, SIGNAL(toggled(bool)), SLOT(debugToggled(bool)));

	connect(
		actionSettings, SIGNAL(triggered(bool)),
		this, SLOT(openSettingsDialog())
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
	m_ptrInteractiveQueue->cancelAndClear();
	if (m_ptrBatchQueue.get()) {
		m_ptrBatchQueue->cancelAndClear();
	}
	m_ptrWorkerThread->shutdown();
	
	removeWidgetsFromLayout(m_pImageFrameLayout);
	removeWidgetsFromLayout(m_pOptionsFrameLayout);
	m_ptrTabbedDebugImages->clear();
}

PageSequence
MainWindow::allPages() const
{
	return m_ptrThumbSequence->toPageSequence();
}

std::set<PageId>
MainWindow::selectedPages() const
{
	return m_ptrThumbSequence->selectedItems();
}

std::vector<PageRange>
MainWindow::selectedRanges() const
{
	return m_ptrThumbSequence->selectedRanges();
}

void
MainWindow::switchToNewProject(
	IntrusivePtr<ProjectPages> const& pages,
	QString const& out_dir, QString const& project_file_path,
	ProjectReader const* project_reader)
{
	stopBatchProcessing(CLEAR_MAIN_AREA);
	m_ptrInteractiveQueue->cancelAndClear();

	Utils::maybeCreateCacheDir(out_dir);
	
	m_ptrPages = pages;
	m_projectFile = project_file_path;

	if (project_reader) {
		m_selectedPage = project_reader->selectedPage();
	}

	IntrusivePtr<FileNameDisambiguator> disambiguator;
	if (project_reader) {
		disambiguator = project_reader->namingDisambiguator();
	} else {
		disambiguator.reset(new FileNameDisambiguator);
	}

	m_outFileNameGen = OutputFileNameGenerator(
		disambiguator, out_dir, pages->layoutDirection()
	);
	// These two need to go in this order.
	updateDisambiguationRecords(pages->toPageSequence(IMAGE_VIEW));

	// Recreate the stages and load their state.
	m_ptrStages.reset(new StageSequence(pages, newPageSelectionAccessor()));
	if (project_reader) {
		project_reader->readFilterSettings(m_ptrStages->filters());
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

	updateSortOptions();
	
	m_ptrContentBoxPropagator.reset(
		new ContentBoxPropagator(
			m_ptrStages->pageLayoutFilter(),
			createCompositeCacheDrivenTask(
				m_ptrStages->selectContentFilterIdx()
			)
		)
	);

	m_ptrPageOrientationPropagator.reset(
		new PageOrientationPropagator(
			m_ptrStages->pageSplitFilter(),
			createCompositeCacheDrivenTask(
				m_ptrStages->fixOrientationFilterIdx()
			)
		)
	);
	
	// Thumbnails are stored relative to the output directory,
	// so recreate the thumbnail cache.
	if (out_dir.isEmpty()) {
		m_ptrThumbnailCache.reset();
	} else {
		m_ptrThumbnailCache = Utils::createThumbnailCache(m_outFileNameGen.outDir());
	}
	resetThumbSequence(currentPageOrderProvider());

	removeFilterOptionsWidget();
	updateProjectActions();
	updateWindowTitle();
	updateMainArea();

	if (!QDir(out_dir).exists()) {
		showRelinkingDialog();
	}
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

void
MainWindow::createBatchProcessingWidget()
{
	using namespace boost::lambda;

	m_ptrBatchProcessingWidget.reset(new QWidget);
	QGridLayout* layout = new QGridLayout(m_ptrBatchProcessingWidget.get());
	m_ptrBatchProcessingWidget->setLayout(layout);

	SkinnedButton* stop_btn = new SkinnedButton(
		":/icons/stop-big.png",
		":/icons/stop-big-hovered.png",
		":/icons/stop-big-pressed.png",
		m_ptrBatchProcessingWidget.get()
	);
	stop_btn->setStatusTip(tr("Stop batch processing"));
	
	class LowerPanel : public QWidget
	{
	public:
		LowerPanel(QWidget* parent = 0) : QWidget(parent) { ui.setupUi(this); }

		Ui::BatchProcessingLowerPanel ui;
	};
	LowerPanel* lower_panel = new LowerPanel(m_ptrBatchProcessingWidget.get());
	m_checkBeepWhenFinished = bind(&QCheckBox::isChecked, lower_panel->ui.beepWhenFinished);

	int row = 0; // Row 0 is reserved.
	layout->addWidget(stop_btn, ++row, 1, Qt::AlignCenter);
	layout->addWidget(lower_panel, ++row, 0, 1, 3, Qt::AlignHCenter|Qt::AlignTop);
	layout->setColumnStretch(0, 1);
	layout->setColumnStretch(2, 1);
	layout->setRowStretch(0, 1);
	layout->setRowStretch(row, 1);
	
	connect(stop_btn, SIGNAL(clicked()), SLOT(stopBatchProcessing()));
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

IntrusivePtr<PageOrderProvider const>
MainWindow::currentPageOrderProvider() const
{
	int const idx = sortOptions->currentIndex();
	if (idx < 0) {
		return IntrusivePtr<PageOrderProvider const>();
	}

	IntrusivePtr<AbstractFilter> const filter(m_ptrStages->filterAt(m_curFilter));
	return filter->pageOrderOptions()[idx].provider();
}

void
MainWindow::updateSortOptions()
{
	ScopedIncDec<int> const guard(m_ignorePageOrderingChanges);

	IntrusivePtr<AbstractFilter> const filter(m_ptrStages->filterAt(m_curFilter));

	sortOptions->clear();
	
	BOOST_FOREACH(PageOrderOption const& opt, filter->pageOrderOptions()) {
		sortOptions->addItem(opt.name());
	}

	sortOptions->setVisible(sortOptions->count() > 0);
	
	if (sortOptions->count() > 0) {
		sortOptions->setCurrentIndex(filter->selectedPageOrder());
	}
}

void
MainWindow::resetThumbSequence(
	IntrusivePtr<PageOrderProvider const> const& page_order_provider)
{
	if (m_ptrThumbnailCache.get()) {
		IntrusivePtr<CompositeCacheDrivenTask> const task(
			createCompositeCacheDrivenTask(m_curFilter)
		);
		
		m_ptrThumbSequence->setThumbnailFactory(
			IntrusivePtr<ThumbnailFactory>(
				new ThumbnailFactory(
					m_ptrThumbnailCache,
					m_maxLogicalThumbSize, task
				)
			)
		);
	}
	
	m_ptrThumbSequence->reset(
		m_ptrPages->toPageSequence(getCurrentView()),
		ThumbnailSequence::RESET_SELECTION, page_order_provider
	);
	
	if (!m_ptrThumbnailCache.get()) {
		// Empty project.
		assert(m_ptrPages->numImages() == 0);
		m_ptrThumbSequence->setThumbnailFactory(
			IntrusivePtr<ThumbnailFactory>()
		);
	}

	PageId const page(m_selectedPage.get(getCurrentView()));
	if (m_ptrThumbSequence->setSelection(page)) {
		// OK
	} else if (m_ptrThumbSequence->setSelection(PageId(page.imageId(), PageId::LEFT_PAGE))) {
		// OK
	} else if (m_ptrThumbSequence->setSelection(PageId(page.imageId(), PageId::RIGHT_PAGE))) {
		// OK
	} else if (m_ptrThumbSequence->setSelection(PageId(page.imageId(), PageId::SINGLE_PAGE))) {
		// OK
	} else {
		// Last resort.
		m_ptrThumbSequence->setSelection(m_ptrThumbSequence->firstPage().id());
	}
}

void
MainWindow::setOptionsWidget(FilterOptionsWidget* widget, Ownership const ownership)
{
	if (isBatchProcessingInProgress()) {
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
			this, SLOT(invalidateThumbnail(PageId const&))
		);
		disconnect(
			m_ptrOptionsWidget, SIGNAL(invalidateThumbnail(PageInfo const&)),
			this, SLOT(invalidateThumbnail(PageInfo const&))
		);
		disconnect(
			m_ptrOptionsWidget, SIGNAL(invalidateAllThumbnails()),
			this, SLOT(invalidateAllThumbnails())
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
		this, SLOT(invalidateThumbnail(PageId const&))
	);
	connect(
		widget, SIGNAL(invalidateThumbnail(PageInfo const&)),
		this, SLOT(invalidateThumbnail(PageInfo const&))
	);
	connect(
		widget, SIGNAL(invalidateAllThumbnails()),
		this, SLOT(invalidateAllThumbnails())
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
	if (isBatchProcessingInProgress() && widget != m_ptrBatchProcessingWidget.get()) {
		if (ownership == TRANSFER_OWNERSHIP) {
			delete widget;
		}
		return;
	}
	
	removeImageWidget();
	
	if (ownership == TRANSFER_OWNERSHIP) {
		m_imageWidgetCleanup.add(widget);
	}
	
	if (!debug_images || debug_images->empty()) {
		m_pImageFrameLayout->addWidget(widget);
	} else {
		m_ptrTabbedDebugImages->addTab(widget, "Main");
		AutoRemovingFile file;
		QString label;
		while (!(file = debug_images->retrieveNext(&label)).get().isNull()) {
			QWidget* widget = new DebugImageView(file);
			m_imageWidgetCleanup.add(widget);
			m_ptrTabbedDebugImages->addTab(widget, label);
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
MainWindow::invalidateThumbnail(PageInfo const& page_info)
{
	m_ptrThumbSequence->invalidateThumbnail(page_info);
}

void
MainWindow::invalidateAllThumbnails()
{
	m_ptrThumbSequence->invalidateAllThumbnails();
}

IntrusivePtr<AbstractCommand0<void> >
MainWindow::relinkingDialogRequester()
{
	class Requester : public AbstractCommand0<void>
	{
	public:
		Requester(MainWindow* wnd) : m_ptrWnd(wnd) {}

		virtual void operator()() {
			if (MainWindow* wnd = m_ptrWnd) {
				wnd->showRelinkingDialog();
			}
		}
	private:
		QPointer<MainWindow> m_ptrWnd;
	};

	return IntrusivePtr<AbstractCommand0<void> >(new Requester(this));
}

void
MainWindow::showRelinkingDialog()
{
	if (!isProjectLoaded()) {
		return;
	}

	RelinkingDialog* dialog = new RelinkingDialog(m_projectFile, this);
	dialog->setAttribute(Qt::WA_DeleteOnClose);
	dialog->setWindowModality(Qt::WindowModal);

	m_ptrPages->listRelinkablePaths(dialog->pathCollector());
	dialog->pathCollector()(RelinkablePath(m_outFileNameGen.outDir(), RelinkablePath::Dir));
	
	new QtSignalForwarder(
		dialog, SIGNAL(accepted()),
		boost::lambda::bind(&MainWindow::performRelinking, this, dialog->relinker())
	);

	dialog->show();
}

void
MainWindow::performRelinking(IntrusivePtr<AbstractRelinker> const& relinker)
{
	assert(relinker.get());

	if (!isProjectLoaded()) {
		return;
	}

	m_ptrPages->performRelinking(*relinker);
	m_ptrStages->performRelinking(*relinker);
	m_outFileNameGen.performRelinking(*relinker);

	Utils::maybeCreateCacheDir(m_outFileNameGen.outDir());

	m_ptrThumbnailCache->setThumbDir(Utils::outputDirToThumbDir(m_outFileNameGen.outDir()));
	resetThumbSequence(currentPageOrderProvider());
	m_selectedPage.set(m_ptrThumbSequence->selectionLeader().id(), getCurrentView());

	reloadRequested();
}

void
MainWindow::goFirstPage()
{
	if (isBatchProcessingInProgress() || !isProjectLoaded()) {
		return;
	}
	
	PageInfo const first_page(m_ptrThumbSequence->firstPage());
	if (!first_page.isNull()) {
		goToPage(first_page.id());
	}
}

void
MainWindow::goLastPage()
{
	if (isBatchProcessingInProgress() || !isProjectLoaded()) {
		return;
	}
	
	PageInfo const last_page(m_ptrThumbSequence->lastPage());
	if (!last_page.isNull()) {
		goToPage(last_page.id());
	}
}

void
MainWindow::goNextPage()
{
	if (isBatchProcessingInProgress() || !isProjectLoaded()) {
		return;
	}
	
	PageInfo const next_page(
		m_ptrThumbSequence->nextPage(m_ptrThumbSequence->selectionLeader().id())
	);
	if (!next_page.isNull()) {
		goToPage(next_page.id());
	}
}

void
MainWindow::goPrevPage()
{
	if (isBatchProcessingInProgress() || !isProjectLoaded()) {
		return;
	}
	
	PageInfo const prev_page(
		m_ptrThumbSequence->prevPage(m_ptrThumbSequence->selectionLeader().id())
	);
	if (!prev_page.isNull()) {
		goToPage(prev_page.id());
	}
}

void
MainWindow::goToPage(PageId const& page_id)
{
	focusButton->setChecked(true);
	
	m_ptrThumbSequence->setSelection(page_id);
	
	// If the page was already selected, it will be reloaded.
	// That's by design.
	updateMainArea();
}

void
MainWindow::currentPageChanged(
	PageInfo const& page_info, QRectF const& thumb_rect,
	ThumbnailSequence::SelectionFlags const flags)
{
	m_selectedPage.set(page_info.id(), getCurrentView());

	if ((flags & ThumbnailSequence::SELECTED_BY_USER) || focusButton->isChecked()) {
		if (!(flags & ThumbnailSequence::AVOID_SCROLLING_TO)) {
			thumbView->ensureVisible(thumb_rect, 0, 0);
		}
	}
	
	if (flags & ThumbnailSequence::SELECTED_BY_USER) {
		if (isBatchProcessingInProgress()) {
			stopBatchProcessing();
		} else if (!(flags & ThumbnailSequence::REDUNDANT_SELECTION)) {
			// Start loading / processing the newly selected page.
			updateMainArea();
		}
	}
}

void
MainWindow::pageContextMenuRequested(
	PageInfo const& page_info_, QPoint const& screen_pos, bool selected)
{
	if (isBatchProcessingInProgress()) {
		return;
	}

	// Make a copy to prevent it from being invalidated.
	PageInfo const page_info(page_info_);
	
	if (!selected) {
		goToPage(page_info.id());
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
		showRemovePagesDialog(m_ptrThumbSequence->selectedItems());
	}
}

void
MainWindow::pastLastPageContextMenuRequested(QPoint const& screen_pos)
{
	if (!isProjectLoaded()) {
		return;
	}

	QMenu menu;
	menu.addAction(QIcon(":/icons/insert-here-16.png"), tr("Insert here ..."));
	
	if (menu.exec(screen_pos)) {
		showInsertFileDialog(BEFORE, ImageId());
	}
}

void
MainWindow::thumbViewFocusToggled(bool const checked)
{
	QRectF const rect(m_ptrThumbSequence->selectionLeaderSceneRect());
	if (rect.isNull()) {
		// No selected items.
		return;
	}
	
	if (checked) {
		thumbView->ensureVisible(rect, 0, 0);
	}
}

void
MainWindow::thumbViewScrolled()
{
	QRectF const rect(m_ptrThumbSequence->selectionLeaderSceneRect());
	if (rect.isNull()) {
		// No items selected.
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
	
	m_ptrInteractiveQueue->cancelAndClear();
	if (m_ptrBatchQueue.get()) {
		// Should not happen, but just in case.
		m_ptrBatchQueue->cancelAndClear();
	}
	
	bool const was_below_fix_orientation = isBelowFixOrientation(m_curFilter);
	bool const was_below_select_content = isBelowSelectContent(m_curFilter);
	m_curFilter = selected.front().top();
	bool const now_below_fix_orientation = isBelowFixOrientation(m_curFilter);
	bool const now_below_select_content = isBelowSelectContent(m_curFilter);
	
	m_ptrStages->filterAt(m_curFilter)->selected();
	
	updateSortOptions();

	// Propagate context boxes down the stage list, if necessary.
	if (!was_below_select_content && now_below_select_content) {
		// IMPORTANT: this needs to go before resetting thumbnails,
		// because it may affect them.
		if (m_ptrContentBoxPropagator.get()) {
			m_ptrContentBoxPropagator->propagate(*m_ptrPages);
		} // Otherwise probably no project is loaded.
	}

	// Propagate page orientations (that might have changed) to the "Split Pages" stage.
	if (!was_below_fix_orientation && now_below_fix_orientation) {
		// IMPORTANT: this needs to go before resetting thumbnails,
		// because it may affect them.
		if (m_ptrPageOrientationPropagator.get()) {
			m_ptrPageOrientationPropagator->propagate(*m_ptrPages);
		} // Otherwise probably no project is loaded.
	}
	
	focusButton->setChecked(true); // Should go before resetThumbSequence().
	resetThumbSequence(currentPageOrderProvider());
	
	updateMainArea();
}

void
MainWindow::pageOrderingChanged(int idx)
{
	if (m_ignorePageOrderingChanges) {
		return;
	}

	focusButton->setChecked(true); // Keep the current page in view.

	m_ptrStages->filterAt(m_curFilter)->selectPageOrder(idx);

	m_ptrThumbSequence->reset(
		m_ptrPages->toPageSequence(getCurrentView()),
		ThumbnailSequence::KEEP_SELECTION,
		currentPageOrderProvider()
	);
}

void
MainWindow::reloadRequested()
{
	// Start loading / processing the current page.
	updateMainArea();
}

void
MainWindow::startBatchProcessing()
{
	if (isBatchProcessingInProgress() || !isProjectLoaded()) {
		return;
	}

	m_ptrInteractiveQueue->cancelAndClear();
	
	m_ptrBatchQueue.reset(
		new ProcessingTaskQueue(
			currentPageOrderProvider().get()
			? ProcessingTaskQueue::RANDOM_ORDER
			: ProcessingTaskQueue::SEQUENTIAL_ORDER
		)
	);
	PageInfo page(m_ptrThumbSequence->selectionLeader());
	for (; !page.isNull(); page = m_ptrThumbSequence->nextPage(page.id())) {
		m_ptrBatchQueue->addProcessingTask(
			page, createCompositeTask(page, m_curFilter, /*batch=*/true, m_debug)
		);
	}

	focusButton->setChecked(true);
	
	removeFilterOptionsWidget();
	filterList->setBatchProcessingInProgress(true);
	filterList->setEnabled(false);

	BackgroundTaskPtr const task(m_ptrBatchQueue->takeForProcessing());
	if (task) {
		m_ptrWorkerThread->performTask(task);
	} else {
		stopBatchProcessing();
	}

	page = m_ptrBatchQueue->selectedPage();
	if (!page.isNull()) {
		m_ptrThumbSequence->setSelection(page.id());
	}

	// Display the batch processing screen.
	updateMainArea();
}

void
MainWindow::stopBatchProcessing(MainAreaAction main_area)
{
	if (!isBatchProcessingInProgress()) {
		return;
	}
	
	PageInfo const page(m_ptrBatchQueue->selectedPage());
	if (!page.isNull()) {
		m_ptrThumbSequence->setSelection(page.id());
	}

	m_ptrBatchQueue->cancelAndClear();
	m_ptrBatchQueue.reset();
	
	filterList->setBatchProcessingInProgress(false);
	filterList->setEnabled(true);

	switch (main_area) {
		case UPDATE_MAIN_AREA:
			updateMainArea();
			break;
		case CLEAR_MAIN_AREA:
			removeImageWidget();
			break;
	}
}

void
MainWindow::filterResult(BackgroundTaskPtr const& task, FilterResultPtr const& result)
{
	// Cancelled or not, we must mark it as finished.
	m_ptrInteractiveQueue->processingFinished(task);
	if (m_ptrBatchQueue.get()) {
		m_ptrBatchQueue->processingFinished(task);
	}

	if (task->isCancelled()) {
		return;
	}
	
	if (!isBatchProcessingInProgress()) {
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

	// This needs to be done even if batch processing is taking place,
	// for instance because thumbnail invalidation is done from here.
	result->updateUI(this);
	
	if (isBatchProcessingInProgress()) {
		if (m_ptrBatchQueue->allProcessed()) {
			stopBatchProcessing();
			
			QApplication::alert(this); // Flash the taskbar entry.
			if (m_checkBeepWhenFinished()) {
				QApplication::beep();
			}

			if (m_selectedPage.get(getCurrentView()) == m_ptrThumbSequence->lastPage().id()) {
				// If batch processing finished at the last page, jump to the first one.	
				goFirstPage();
			}

			return;
		}

		BackgroundTaskPtr const task(m_ptrBatchQueue->takeForProcessing());
		if (task) {
			m_ptrWorkerThread->performTask(task);
		}

		PageInfo const page(m_ptrBatchQueue->selectedPage());
		if (!page.isNull()) {
			m_ptrThumbSequence->setSelection(page.id());
		}
	}
}

void
MainWindow::debugToggled(bool const enabled)
{
	m_debug = enabled;
}

void
MainWindow::fixDpiDialogRequested()
{
	if (isBatchProcessingInProgress() || !isProjectLoaded()) {
		return;
	}

	assert(!m_ptrFixDpiDialog);
	m_ptrFixDpiDialog = new FixDpiDialog(m_ptrPages->toImageFileInfo(), this);
	m_ptrFixDpiDialog->setAttribute(Qt::WA_DeleteOnClose);
	m_ptrFixDpiDialog->setWindowModality(Qt::WindowModal);

	connect(m_ptrFixDpiDialog, SIGNAL(accepted()), SLOT(fixedDpiSubmitted()));

	m_ptrFixDpiDialog->show();
}

void
MainWindow::fixedDpiSubmitted()
{
	assert(m_ptrFixDpiDialog);
	assert(m_ptrPages);
	assert(m_ptrThumbSequence.get());

	PageInfo const selected_page_before(m_ptrThumbSequence->selectionLeader());

	m_ptrPages->updateMetadataFrom(m_ptrFixDpiDialog->files());
	
	// The thumbnail list also stores page metadata, including the DPI.
	m_ptrThumbSequence->reset(
		m_ptrPages->toPageSequence(getCurrentView()),
		ThumbnailSequence::KEEP_SELECTION, m_ptrThumbSequence->pageOrderProvider()
	);

	PageInfo const selected_page_after(m_ptrThumbSequence->selectionLeader());

	// Reload if the current page was affected.
	// Note that imageId() isn't supposed to change - we check just in case.
	if (selected_page_before.imageId() != selected_page_after.imageId() ||
		selected_page_before.metadata() != selected_page_after.metadata()) {
		
		reloadRequested();
	}
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
	// XXX: this function is duplicated in OutOfMemoryDialog.

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
	IntrusivePtr<ProjectPages> pages(
		new ProjectPages(
			context->files(), ProjectPages::AUTO_PAGES,
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
			this, tr("Open Project"), project_dir,
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
			this, tr("Error"),
			tr("Unable to open the project file.")
		);
		return;
	}
	
	QDomDocument doc;
	if (!doc.setContent(&file)) {
		QMessageBox::warning(
			this, tr("Error"),
			tr("The project file is broken.")
		);
		return;
	}
	
	file.close();
	
	ProjectOpeningContext* context = new ProjectOpeningContext(this, project_file, doc);
	connect(context, SIGNAL(done(ProjectOpeningContext*)), SLOT(projectOpened(ProjectOpeningContext*)));
	context->proceed();
}

void
MainWindow::projectOpened(ProjectOpeningContext* context)
{
	RecentProjects rp;
	rp.read();
	rp.setMostRecent(context->projectFile());
	rp.write();
	
	QSettings settings;
	settings.setValue(
		"project/lastDir",
		QFileInfo(context->projectFile()).absolutePath()
	);
	
	switchToNewProject(
		context->projectReader()->pages(),
		context->projectReader()->outputDirectory(),
		context->projectFile(), context->projectReader()
	);
}

void
MainWindow::closeProject()
{
	closeProjectInteractive();
}

void
MainWindow::openSettingsDialog()
{
	SettingsDialog* dialog = new SettingsDialog(this);
	dialog->setAttribute(Qt::WA_DeleteOnClose);
	dialog->setWindowModality(Qt::WindowModal);
	dialog->show();
}

void
MainWindow::showAboutDialog()
{
	Ui::AboutDialog ui;
	QDialog* dialog = new QDialog(this);
	ui.setupUi(dialog);
	ui.version->setText(QString::fromUtf8(VERSION));

	QResource license(":/GPLv3.html");
	ui.licenseViewer->setHtml(QString::fromUtf8((char const*)license.data(), license.size()));

	dialog->setAttribute(Qt::WA_DeleteOnClose);
	dialog->setWindowModality(Qt::WindowModal);
	dialog->show();
}

/**
 * This function is called asynchronously, always from the main thread.
 */
void
MainWindow::handleOutOfMemorySituation()
{
	deleteLater();

	m_ptrOutOfMemoryDialog->setParams(
		m_projectFile, m_ptrStages, m_ptrPages, m_selectedPage, m_outFileNameGen
	);

	closeProjectWithoutSaving();

	m_ptrOutOfMemoryDialog->setAttribute(Qt::WA_DeleteOnClose);
	m_ptrOutOfMemoryDialog.release()->show();
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
	actionFixDpi->setEnabled(loaded);
	actionRelinking->setEnabled(loaded);
}

bool
MainWindow::isBatchProcessingInProgress() const
{
	return m_ptrBatchQueue.get() != 0;
}

bool
MainWindow::isProjectLoaded() const
{
	return !m_outFileNameGen.outDir().isEmpty();
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
MainWindow::isBelowFixOrientation(int filter_idx) const
{
	return (filter_idx > m_ptrStages->fixOrientationFilterIdx());
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

PageView
MainWindow::getCurrentView() const
{
	return m_ptrStages->filterAt(m_curFilter)->getView();
}

void
MainWindow::updateMainArea()
{
	if (m_ptrPages->numImages() == 0) {
		filterList->setBatchProcessingPossible(false);
		showNewOpenProjectPanel();
	} else if (isBatchProcessingInProgress()) {
		filterList->setBatchProcessingPossible(false);
		setImageWidget(m_ptrBatchProcessingWidget.get(), KEEP_OWNERSHIP);
	} else {
		PageInfo const page(m_ptrThumbSequence->selectionLeader());
		if (page.isNull()) {
			filterList->setBatchProcessingPossible(false);
			removeImageWidget();
			removeFilterOptionsWidget();
		} else {
			// Note that loadPageInteractive may reset it to false.
			filterList->setBatchProcessingPossible(true);
			loadPageInteractive(page);
		}
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
MainWindow::loadPageInteractive(PageInfo const& page)
{
	assert(!isBatchProcessingInProgress());
	
	m_ptrInteractiveQueue->cancelAndClear();
	
	if (isOutputFilter() && !checkReadyForOutput(&page.id())) {
		filterList->setBatchProcessingPossible(false);
		
		// Switch to the first page - the user will need
		// to process all pages in batch mode.
		m_ptrThumbSequence->setSelection(m_ptrThumbSequence->firstPage().id());
		
		QString const err_text(
			tr("Output is not yet possible, as the final size"
			" of pages is not yet known.\nTo determine it,"
			" run batch processing at \"Select Content\" or"
			" \"Margins\".")
		);
		
		removeFilterOptionsWidget();
		setImageWidget(new ErrorWidget(err_text), TRANSFER_OWNERSHIP);
		return;
	}
	
	if (!isBatchProcessingInProgress()) {
		if (m_pImageFrameLayout->indexOf(m_ptrProcessingIndicationWidget.get()) != -1) {
			m_ptrProcessingIndicationWidget->processingRestartedEffect();
		}
		setImageWidget(m_ptrProcessingIndicationWidget.get(), KEEP_OWNERSHIP);
		m_ptrStages->filterAt(m_curFilter)->preUpdateUI(this, page.id());
	}
	
	assert(m_ptrThumbnailCache.get());

	m_ptrInteractiveQueue->cancelAndClear();
	m_ptrInteractiveQueue->addProcessingTask(
		page, createCompositeTask(page, m_curFilter, /*batch=*/false, m_debug)
	);
	m_ptrWorkerThread->performTask(m_ptrInteractiveQueue->takeForProcessing());
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
	setWindowTitle(tr("%2 - Scan Tailor %3 [%1bit]").arg(sizeof(void*)*8).arg(project_name, version));
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
	
	ProjectWriter writer(m_ptrPages, m_selectedPage, m_outFileNameGen);
	
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
	IntrusivePtr<ProjectPages> pages(new ProjectPages());
	switchToNewProject(pages, QString());
}

bool
MainWindow::saveProjectWithFeedback(QString const& project_file)
{
	ProjectWriter writer(m_ptrPages, m_selectedPage, m_outFileNameGen);
	
	if (!writer.write(project_file, m_ptrStages->filters())) {
		QMessageBox::warning(
			this, tr("Error"),
			tr("Error saving the project file!")
		);
		return false;
	}
	
	return true;
}

/**
 * Note: showInsertFileDialog(BEFORE, ImageId()) is legal and means inserting at the end.
 */
void
MainWindow::showInsertFileDialog(BeforeOrAfter before_or_after, ImageId const& existing)
{
	if (isBatchProcessingInProgress() || !isProjectLoaded()) {
		return;
	}

	// We need to filter out files already in project.
	class ProxyModel : public QSortFilterProxyModel
	{
	public:
		ProxyModel(ProjectPages const& pages) {
			setDynamicSortFilter(true);
			
			PageSequence const sequence(pages.toPageSequence(IMAGE_VIEW));
			unsigned const count = sequence.numPages();
			for (unsigned i = 0; i < count; ++i) {
				PageInfo const& page = sequence.pageAt(i);
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
			this, tr("Files to insert"),
			QFileInfo(existing.filePath()).absolutePath()
		)
	);
	dialog->setFileMode(QFileDialog::ExistingFiles);
	dialog->setProxyModel(new ProxyModel(*m_ptrPages));
	dialog->setNameFilter(tr("Images not in project (%1)").arg("*.png *.tiff *.tif *.jpeg *.jpg"));
	// XXX: Adding individual pages from a multi-page TIFF where
	// some of the pages are already in project is not supported right now.

	if (dialog->exec() != QDialog::Accepted) {
		return;
	}
	
	QStringList files(dialog->selectedFiles());
	if (files.empty()) {
		return;
	}

	// The order of items returned by QFileDialog is platform-dependent,
	// so we enforce our own ordering.
	std::sort(files.begin(), files.end(), SmartFilenameOrdering());

	// I suspect on some platforms it may be possible to select the same file twice,
	// so to be safe, remove duplicates.
	files.erase(std::unique(files.begin(), files.end()), files.end());
	
	using namespace boost::lambda;
	
	std::vector<ImageFileInfo> new_files;
	std::vector<QString> loaded_files;
	std::vector<QString> failed_files; // Those we failed to read metadata from.

	// dialog->selectedFiles() returns file list in reverse order.
	for (int i = files.size() - 1; i >= 0; --i) {
		QFileInfo const file_info(files[i]);
		ImageFileInfo image_file_info(file_info, std::vector<ImageMetadata>());

		void (std::vector<ImageMetadata>::*push_back) (const ImageMetadata&) =
			&std::vector<ImageMetadata>::push_back;
		ImageMetadataLoader::Status const status = ImageMetadataLoader::load(
			files.at(i), boost::lambda::bind(push_back,
			boost::ref(image_file_info.imageInfo()), boost::lambda::_1)
		);

		if (status == ImageMetadataLoader::LOADED) {
			new_files.push_back(image_file_info);
			loaded_files.push_back(file_info.absoluteFilePath());
		} else {
			failed_files.push_back(file_info.absoluteFilePath());
		}
	}

	if (!failed_files.empty()) {
		std::auto_ptr<LoadFilesStatusDialog> err_dialog(new LoadFilesStatusDialog(this));
		err_dialog->setLoadedFiles(loaded_files);
		err_dialog->setFailedFiles(failed_files);
		err_dialog->setOkButtonName(QString(" %1 ").arg(tr("Skip failed files")));
		if (err_dialog->exec() != QDialog::Accepted || loaded_files.empty()) {
			return;
		}
	}

	// Check if there is at least one DPI that's not OK.
	if (std::find_if(new_files.begin(), new_files.end(), !boost::lambda::bind(&ImageFileInfo::isDpiOK, boost::lambda::_1)) != new_files.end()) {

		std::auto_ptr<FixDpiDialog> dpi_dialog(new FixDpiDialog(new_files, this));
		dpi_dialog->setWindowModality(Qt::WindowModal);
		if (dpi_dialog->exec() != QDialog::Accepted) {
			return;
		}

		new_files = dpi_dialog->files();
	}

	// Actually insert the new pages.
	BOOST_FOREACH(ImageFileInfo const& file, new_files) {
		int image_num = -1; // Zero-based image number in a multi-page TIFF.

		BOOST_FOREACH(ImageMetadata const& metadata, file.imageInfo()) {
			++image_num;

			int const num_sub_pages = ProjectPages::adviseNumberOfLogicalPages(
				metadata, OrthogonalRotation()
			);
			ImageInfo const image_info(
				ImageId(file.fileInfo(), image_num), metadata, num_sub_pages, false, false
			);
			insertImage(image_info, before_or_after, existing);
		}
	}
}

void
MainWindow::showRemovePagesDialog(std::set<PageId> const& pages)
{
	std::auto_ptr<QDialog> dialog(new QDialog(this));
	Ui::RemovePagesDialog ui;
	ui.setupUi(dialog.get());
	ui.icon->setPixmap(style()->standardIcon(QStyle::SP_MessageBoxQuestion).pixmap(48, 48));

	ui.text->setText(ui.text->text().arg(pages.size()));

	QPushButton* remove_btn = ui.buttonBox->button(QDialogButtonBox::Ok);
	remove_btn->setText(tr("Remove"));

	dialog->setWindowModality(Qt::WindowModal);
	if (dialog->exec() == QDialog::Accepted) {
		removeFromProject(pages);
		eraseOutputFiles(pages);
	}
}

/**
 * Note: insertImage(..., BEFORE, ImageId()) is legal and means inserting at the end.
 */
void
MainWindow::insertImage(ImageInfo const& new_image,
	BeforeOrAfter before_or_after, ImageId existing)
{
	std::vector<PageInfo> pages(
		m_ptrPages->insertImage(
			new_image, before_or_after, existing, getCurrentView()
		)
	);

	if (before_or_after == BEFORE) {
		// The second one will be inserted first, then the first
		// one will be inserted BEFORE the second one.
		std::reverse(pages.begin(), pages.end());
	}
	
	BOOST_FOREACH(PageInfo const& page_info, pages) {
		m_outFileNameGen.disambiguator()->registerFile(page_info.imageId().filePath());
		m_ptrThumbSequence->insert(page_info, before_or_after, existing);
		existing = page_info.imageId();
	}
}

void
MainWindow::removeFromProject(std::set<PageId> const& pages)
{
	m_ptrInteractiveQueue->cancelAndRemove(pages);
	if (m_ptrBatchQueue.get()) {
		m_ptrBatchQueue->cancelAndRemove(pages);
	}

	m_ptrPages->removePages(pages);
	m_ptrThumbSequence->removePages(pages);
	
	if (m_ptrThumbSequence->selectionLeader().isNull()) {
		m_ptrThumbSequence->setSelection(m_ptrThumbSequence->firstPage().id());
	}
	
	updateMainArea();
}

void
MainWindow::eraseOutputFiles(std::set<PageId> const& pages)
{
	std::vector<PageId::SubPage> erase_variations;
	erase_variations.reserve(3);

	BOOST_FOREACH(PageId const& page_id, pages) {
		erase_variations.clear();
		switch (page_id.subPage()) {
			case PageId::SINGLE_PAGE:
				erase_variations.push_back(PageId::SINGLE_PAGE);
				erase_variations.push_back(PageId::LEFT_PAGE);
				erase_variations.push_back(PageId::RIGHT_PAGE);
				break;
			case PageId::LEFT_PAGE:
				erase_variations.push_back(PageId::SINGLE_PAGE);
				erase_variations.push_back(PageId::LEFT_PAGE);
				break;
			case PageId::RIGHT_PAGE:
				erase_variations.push_back(PageId::SINGLE_PAGE);
				erase_variations.push_back(PageId::RIGHT_PAGE);
				break;
		}
		
		BOOST_FOREACH(PageId::SubPage subpage, erase_variations) {
			QFile::remove(m_outFileNameGen.filePathFor(PageId(page_id.imageId(), subpage))); 
		}
	}
}

BackgroundTaskPtr
MainWindow::createCompositeTask(
	PageInfo const& page, int const last_filter_idx, bool const batch, bool debug)
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
				->createCacheDrivenTask(m_outFileNameGen);
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

void
MainWindow::updateDisambiguationRecords(PageSequence const& pages)
{
	int const count = pages.numPages();
	for (int i = 0; i < count; ++i) {
		m_outFileNameGen.disambiguator()->registerFile(pages.pageAt(i).imageId().filePath());
	}
}

PageSelectionAccessor
MainWindow::newPageSelectionAccessor()
{
	IntrusivePtr<PageSelectionProvider const> provider(new PageSelectionProviderImpl(this));
	return PageSelectionAccessor(provider);
}
