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

#include "ProjectCreationContext.h.moc"
#include "ProjectFilesDialog.h"
#include "FixDpiDialog.h"
#include "ImageFileInfo.h"
#include <QString>
#include <Qt>
#ifndef Q_MOC_RUN
#include <boost/lambda/lambda.hpp>
#include <boost/lambda/bind.hpp>
#endif
#include <algorithm>
#include <assert.h>

ProjectCreationContext::ProjectCreationContext(QWidget* parent)
:	m_layoutDirection(Qt::LeftToRight),
	m_pParent(parent)
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
bool allDpisOK(T const& container)
{
	using namespace boost::lambda;
	
	return std::find_if(
		container.begin(), container.end(),
		!bind(&ImageFileInfo::isDpiOK, _1)
	) == container.end();
}

} // anonymous namespace

void
ProjectCreationContext::projectFilesSubmitted()
{
	m_files = m_ptrProjectFilesDialog->inProjectFiles();
	m_outDir = m_ptrProjectFilesDialog->outputDirectory();
	m_layoutDirection = Qt::LeftToRight;
	if (m_ptrProjectFilesDialog->isRtlLayout()) {
		m_layoutDirection = Qt::RightToLeft;
	}
	
	if (!m_ptrProjectFilesDialog->isDpiFixingForced() && allDpisOK(m_files)) {
		emit done(this);
	} else {
		showFixDpiDialog();
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
	m_files = m_ptrFixDpiDialog->files();
	emit done(this);
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
	m_ptrProjectFilesDialog = new ProjectFilesDialog(m_pParent);
	m_ptrProjectFilesDialog->setAttribute(Qt::WA_DeleteOnClose);
	m_ptrProjectFilesDialog->setAttribute(Qt::WA_QuitOnClose, false);
	if (m_pParent) {
		m_ptrProjectFilesDialog->setWindowModality(Qt::WindowModal);
	}
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
ProjectCreationContext::showFixDpiDialog()
{
	assert(!m_ptrFixDpiDialog);
	m_ptrFixDpiDialog = new FixDpiDialog(m_files, m_pParent);
	m_ptrFixDpiDialog->setAttribute(Qt::WA_DeleteOnClose);
	m_ptrFixDpiDialog->setAttribute(Qt::WA_QuitOnClose, false);
	if (m_pParent) {
		m_ptrFixDpiDialog->setWindowModality(Qt::WindowModal);
	}
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


