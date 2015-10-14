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

#include "LoadFilesStatusDialog.h"
#include <QPushButton>
#ifndef Q_MOC_RUN
#include <boost/foreach.hpp>
#endif

LoadFilesStatusDialog::LoadFilesStatusDialog(QWidget* parent)
:	QDialog(parent)
{
	ui.setupUi(this);
	ui.tabWidget->setCurrentWidget(ui.failedTab);
	
	m_loadedTabNameTemplate = ui.tabWidget->tabText(0);
	m_failedTabNameTemplate = ui.tabWidget->tabText(1);

	setLoadedFiles(std::vector<QString>());
	setFailedFiles(std::vector<QString>());
}

void
LoadFilesStatusDialog::setLoadedFiles(std::vector<QString> const& files)
{
	ui.tabWidget->setTabText(0, m_loadedTabNameTemplate.arg(files.size()));

	QString text;
	BOOST_FOREACH(QString const& file, files) {
		text.append(file);
		text.append(QChar('\n'));
	}

	ui.loadedFiles->setPlainText(text);
}

void
LoadFilesStatusDialog::setFailedFiles(std::vector<QString> const& files)
{
	ui.tabWidget->setTabText(1, m_failedTabNameTemplate.arg(files.size()));

	QString text;
	BOOST_FOREACH(QString const& file, files) {
		text.append(file);
		text.append(QChar('\n'));
	}

	ui.failedFiles->setPlainText(text);
}

void
LoadFilesStatusDialog::setOkButtonName(QString const& name)
{
	ui.buttonBox->button(QDialogButtonBox::Ok)->setText(name);
}
