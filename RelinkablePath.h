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

#ifndef RELINKABLE_PATH_H_
#define RELINKABLE_PATH_H_

#include <QString>

/**
 * \brief Represents a file or directory.
 */
class RelinkablePath
{
	// Member-wise copying is OK.
public:
	enum Type { File, Dir };

	RelinkablePath(QString const& path, Type type);

	QString const& normalizedPath() const { return m_normalizedPath; }

	Type type() const { return m_type; }

	/**
	 * Performs the following operations on the path:
	 * \li Converts backwards slashes to forward slashes.
	 * \li Eliminates redundant slashes.
	 * \li Eliminates "/./" and resolves "/../" components.
	 * \li Removes trailing slashes.
	 *
	 * \return The normalized string on success or an empty string on failure.
	 *         Failure can happen because of unresolvable "/../" components.
	 */
	static QString normalize(QString const& path);
private:
	QString m_normalizedPath;
	Type m_type;
};

#endif
