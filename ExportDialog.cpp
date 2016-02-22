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
//begin of modified by monday2000
//Export_Subscans
// This file was added by monday2000

#include "ExportDialog.h"
#include "ExportDialog.h.moc"
#include "OpenGLSupport.h"
#include "config.h"
#include <QSettings>
#include <QVariant>
#include <QDir>
#include <QFileDialog>
#include <QMessageBox>
#include <QTimer>

ExportDialog::ExportDialog(QWidget* parent)
:	QDialog(parent)
{
	ui.setupUi(this);

	QSettings settings;
	
	connect(ui.SplitMixed, SIGNAL(toggled(bool)), this, SLOT(OnCheckSplitMixed(bool)));
	connect(ui.DefaultOutputFolder, SIGNAL(toggled(bool)), this, SLOT(OnCheckDefaultOutputFolder(bool)));
	connect(ui.ExportButton, SIGNAL(clicked()), this, SLOT(OnClickExport()));

	connect(ui.outExportDirBrowseBtn, SIGNAL(clicked()), this, SLOT(outExportDirBrowse()));

	connect(ui.outExportDirLine, SIGNAL(textEdited(QString const&)),
		this, SLOT(outExportDirEdited(QString const&))
	);

	ui.SplitMixed->setChecked(settings.value("settings/split_mixed").toBool());	
	ui.DefaultOutputFolder->setChecked(settings.value("settings/default_output_folder").toBool());	
	ui.labelFilesProcessed->clear();
	ui.ExportButton->setText(tr("Export"));	
	ui.OkButton->setText(tr("Close"));
	ui.tabWidget->setTabText(0, tr("Main"));
	ui.tabWidget->setTabText(1, tr("Rare options"));
	//ui.tabWidget->setCurrentIndex(0);
	//connect(ui.tabWidget, SIGNAL(currentChanged(int)), this, SLOT(tabChanged(int)));

	connect(ui.GenerateBlankBackSubscans, SIGNAL(toggled(bool)), this, SLOT(OnCheckGenerateBlankBackSubscans(bool)));
	connect(ui.KeepOriginalColorIllumForeSubscans, SIGNAL(toggled(bool)), this, SLOT(OnCheckKeepOriginalColorIllumForeSubscans(bool)));

	ui.GenerateBlankBackSubscans->setChecked(settings.value("settings/generate_blank_back_subscans").toBool());
	ui.KeepOriginalColorIllumForeSubscans->setChecked(settings.value("settings/keep_original_color_illum_fore_subscans").toBool());
	//ui.KeepOriginalColorIllumForeSubscans->setVisible(false);
}

ExportDialog::~ExportDialog()
{
}

void
ExportDialog::OnCheckSplitMixed(bool state)
{
	QSettings settings;	

	settings.setValue("settings/split_mixed", state);
}

void
ExportDialog::OnCheckDefaultOutputFolder(bool state)
{
	QSettings settings;

	settings.setValue("settings/default_output_folder", state);
}

void
ExportDialog::OnClickExport()
{
	if (ui.outExportDirLine->text().isEmpty() &&
		!ui.DefaultOutputFolder->isChecked())
	{
		QMessageBox::warning(
			this, tr("Error"),
			tr("The export output directory is empty.")
			);
		return;	
	}

	QDir const out_dir(ui.outExportDirLine->text());

	if (out_dir.isAbsolute() && !out_dir.exists()) {
		// Maybe create it.
		bool create = m_autoOutDir;
		if (!m_autoOutDir) {
			create = QMessageBox::question(
				this, tr("Create Directory?"),
				tr("The export output directory doesn't exist.  Create it?"),
				QMessageBox::Yes|QMessageBox::No
			) == QMessageBox::Yes;
			if (!create) {
				return;
			}
		}
		if (create) {
			if (!out_dir.mkpath(out_dir.path())) {
				QMessageBox::warning(
					this, tr("Error"),
					tr("Unable to create the export output directory.")
				);
				return;
			}
		}
	}
	if ((!out_dir.isAbsolute() || !out_dir.exists())&& !ui.DefaultOutputFolder->isChecked()) {		

		QMessageBox::warning(
			this, tr("Error"),
			tr("The export output directory is not set or doesn't exist.")
		);
		return;
	}

	QString export_dir_path = ui.outExportDirLine->text();
	bool split_subscans = ui.SplitMixed->isChecked();
	bool default_out_dir = ui.DefaultOutputFolder->isChecked();

	if (ui.ExportButton->text() != tr("Stop"))	
		emit SetStartExportSignal();	
	
	else			
		emit ExportStopSignal();		
}

void
ExportDialog::outExportDirBrowse()
{
	QString initial_dir(ui.outExportDirLine->text());
	if (initial_dir.isEmpty() || !QDir(initial_dir).exists()) {
		initial_dir = QDir::home().absolutePath();
	}
	
	QString const dir(
		QFileDialog::getExistingDirectory(
			this, tr("Export output directory"), initial_dir
		)
	);
	
	if (!dir.isEmpty()) {
		setExportOutputDir(dir);
	}
}

void
ExportDialog::setExportOutputDir(QString const& dir)
{
	ui.outExportDirLine->setText(QDir::toNativeSeparators(dir));
}

void
ExportDialog::outExportDirEdited(QString const& text)
{
	m_autoOutDir = false;
}

void
ExportDialog::setCount(int count)
{
	m_count = count;
	ui.progressBar->setMaximum(m_count);
}

void
ExportDialog::StepProgress()
{
	ui.labelFilesProcessed->setText(tr("Processed file") + " " + QString::number(ui.progressBar->value()+1) + " " + tr("of") + " " + QString::number(m_count));
	ui.progressBar->setValue(ui.progressBar->value() + 1);	
}

void
ExportDialog::startExport(void)
{
	QString export_dir_path = ui.outExportDirLine->text();
	bool split_subscans = ui.SplitMixed->isChecked();
	bool default_out_dir = ui.DefaultOutputFolder->isChecked();
	bool generate_blank_back_subscans = ui.GenerateBlankBackSubscans->isChecked();
	bool keep_original_color_illum_fore_subscans = ui.KeepOriginalColorIllumForeSubscans->isChecked();

	emit ExportOutputSignal(export_dir_path, default_out_dir, split_subscans, 
		generate_blank_back_subscans, keep_original_color_illum_fore_subscans);
}

void
ExportDialog::reset(void)
{
	ui.labelFilesProcessed->clear();
	ui.progressBar->setValue(0);	
	ui.ExportButton->setText(tr("Export"));
}

void
ExportDialog::setExportLabel(void)
{
	ui.ExportButton->setText(tr("Export"));
}

void
ExportDialog::setStartExport(void)
{
	m_count = 0;

	ui.progressBar->setValue(0);
	ui.labelFilesProcessed->setText(tr("Starting the export..."));		
	ui.ExportButton->setText(tr("Stop"));	

	QTimer::singleShot(1, this, SLOT(startExport()));
}


//void
//ExportDialog::tabChanged(int const tab)
//{
//
//}
//end of modified by monday2000


void
ExportDialog::OnCheckGenerateBlankBackSubscans(bool state)
{
	QSettings settings;	

	settings.setValue("settings/generate_blank_back_subscans", state);
}

void
ExportDialog::OnCheckKeepOriginalColorIllumForeSubscans(bool state)
{		
	QSettings settings;	

	settings.setValue("settings/keep_original_color_illum_fore_subscans", state);
}