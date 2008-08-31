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

#include "Utils.h"
#include <QString>
#include <QByteArray>
#include <QFile>

#ifdef Q_WS_WIN
#include <windows.h>
#else
#include <stdio.h>
#endif

bool
Utils::overwritingRename(QString const& from, QString const& to)
{
#ifdef Q_WS_WIN
	return MoveFileExW(
		(WCHAR*)from.utf16(), (WCHAR*)to.utf16(),
		MOVEFILE_REPLACE_EXISTING
	) != 0;
#else
	return rename(
		QFile::encodeName(from).data(),
		QFile::encodeName(to).data()
	) == 0;
#endif
}
