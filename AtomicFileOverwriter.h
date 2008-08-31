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

#ifndef ATOMICFILEOVERWRITER_H_
#define ATOMICFILEOVERWRITER_H_

#include "NonCopyable.h"
#include <memory>

class QString;
class QIODevice;
class QTemporaryFile;

/**
 * \brief Overwrites files by writing to a temporary file and then replacing
 *        the target file with it.
 *
 * Because renaming across volumes doesn't work, we create a temporary file
 * in the same directory as the target file.
 */
class AtomicFileOverwriter
{
	DECLARE_NON_COPYABLE(AtomicFileOverwriter)
public:
	AtomicFileOverwriter();
	
	/**
	 * \brief Destroys the object and calls abort() if necessary.
	 */
	~AtomicFileOverwriter();
	
	/**
	 * \brief Start writing to a temporary file.
	 *
	 * \returns A temporary file as QIODevice, or null of temporary file
	 *          could not be opened.  In latter case, calling abort()
	 *          is not necessary.
	 *
	 * If a file is already being written, it calles abort() and then
	 * proceeds as usual.
	 */
	QIODevice* startWriting(QString const& file_path);
	
	/**
	 * \brief Replaces the target file with the temporary one.
	 *
	 * If replacing failed, false is returned and the temporary file
	 * is removed.
	 */
	bool commit();
	
	/**
	 * \brief Removes the temporary file without touching the target one.
	 */
	void abort();
private:
	std::auto_ptr<QTemporaryFile> m_ptrTempFile;
};

#endif
