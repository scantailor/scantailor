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

#include "ProjectOpeningContext.h"
#include "ProjectOpeningContext.h.moc"
#include "FixDpiDialog.h"
#include "ProjectPages.h"
#include <QString>
#include <QMessageBox>
#include <Qt>
#include <boost/lambda/lambda.hpp>
#include <boost/lambda/bind.hpp>
#include <algorithm>
#include <assert.h>

ProjectOpeningContext::ProjectOpeningContext(
	QWidget* parent, QString const& project_file, QDomDocument const& doc)
:	m_projectFile(project_file),
	m_reader(doc),
	m_pParent(parent)
{
}

ProjectOpeningContext::~ProjectOpeningContext()
{
	// Deleting a null pointer is OK.
	delete m_ptrFixDpiDialog;
}

void
ProjectOpeningContext::proceed()
{
	if (!m_reader.success()) {
		deleteLater();
		QMessageBox::warning(
			m_pParent, tr("Error"),
			tr("Unable to interpret the project file.")
		);
		return;
	}
	
	if (m_reader.pages()->validateDpis()) {
		deleteLater();
		emit done(this);
		return;
	}
	
	showFixDpiDialog();
}

void
ProjectOpeningContext::fixedDpiSubmitted()
{
	m_reader.pages()->updateMetadataFrom(m_ptrFixDpiDialog->files());
	emit done(this);
}

void
ProjectOpeningContext::fixDpiDialogDestroyed()
{
	deleteLater();
}

void
ProjectOpeningContext::showFixDpiDialog()
{
	assert(!m_ptrFixDpiDialog);
	m_ptrFixDpiDialog = new FixDpiDialog(m_reader.pages()->toImageFileInfo(), m_pParent);
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
