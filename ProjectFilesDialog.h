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

#ifndef PROJECTFILESDIALOG_H_
#define PROJECTFILESDIALOG_H_

#include "ui_ProjectFilesDialog.h"
#include "ImageFileInfo.h"
#include <QDialog>
#include <QString>
#include <QSet>
#include <vector>
#include <memory>

class ProjectFilesDialog : public QDialog, private Ui::ProjectFilesDialog
{
	Q_OBJECT
public:
	ProjectFilesDialog(QWidget* parent = 0);
	
	virtual ~ProjectFilesDialog();
	
	QString inputDirectory() const;
	
	QString outputDirectory() const;
	
	std::vector<ImageFileInfo> inProjectFiles() const;
	
	bool isRtlLayout() const;
	
	bool isDpiFixingForced() const;
private slots:
	static QString sanitizePath(QString const& path);
	
	void inpDirBrowse();
	
	void outDirBrowse();
	
	void inpDirEdited(QString const& text);
	
	void outDirEdited(QString const& text);
	
	void addToProject();
	
	void removeFromProject();
	
	void onOK();
private:
	class Item;
	class FileList;
	class SortedFileList;
	class ItemVisualOrdering;
	
	void setInputDir(QString const& dir, bool auto_add_files = true);
	
	void setOutputDir(QString const& dir);
	
	void startLoadingMetadata();
	
	virtual void timerEvent(QTimerEvent* event);
	
	void finishLoadingMetadata();
	
	QSet<QString> m_supportedExtensions;
	std::auto_ptr<FileList> m_ptrOffProjectFiles;
	std::auto_ptr<SortedFileList> m_ptrOffProjectFilesSorted;
	std::auto_ptr<FileList> m_ptrInProjectFiles;
	std::auto_ptr<SortedFileList> m_ptrInProjectFilesSorted;
	int m_loadTimerId;
	bool m_metadataLoadFailed;
	bool m_autoOutDir;
};

#endif
