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

#include "ProjectCreationContext.h.moc"
#include "ProjectFilesDialog.h"
#include "FixDpiDialog.h"
#include "MainWindow.h"
#include "ImageFileInfo.h"
#include <QString>
#include <boost/lambda/lambda.hpp>
#include <boost/lambda/bind.hpp>
#include <algorithm>
#include <assert.h>

ProjectCreationContext::ProjectCreationContext()
{
	showProjectFilesDialog();
}

ProjectCreationContext::~ProjectCreationContext()
{
	// Deleting a null pointer is OK.
	delete m_ptrProjectFilesDialog;
	delete m_ptrFixDpiDialog;
}

namespace
{

template<typename T>
bool haveUndefinedDpi(T const& container)
{
	using namespace boost::lambda;
	
	return std::find_if(
		container.begin(), container.end(),
		bind(&ImageFileInfo::haveUndefinedDpi, _1)
	) != container.end();
}

} // anonymous namespace

void
ProjectCreationContext::projectFilesSubmitted()
{
	std::vector<ImageFileInfo> const files(
		m_ptrProjectFilesDialog->inProjectFiles()
	);
	m_outDir = m_ptrProjectFilesDialog->outputDirectory();
	
	if (!haveUndefinedDpi(files)) {
		showMainWindow(files);
	} else {
		showFixDpiDialog(files);
	}
}

void
ProjectCreationContext::projectFilesDialogDestroyed()
{
	if (!m_ptrFixDpiDialog) {
		deleteLater();
	}
}

void
ProjectCreationContext::fixedDpiSubmitted()
{
	showMainWindow(m_ptrFixDpiDialog->files());
}

void
ProjectCreationContext::fixDpiDialogDestroyed()
{
	deleteLater();
}

void
ProjectCreationContext::showProjectFilesDialog()
{
	assert(!m_ptrProjectFilesDialog);
	m_ptrProjectFilesDialog = new ProjectFilesDialog();
	m_ptrProjectFilesDialog->setAttribute(Qt::WA_DeleteOnClose);
	m_ptrProjectFilesDialog->setAttribute(Qt::WA_QuitOnClose, false);
	connect(
		m_ptrProjectFilesDialog, SIGNAL(accepted()),
		this, SLOT(projectFilesSubmitted())
	);
	connect(
		m_ptrProjectFilesDialog, SIGNAL(destroyed(QObject*)),
		this, SLOT(projectFilesDialogDestroyed())
	);
	m_ptrProjectFilesDialog->show();
}

void
ProjectCreationContext::showFixDpiDialog(
	std::vector<ImageFileInfo> const& files)
{
	assert(!m_ptrFixDpiDialog);
	m_ptrFixDpiDialog = new FixDpiDialog(files);
	m_ptrFixDpiDialog->setAttribute(Qt::WA_DeleteOnClose);
	m_ptrFixDpiDialog->setAttribute(Qt::WA_QuitOnClose, false);
	connect(
		m_ptrFixDpiDialog, SIGNAL(accepted()),
		this, SLOT(fixedDpiSubmitted())
	);
	connect(
		m_ptrFixDpiDialog, SIGNAL(destroyed(QObject*)),
		this, SLOT(fixDpiDialogDestroyed())
	);
	m_ptrFixDpiDialog->show();
}

void
ProjectCreationContext::showMainWindow(
	std::vector<ImageFileInfo> const& files)
{
	MainWindow* window = new MainWindow(files, m_outDir);
	window->setAttribute(Qt::WA_DeleteOnClose);
	window->showMaximized();
	deleteLater();
}

