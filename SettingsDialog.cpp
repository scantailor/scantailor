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
#include "Utils.h"

#include <QVariant>
#include <QToolButton>
#include <QToolTip>
#include <QString>
#include <QPoint>

SettingsDialog::SettingsDialog(QWidget* parent)
:	QDialog(parent)
{
	ui.setupUi(this);
	ui.autosaveSpinBox->setEnabled(false);

	SettingsManager sm;
	
	// autosave settings
	ui.autosaveCheckBox->setChecked(sm.GetAutoSave());
	ui.autosaveSpinBox->setValue(sm.GetAutoSaveValue());
	
	// threshold settings
	connect(
		ui.thresholdLevelSpinBox, SIGNAL(valueChanged(int)),
		this, SLOT(bwThresholdLevelChanged())
	);
	ui.darkerThresholdLink->setText(
		Utils::richTextForLink(ui.darkerThresholdLink->text())
	);
	ui.lighterThresholdLink->setText(
		Utils::richTextForLink(ui.lighterThresholdLink->text())
	);
	ui.thresholdSlider->setToolTip(QString::number(ui.thresholdSlider->value()));
	ui.thresholdLabel->setText(QString::number(ui.thresholdSlider->value()));
	connect(
		ui.lighterThresholdLink, SIGNAL(linkActivated(QString const&)),
		this, SLOT(setLighterThreshold())
	);
	connect(
		ui.darkerThresholdLink, SIGNAL(linkActivated(QString const&)),
		this, SLOT(setDarkerThreshold())
	);
	connect(
		ui.neutralThresholdBtn, SIGNAL(clicked()),
		this, SLOT(setNeutralThreshold())
	);
	connect(
		ui.thresholdSlider, SIGNAL(valueChanged(int)),
		this, SLOT(bwThresholdChanged())
	);
	connect(
		ui.thresholdSlider, SIGNAL(sliderReleased()),
		this, SLOT(bwThresholdChanged())
	);
	bwThresholdLevelChanged();
	
	ui.thresholdLevelSpinBox->setValue(sm.GetThresholdLevelValue());
	ui.thresholdSlider->setValue(sm.GetThresholdValue());
	
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
	
	// autosave settings
	sm.SetAutoSave(ui.autosaveCheckBox->isChecked());
	sm.SetAutoSaveValue(ui.autosaveSpinBox->value());
	stopAutoSaveTimer();
	if(ui.autosaveCheckBox->isChecked()) {
		startAutoSaveTimer();
	}
	
	// threshold settings
	int range = sm.GetThresholdLevelValue();
	sm.SetThresholdLevelValue(ui.thresholdLevelSpinBox->value());
	sm.SetThresholdValue(ui.thresholdSlider->value());
	
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
	
	if(ui.thresholdLevelSpinBox->value()!=range) {
		updateUIThresholdSlider();
	}
}

void
SettingsDialog::setLighterThreshold()
{
	ui.thresholdSlider->setValue(ui.thresholdSlider->value() - 1);
}

void
SettingsDialog::setDarkerThreshold()
{
	ui.thresholdSlider->setValue(ui.thresholdSlider->value() + 1);
}

void
SettingsDialog::setNeutralThreshold()
{
	ui.thresholdSlider->setValue(0);
}

void
SettingsDialog::bwThresholdChanged()
{
	int const value = ui.thresholdSlider->value();
	QString const tooltip_text(QString::number(value));
	ui.thresholdSlider->setToolTip(tooltip_text);
	
	ui.thresholdLabel->setText(QString::number(value));
	
	// Show the tooltip immediately.
	QPoint const center(ui.thresholdSlider->rect().center());
	QPoint tooltip_pos(ui.thresholdSlider->mapFromGlobal(QCursor::pos()));
	tooltip_pos.setY(center.y());
	tooltip_pos.setX(qBound(0, tooltip_pos.x(), ui.thresholdSlider->width()));
	tooltip_pos = ui.thresholdSlider->mapToGlobal(tooltip_pos);
	QToolTip::showText(tooltip_pos, tooltip_text, ui.thresholdSlider);
	
	if (ui.thresholdSlider->isSliderDown()) {
		// Wait for it to be released.
		// We could have just disabled tracking, but in that case we wouldn't
		// be able to show tooltips with a precise value.
		return;
	}
}

void
SettingsDialog::bwThresholdLevelChanged()
{
	ui.thresholdSlider->setMinimum(-ui.thresholdLevelSpinBox->value());
	ui.thresholdSlider->setMaximum(ui.thresholdLevelSpinBox->value());
}
