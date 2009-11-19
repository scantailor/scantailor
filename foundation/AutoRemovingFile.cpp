/*
	Scan Tailor - Interactive post-processing tool for scanned pages.
	Copyright (C) Joseph Artsimovich <joseph_a@mail.ru>

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

#include "AutoRemovingFile.h"
#include <QFile>

AutoRemovingFile::AutoRemovingFile()
{
}

AutoRemovingFile::AutoRemovingFile(QString const& file_path)
:	m_file(file_path)
{
}

AutoRemovingFile::AutoRemovingFile(AutoRemovingFile& other)
:	m_file(other.release())
{
}

AutoRemovingFile::AutoRemovingFile(CopyHelper other)
:	m_file(other.obj->release())
{
}

AutoRemovingFile::~AutoRemovingFile()
{
	if (!m_file.isEmpty()) {
		QFile::remove(m_file);
	}
}

AutoRemovingFile&
AutoRemovingFile::operator=(AutoRemovingFile& other)
{
	m_file = other.release();
	return *this;
}

AutoRemovingFile&
AutoRemovingFile::operator=(CopyHelper other)
{
	m_file = other.obj->release();
	return *this;
}

void
AutoRemovingFile::reset(QString const& file)
{
	QString const old_file(file);

	m_file = file;

	if (!old_file.isEmpty()) {
		QFile::remove(old_file);
	}
}

QString
AutoRemovingFile::release()
{
	QString saved(m_file);
	m_file = QString();
	return saved;
}
