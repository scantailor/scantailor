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
#ifndef SETTINGS_MANAGER_H_
#define SETTINGS_MANAGER_H_

#include "foundation/NonCopyable.h"

#include <QSettings>

class QString;

class SettingsManager : public QSettings
{
	DECLARE_NON_COPYABLE(SettingsManager)
public:
	SettingsManager();
	virtual ~SettingsManager();
	
	// despeckling settings
	void SetDespeckling(const QString&);
	QString GetDespeckling();
	
	// bitonal_compress_g4fax settings
	void SetCompressG4Fax(bool);
	bool GetCompressG4Fax();
	
	// use_3d_acceleration settings
	void SetUse3dAcceleration(bool);
	bool GetUse3dAcceleration();
	
private:
	QString m_despeckling;
	QString m_bitonal_compress_g4fax;
	QString m_use_3d_acceleration;
};

#endif // SETTINGS_MANAGER_H_
