/*
    Scan Tailor - Interactive post-processing tool for scanned pages.
	Copyright (C)  Vadim Kuznetsov <dikbsd@gmail.com>

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
#include "SettingsManager.h"

#include <QString>

SettingsManager::SettingsManager() :
	m_AutoSave("settings/general/autosave"),
	m_AutoSaveValue("settings/general/autosave_value"),
	m_thresholdLevelValue("settings/output/threshold_level_value"),
	m_thresholdValue("settings/output/threshold_value"),
	m_despeckling("settings/output/despeckling"),
	m_bitonal_compress_g4fax("settings/output/bitonal_compress_g4fax"),
	m_use_3d_acceleration("settings/use_3d_acceleration")
{
}

SettingsManager::~SettingsManager()
{
}

// autosave settings
void
SettingsManager::SetAutoSave (bool as)
{
	setValue(m_AutoSave, as);
}

bool
SettingsManager::GetAutoSave() const
{
	return value(m_AutoSave, false).toBool();
}

void
SettingsManager::SetAutoSaveValue (int value)
{
	setValue(m_AutoSaveValue, value);
}

int
SettingsManager::GetAutoSaveValue() const
{
	return value(m_AutoSaveValue, 5).toInt();
}

// threshold settings
void
SettingsManager::SetThresholdLevelValue (int value)
{
	setValue(m_thresholdLevelValue, value);
}

int
SettingsManager::GetThresholdLevelValue() const
{
	return value(m_thresholdLevelValue, 50).toInt();
}

void
SettingsManager::SetThresholdValue (int value)
{
	setValue(m_thresholdValue, value);
}

int
SettingsManager::GetThresholdValue() const
{
	return value(m_thresholdValue, 0).toInt();
}

// despeckling settings
void
SettingsManager::SetDespeckling(const QString& despeckle)
{
	setValue(m_despeckling, despeckle);
}

QString
SettingsManager::GetDespeckling() const
{
	return value(m_despeckling, "cautious").toString();
}

// bitonal_compress_g4fax settings
void
SettingsManager::SetCompressG4Fax(bool g4fax)
{
	setValue(m_bitonal_compress_g4fax, g4fax);
}

bool
SettingsManager::GetCompressG4Fax() const
{
	return value(m_bitonal_compress_g4fax, false).toBool();
}

// use_3d_acceleration settings
void
SettingsManager::SetUse3dAcceleration(bool u3d)
{
	setValue(m_use_3d_acceleration, u3d);
}

bool
SettingsManager::GetUse3dAcceleration() const
{
	return value(m_use_3d_acceleration, false).toBool();
}
