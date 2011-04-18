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
#include "SettingsManager.h"

#include <QVariant>
#include <QToolButton>

SettingsDialog::SettingsDialog(QWidget* parent)
:	QDialog(parent)
{
	ui.setupUi(this);

	SettingsManager sm;
	// bitonal_compress_g4fax settings
	ui.gfax4RadioButton->setChecked(sm.GetCompressG4Fax());
	// despeckling settings
	QString despeckle = sm.GetDespeckling();
	ui.despeckleCautiousBtn->setChecked(true);
	if(despeckle=="off") {
		ui.despeckleOffBtn->setChecked(true);
	} else if(despeckle=="cautious") {
		ui.despeckleCautiousBtn->setChecked(true);
	} else if (despeckle=="normal") {
		ui.despeckleNormalBtn->setChecked(true);
	} else if(despeckle=="aggressive") {
		ui.despeckleAggressiveBtn->setChecked(true);
	}
	
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
			sm.GetUse3dAcceleration()
		);
	}
#endif

	connect(ui.buttonBox, SIGNAL(accepted()), SLOT(commitChanges()));
}

SettingsDialog::~SettingsDialog()
{
}

void
SettingsDialog::commitChanges()
{
	SettingsManager sm;
#ifdef ENABLE_OPENGL
	sm.SetUse3dAcceleration(ui.use3DAcceleration->isChecked());
#endif
	// bitonal_compress_g4fax settings
	sm.SetCompressG4Fax(ui.gfax4RadioButton->isChecked());
	// despeckling settings
	QString despeckle = "cautious";
	if(ui.despeckleOffBtn->isChecked()) {
		despeckle = "off";
	} else if(ui.despeckleCautiousBtn->isChecked()) {
		despeckle = "cautious";
	} else if(ui.despeckleNormalBtn->isChecked()) {
		despeckle = "normal";
	} else if(ui.despeckleAggressiveBtn->isChecked()) {
		despeckle = "aggressive";
	}
	sm.SetDespeckling(despeckle);
}
