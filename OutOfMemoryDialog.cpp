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

#include "OutOfMemoryDialog.h"
#include "OutOfMemoryDialog.h.moc"
#include "ProjectWriter.h"
#include "RecentProjects.h"
#include <QFileDialog>
#include <QMessageBox>
#include <QFileInfo>
#include <QSettings>
#include <QVariant>

OutOfMemoryDialog::OutOfMemoryDialog(QWidget* parent)
:	QDialog(parent)
{
	ui.setupUi(this);
	if (sizeof(void*) > 32) {
		ui.only_32bit_1->hide();
		ui.only_32bit_2->hide();
	}

	ui.topLevelStack->setCurrentWidget(ui.mainPage);

	connect(ui.saveProjectBtn, SIGNAL(clicked()), SLOT(saveProject()));
	connect(ui.saveProjectAsBtn, SIGNAL(clicked()), SLOT(saveProjectAs()));
	connect(ui.dontSaveBtn, SIGNAL(clicked()), SLOT(reject()));
}

void
OutOfMemoryDialog::setParams(
	QString const& project_file,
	IntrusivePtr<StageSequence> const& stages,
	IntrusivePtr<ProjectPages> const& pages,
	SelectedPage const& selected_page,
	OutputFileNameGenerator const& out_file_name_gen)
{
	m_projectFile = project_file;
	m_ptrStages = stages;
	m_ptrPages = pages;
	m_selectedPage = selected_page;
	m_outFileNameGen = out_file_name_gen;

	ui.saveProjectBtn->setVisible(!project_file.isEmpty());
}

void
OutOfMemoryDialog::saveProject()
{
	if (m_projectFile.isEmpty()) {
		saveProjectAs();
	} else if (saveProjectWithFeedback(m_projectFile)) {
		showSaveSuccessScreen();
	}
}

void
OutOfMemoryDialog::saveProjectAs()
{
	// XXX: this function is duplicated MainWindow

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
		showSaveSuccessScreen();

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

bool
OutOfMemoryDialog::saveProjectWithFeedback(QString const& project_file)
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

void
OutOfMemoryDialog::showSaveSuccessScreen()
{
	ui.topLevelStack->setCurrentWidget(ui.saveSuccessPage);
}
