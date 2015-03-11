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

#ifndef MAINWINDOW_H_
#define MAINWINDOW_H_

#include "ui_MainWindow.h"
#include "FilterUiInterface.h"
#include "NonCopyable.h"
#include "AbstractCommand.h"
#include "IntrusivePtr.h"
#include "BackgroundTask.h"
#include "FilterResult.h"
#include "ThumbnailSequence.h"
#include "OutputFileNameGenerator.h"
#include "PageId.h"
#include "PageView.h"
#include "PageRange.h"
#include "SelectedPage.h"
#include "BeforeOrAfter.h"
#include <boost/function.hpp>
#include <QMainWindow>
#include <QString>
#include <QPointer>
#include <QObjectCleanupHandler>
#include <QSizeF>
#include <memory>
#include <vector>
#include <set>
//Export_Subscans
#include <QMessageBox>
#include "stdint.h"
#include "TiffWriter.h"
#include "ImageLoader.h"
#include "ExportDialog.h"

class AbstractFilter;
class AbstractRelinker;
class ThumbnailPixmapCache;
class ProjectPages;
class PageSequence;
class StageSequence;
class PageOrderProvider;
class PageSelectionAccessor;
class FilterOptionsWidget;
class ProcessingIndicationWidget;
class ImageInfo;
class PageInfo;
class QStackedLayout;
class WorkerThread;
class ProjectReader;
class DebugImages;
class ContentBoxPropagator;
class PageOrientationPropagator;
class ProjectCreationContext;
class ProjectOpeningContext;
class CompositeCacheDrivenTask;
class TabbedDebugImages;
class ProcessingTaskQueue;
class FixDpiDialog;
class OutOfMemoryDialog;
class QLineF;
class QRectF;
class QLayout;

