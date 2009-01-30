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

#ifndef MAINWINDOW_H_
#define MAINWINDOW_H_

#include "ui_MainWindow.h"
#include "FilterUiInterface.h"
#include "NonCopyable.h"
#include "IntrusivePtr.h"
#include "BackgroundTask.h"
#include "FilterResult.h"
#include "PageSequence.h"
#include <QMainWindow>
#include <QString>
#include <QPointer>
#include <QObjectCleanupHandler>
#include <QSizeF>
#include <memory>

class AbstractFilter;
class ThumbnailPixmapCache;
class ThumbnailSequence;
class FilterOptionsWidget;
class PageInfo;
class QStackedLayout;
class WorkerThread;
class ProjectReader;
class DebugImages;
class ContentBoxPropagator;
class ProjectCreationContext;
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
protected:
	virtual void closeEvent(QCloseEvent* event);
	
	virtual void timerEvent(QTimerEvent* event);
private slots:
	void nextPage();
	
	void prevPage();
	
	void goToPage(PageId const& page_id);
	
	void pageSelected(
		PageInfo const& page_info, QRectF const& thumb_rect,
		bool by_user, bool was_already_selected);
	
	void thumbViewFocusToggled(bool checked);
	
	void thumbViewScrolled();
	
	void filterSelectionChanged(QItemSelection const& selected);
	
	void reloadRequested();
	
	void startBatchProcessing();
	
	void stopBatchProcessing();
	
	void invalidateThumbnailSlot(PageId const& page_id);
	
	void invalidateAllThumbnailsSlot();
	
	void filterResult(
		BackgroundTaskPtr const& task,
		FilterResultPtr const& result);
	
	void debugToggled(bool enabled);
	
	void saveProjectTriggered();
	
	void saveProjectAsTriggered();
	
	void newProject();
	
	void newProjectCreated(ProjectCreationContext* context);
	
	void openProject();
	
	void openProject(QString const& project_file);
	
	void closeProject();
private:
	class FilterListModel;
	
	enum SavePromptResult { SAVE, DONT_SAVE, CANCEL };
	
	typedef IntrusivePtr<AbstractFilter> FilterPtr;
	
	virtual void setOptionsWidget(
		FilterOptionsWidget* widget, Ownership ownership);
	
	virtual void setImageWidget(
		QWidget* widget, Ownership ownership,
		DebugImages* debug_images = 0);
	
	virtual void invalidateThumbnail(PageId const& page_id);
	
	virtual void invalidateAllThumbnails();
	
	void cancelOngoingTask();
	
	void switchToNewProject(
		IntrusivePtr<PageSequence> const& pages,
		QString const& out_dir,
		QString const& project_file_path = QString(),
		ProjectReader const* project_reader = 0);
	
	std::auto_ptr<ThumbnailPixmapCache> createThumbnailCache();
	
	void setupThumbView();
	
	void showNewOpenProjectPanel();
	
	SavePromptResult promptProjectSave();
	
	static bool compareFiles(QString const& fpath1, QString const& fpath2);
	
	void resetThumbSequence();
	
	void removeWidgetsFromLayout(QLayout* layout, bool delete_widgets);
	
	void updateProjectActions();
	
	void updateBatchProcessingActions();
	
	bool isProjectLoaded() const { return !m_outDir.isEmpty(); }
	
	bool isBelowSelectContent() const;
	
	bool isBelowSelectContent(int filter_idx) const;
	
	bool isOutputFilter() const;
	
	bool isOutputFilter(int filter_idx) const;
	
	PageSequence::View getCurrentView() const;
	
	void updateMainArea();
	
	void loadImage(PageInfo const& page, int page_num);
	
	void updateWindowTitle();
	
	bool closeProjectInteractive();
	
	void closeProjectWithoutSaving();
	
	bool saveProjectWithFeedback(QString const& project_file);
	
	QSizeF m_maxLogicalThumbSize;
	IntrusivePtr<PageSequence> m_ptrPages;
	QString m_outDir;
	QString m_projectFile;
	std::auto_ptr<ThumbnailPixmapCache> m_ptrThumbnailCache;
	std::auto_ptr<ThumbnailSequence> m_ptrThumbSequence;
	std::auto_ptr<WorkerThread> m_ptrWorkerThread;
	QStackedLayout* m_pImageFrameLayout;
	QStackedLayout* m_pOptionsFrameLayout;
	QPointer<FilterOptionsWidget> m_ptrOptionsWidget;
	std::auto_ptr<QTabWidget> m_ptrTabbedDebugImages;
	std::auto_ptr<FilterListModel> m_ptrFilterListModel;
	std::auto_ptr<ContentBoxPropagator> m_ptrContentBoxPropagator;
	BackgroundTaskPtr m_ptrCurTask;
	QObjectCleanupHandler m_optionsWidgetCleanup;
	QObjectCleanupHandler m_imageWidgetCleanup;
	int m_curFilter;
	int m_ignoreSelectionChanges;
	bool m_debug;
	bool m_batchProcessing;
	bool m_closing;
};

#endif
