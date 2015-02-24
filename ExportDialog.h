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
//begin of modified by monday2000
//Export_Subscans
// This file was added by monday2000

#ifndef EXPORT_DIALOG_H_
#define EXPORT_DIALOG_H_

#include "ui_ExportDialog.h"
#include <QDialog>

class ExportDialog : public QDialog
{
	Q_OBJECT
public:
	ExportDialog(QWidget* parent = 0);

	virtual ~ExportDialog();

	void setCount(int count);
	void StepProgress();
	void reset(void);
	void setExportLabel(void);
	void setStartExport(void);	

signals:
	void ExportOutputSignal(QString export_dir_path, bool default_out_dir, bool split_subscans,
		bool generate_blank_back_subscans, bool keep_original_color_illum_fore_subscans);
	void ExportStopSignal();
	void SetStartExportSignal();	

public slots:	
	void startExport(void);

private slots:	
	void OnCheckSplitMixed(bool);
	void OnCheckDefaultOutputFolder(bool);
	void OnClickExport();
	void outExportDirBrowse();
	void outExportDirEdited(QString const&);
	//void tabChanged(int tab);	
	void OnCheckGenerateBlankBackSubscans(bool);
	void OnCheckKeepOriginalColorIllumForeSubscans(bool);

private:
	Ui::ExportDialog ui;

	bool m_autoOutDir;
	void setExportOutputDir(QString const& dir);
	int m_count;	
};

#endif
//end of modified by monday2000