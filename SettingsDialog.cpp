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

#include "SettingsDialog.h"
#include "SettingsDialog.h.moc"
#include "OpenGLSupport.h"
#include "config.h"
#include <QSettings>
#include <QVariant>

SettingsDialog::SettingsDialog(QWidget* parent)
:	QDialog(parent)
{
	ui.setupUi(this);

	QSettings settings;

#ifndef ENABLE_OPENGL
	ui.use3DAcceleration->setChecked(false);
	ui.use3DAcceleration->setEnabled(false);
	ui.use3DAcceleration->setToolTip(tr("Compiled without OpenGL support."));
#else
	if (!OpenGLSupport::supported()) {
		ui.use3DAcceleration->setChecked(false);
		ui.use3DAcceleration->setEnabled(false);
		ui.use3DAcceleration->setToolTip(tr("Your hardware / driver don't provide the necessary features."));
	} else {
		ui.use3DAcceleration->setChecked(
			settings.value("settings/use_3d_acceleration", false).toBool()
		);
	}
#endif

	connect(ui.buttonBox, SIGNAL(accepted()), SLOT(commitChanges()));
//begin of modified by monday2000
//Auto_Save_Project
	ui.AutoSaveProject->setChecked(settings.value("settings/auto_save_project").toBool());
	connect(ui.AutoSaveProject, SIGNAL(toggled(bool)), this, SLOT(OnCheckAutoSaveProject(bool)));
//Dont_Equalize_Illumination_Pic_Zones
	ui.DontEqualizeIlluminationPicZones->setChecked(settings.value("settings/dont_equalize_illumination_pic_zones").toBool());
	connect(ui.DontEqualizeIlluminationPicZones, SIGNAL(toggled(bool)), this, SLOT(OnCheckDontEqualizeIlluminationPicZones(bool)));
//end of modified by monday2000
}

SettingsDialog::~SettingsDialog()
{
}

void
SettingsDialog::commitChanges()
{
	QSettings settings;
#ifdef ENABLE_OPENGL
	settings.setValue("settings/use_3d_acceleration", ui.use3DAcceleration->isChecked());
#endif
}

//begin of modified by monday2000
//Auto_Save_Project
void
SettingsDialog::OnCheckAutoSaveProject(bool state)
{
	QSettings settings;	

	settings.setValue("settings/auto_save_project", state);

	emit AutoSaveProjectStateSignal(state);
}

//Dont_Equalize_Illumination_Pic_Zones
void
SettingsDialog::OnCheckDontEqualizeIlluminationPicZones(bool state)
{
	QSettings settings;	

	settings.setValue("settings/dont_equalize_illumination_pic_zones", state);

	emit DontEqualizeIlluminationPicZonesSignal(state);
}
//end of modified by monday2000