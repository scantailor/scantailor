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

#include "AtomicFileOverwriter.h"
#include "Utils.h"
#include <QString>
#include <QFile>
#include <QTemporaryFile>

AtomicFileOverwriter::AtomicFileOverwriter()
{
}

AtomicFileOverwriter::~AtomicFileOverwriter()
{
	abort();
}

QIODevice*
AtomicFileOverwriter::startWriting(QString const& file_path)
{
	abort();
	
	m_ptrTempFile.reset(new QTemporaryFile(file_path));
	m_ptrTempFile->setAutoRemove(false);
	if (!m_ptrTempFile->open()) {
		m_ptrTempFile.reset();
	}
	
	return m_ptrTempFile.get();
}

bool
AtomicFileOverwriter::commit()
{
	if (!m_ptrTempFile.get()) {
		return false;
	}
	
	QString const temp_file_path(m_ptrTempFile->fileName());
	QString const target_path(m_ptrTempFile->fileTemplate());
	
	// Yes, we have to destroy this object here, because:
	// 1. Under Windows, open files can't be renamed or deleted.
	// 2. QTemporaryFile::close() doesn't really close it.
	m_ptrTempFile.reset();
	
	if (!Utils::overwritingRename(temp_file_path, target_path)) {
		QFile::remove(temp_file_path);
		return false;
	}
	
	return true;
}

void
AtomicFileOverwriter::abort()
{
	if (!m_ptrTempFile.get()) {
		return;
	}
	
	QString const temp_file_path(m_ptrTempFile->fileName());
	m_ptrTempFile.reset(); // See comments in commit()
	QFile::remove(temp_file_path);
}
