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

#ifndef PROJECTCREATIONCONTEXT_H_
#define PROJECTCREATIONCONTEXT_H_

#include "NonCopyable.h"
#include "ImageFileInfo.h"
#include <QObject>
#include <QPointer>
#include <QString>
#include <Qt>
#include <vector>

class ProjectFilesDialog;
class FixDpiDialog;
class QWidget;

class ProjectCreationContext : public QObject
{
	Q_OBJECT
	DECLARE_NON_COPYABLE(ProjectCreationContext)
public:
	ProjectCreationContext(QWidget* parent);
	
	virtual ~ProjectCreationContext();
	
	std::vector<ImageFileInfo> const& files() const { return m_files; }
	
	QString const& outDir() const { return m_outDir; }
	
	Qt::LayoutDirection layoutDirection() const { return m_layoutDirection; }
signals:
	void done(ProjectCreationContext* context);
private slots:
	void projectFilesSubmitted();
	
	void projectFilesDialogDestroyed();
	
	void fixedDpiSubmitted();
	
	void fixDpiDialogDestroyed();
private:
	void showProjectFilesDialog();
	
	void showFixDpiDialog();
	
	QPointer<ProjectFilesDialog> m_ptrProjectFilesDialog;
	QPointer<FixDpiDialog> m_ptrFixDpiDialog;
	QString m_outDir;
	std::vector<ImageFileInfo> m_files;
	Qt::LayoutDirection m_layoutDirection;
	QWidget* m_pParent;
};

#endif
