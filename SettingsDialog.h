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

#ifndef SETTINGS_DIALOG_H_
#define SETTINGS_DIALOG_H_

#include "ui_SettingsDialog.h"
#include <QDialog>

class SettingsDialog : public QDialog
{
	Q_OBJECT
public:
	SettingsDialog(QWidget* parent = 0);
	
	virtual ~SettingsDialog();
//begin of modified by monday2000
signals:
//Auto_Save_Project
	void AutoSaveProjectStateSignal(bool auto_save);
//Dont_Equalize_Illumination_Pic_Zones
	void DontEqualizeIlluminationPicZonesSignal(bool state);
//end of modified by monday2000
private slots:
	void commitChanges();
//begin of modified by monday2000
//Auto_Save_Project
	void OnCheckAutoSaveProject(bool);
//Dont_Equalize_Illumination_Pic_Zones
	void OnCheckDontEqualizeIlluminationPicZones(bool);
//end of modified by monday2000
private:
	Ui::SettingsDialog ui;
};

#endif