class MainWindow :
	public QMainWindow,
	private FilterUiInterface,
	private Ui::MainWindow
{
	DECLARE_NON_COPYABLE(MainWindow)
	Q_OBJECT
public:
	MainWindow();
	
	virtual ~MainWindow();
	
	PageSequence allPages() const;

	std::set<PageId> selectedPages() const;
	
	std::vector<PageRange> selectedRanges() const;
	QImage m_orig_fore_subscan;	
protected:
    bool eventFilter(QObject *obj, QEvent *ev);
	virtual void closeEvent(QCloseEvent* event);
	
	virtual void timerEvent(QTimerEvent* event);
public slots:
	void openProject(QString const& project_file);
//Export_Subscans
	void ExportOutput(QString export_dir_path, bool default_out_dir, bool split_subscans,
		bool generate_blank_back_subscans, bool orig_fore_subscan);
	void ExportStop();
	void SetStartExport();
//Auto_Save_Project
	void AutoSaveProjectState(bool auto_save);
//Dont_Equalize_Illumination_Pic_Zones
	void DontEqualizeIlluminationPicZones(bool);
private:
	enum MainAreaAction { UPDATE_MAIN_AREA, CLEAR_MAIN_AREA };
//Original_Foreground_Mixed
signals:
	void StartExportTimerSignal();
private slots:
	void goFirstPage();

	void goLastPage();

	void goNextPage();
	
	void goPrevPage();
	
	void goToPage(PageId const& page_id);
	
	void currentPageChanged(
		PageInfo const& page_info, QRectF const& thumb_rect,
		ThumbnailSequence::SelectionFlags flags);
	
	void pageContextMenuRequested(
		PageInfo const& page_info, QPoint const& screen_pos, bool selected);
	
	void pastLastPageContextMenuRequested(QPoint const& screen_pos);

	void thumbViewFocusToggled(bool checked);
	
	void thumbViewScrolled();

	void filterSelectionChanged(QItemSelection const& selected);
	void switchFilter1();
	void switchFilter2();
	void switchFilter3();
	void switchFilter4();
	void switchFilter5();
	void switchFilter6();

	void pageOrderingChanged(int idx);
	
	void reloadRequested();
	
	void startBatchProcessing();
	
	void stopBatchProcessing(MainAreaAction main_area = UPDATE_MAIN_AREA);
	
	void invalidateThumbnail(PageId const& page_id);

	void invalidateThumbnail(PageInfo const& page_info);
	
	void invalidateAllThumbnails();

	void showRelinkingDialog();
	
	void filterResult(
		BackgroundTaskPtr const& task,
		FilterResultPtr const& result);
	
	void debugToggled(bool enabled);
	
	void fixDpiDialogRequested();

	void fixedDpiSubmitted();

	void saveProjectTriggered();
	
	void saveProjectAsTriggered();
	
	void newProject();
	
	void newProjectCreated(ProjectCreationContext* context);
	
	void openProject();
	
	void projectOpened(ProjectOpeningContext* context);
	
	void closeProject();

	void openSettingsDialog();

	void showAboutDialog();

	void handleOutOfMemorySituation();
//Export_Subscans
	void openExportDialog();
private:
	class PageSelectionProviderImpl;
	enum SavePromptResult { SAVE, DONT_SAVE, CANCEL };
	
	typedef IntrusivePtr<AbstractFilter> FilterPtr;
	
	virtual void setOptionsWidget(
		FilterOptionsWidget* widget, Ownership ownership);
	
	virtual void setImageWidget(
		QWidget* widget, Ownership ownership,
		DebugImages* debug_images = 0);

	virtual IntrusivePtr<AbstractCommand0<void> > relinkingDialogRequester();
	
	void switchToNewProject(
		IntrusivePtr<ProjectPages> const& pages,
		QString const& out_dir,
		QString const& project_file_path = QString(),
		ProjectReader const* project_reader = 0);
	
	IntrusivePtr<ThumbnailPixmapCache> createThumbnailCache();
	
	void setupThumbView();
	
	void showNewOpenProjectPanel();
	
	SavePromptResult promptProjectSave();
	
	static bool compareFiles(QString const& fpath1, QString const& fpath2);
	
	IntrusivePtr<PageOrderProvider const> currentPageOrderProvider() const;

	void updateSortOptions();

	void resetThumbSequence(
		IntrusivePtr<PageOrderProvider const> const& page_order_provider);
	
	void removeWidgetsFromLayout(QLayout* layout);
	
	void removeFilterOptionsWidget();
	
	void removeImageWidget();
	
	void updateProjectActions();
	
	bool isBatchProcessingInProgress() const;

	bool isProjectLoaded() const;
	
	bool isBelowSelectContent() const;
	
	bool isBelowSelectContent(int filter_idx) const;

	bool isBelowFixOrientation(int filter_idx) const;
	
	bool isOutputFilter() const;
	
	bool isOutputFilter(int filter_idx) const;
	
	PageView getCurrentView() const;
	
	void updateMainArea();
	
	bool checkReadyForOutput(PageId const* ignore = 0) const;
	
	void loadPageInteractive(PageInfo const& page);
	
	void updateWindowTitle();
	
	bool closeProjectInteractive();
	
	void closeProjectWithoutSaving();
	
	bool saveProjectWithFeedback(QString const& project_file);
	
	void showInsertFileDialog(
		BeforeOrAfter before_or_after, ImageId const& existig);

	void showRemovePagesDialog(std::set<PageId> const& pages);
	
	void insertImage(ImageInfo const& new_image,
		BeforeOrAfter before_or_after, ImageId existing);

	void removeFromProject(std::set<PageId> const& pages);

	void eraseOutputFiles(std::set<PageId> const& pages);
	
	BackgroundTaskPtr createCompositeTask(
		PageInfo const& page, int last_filter_idx, bool batch, bool debug);
	
	IntrusivePtr<CompositeCacheDrivenTask>
	createCompositeCacheDrivenTask(int last_filter_idx);
	
	void createBatchProcessingWidget();

	void updateDisambiguationRecords(PageSequence const& pages);

	void performRelinking(IntrusivePtr<AbstractRelinker> const& relinker);

	PageSelectionAccessor newPageSelectionAccessor();
	
	QSizeF m_maxLogicalThumbSize;
	IntrusivePtr<ProjectPages> m_ptrPages;
	IntrusivePtr<StageSequence> m_ptrStages;
	QString m_projectFile;
	OutputFileNameGenerator m_outFileNameGen;
	IntrusivePtr<ThumbnailPixmapCache> m_ptrThumbnailCache;
	std::auto_ptr<ThumbnailSequence> m_ptrThumbSequence;
	std::auto_ptr<WorkerThread> m_ptrWorkerThread;
	std::auto_ptr<ProcessingTaskQueue> m_ptrBatchQueue;
	std::auto_ptr<ProcessingTaskQueue> m_ptrInteractiveQueue;
	QStackedLayout* m_pImageFrameLayout;
	QStackedLayout* m_pOptionsFrameLayout;
	QPointer<FilterOptionsWidget> m_ptrOptionsWidget;
	QPointer<FixDpiDialog> m_ptrFixDpiDialog;
	std::auto_ptr<TabbedDebugImages> m_ptrTabbedDebugImages;
	std::auto_ptr<ContentBoxPropagator> m_ptrContentBoxPropagator;
	std::auto_ptr<PageOrientationPropagator> m_ptrPageOrientationPropagator;
	std::auto_ptr<QWidget> m_ptrBatchProcessingWidget;
	std::auto_ptr<ProcessingIndicationWidget> m_ptrProcessingIndicationWidget;
	boost::function<bool()> m_checkBeepWhenFinished;
	SelectedPage m_selectedPage;
	QObjectCleanupHandler m_optionsWidgetCleanup;
	QObjectCleanupHandler m_imageWidgetCleanup;
	std::auto_ptr<OutOfMemoryDialog> m_ptrOutOfMemoryDialog;
	int m_curFilter;
	int m_ignoreSelectionChanges;
	int m_ignorePageOrderingChanges;
	bool m_debug;
	bool m_closing;
	bool m_beepOnBatchProcessingCompletion;
//Export_Subscans
	ExportDialog* m_p_export_dialog;
	QVector<QString> m_outpaths_vector;
	int ExportNextFile();
	int m_exportTimerId;
	QString m_export_dir;
	bool m_split_subscans;
	bool m_generate_blank_back_subscans;
	int m_pos_export;
//Original_Foreground_Mixed
	bool m_keep_orig_fore_subscan;
	std::auto_ptr<ThumbnailSequence> m_ptrThumbSequence_export;	
//Auto_Save_Project
	void autoSaveProject();
	bool m_auto_save_project;
//Dont_Equalize_Illumination_Pic_Zones
	bool m_dont_equalize_illumination_pic_zones;
};

#endif
