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

#include "ProjectOpeningContext.h.moc"
#include "ProjectReader.h"
#include "MainWindow.h"
#include <QFileDialog>
#include <QMessageBox>
#include <QFile>
#include <QDir>
#include <QString>
#include <QSettings>
#include <QDomDocument>

ProjectOpeningContext::ProjectOpeningContext()
{
}

ProjectOpeningContext::~ProjectOpeningContext()
{
}

void
ProjectOpeningContext::openProject()
{
	deleteLater();
	openProjectImpl();
}

void
ProjectOpeningContext::openProjectImpl()
{
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
		return;
	}
	
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
	
	MainWindow* window = new MainWindow(project_file, reader);
	window->setAttribute(Qt::WA_DeleteOnClose);
	window->showMaximized();
}
