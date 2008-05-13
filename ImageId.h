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

#ifndef IMAGEID_H_
#define IMAGEID_H_

#include <QString>

class QFileInfo;

class ImageId
{
	// Member-wise copying is OK.
public:
	ImageId() : m_filePath(), m_page(0) {}
	
	explicit ImageId(QString const& file_path, int page = 0);
	
	explicit ImageId(QFileInfo const& file_info, int page = 0);
	
	bool isNull() const { return m_filePath.isNull(); }
	
	QString const& filePath() const { return m_filePath; }
	
	int page() const { return m_page; }
private:
	QString m_filePath;
	
	/**
	 * \brief A zero-based page number of a multi-page file.
	 */
	int m_page;
};

bool operator==(ImageId const& lhs, ImageId const& rhs);
bool operator!=(ImageId const& lhs, ImageId const& rhs);
bool operator<(ImageId const& lhs, ImageId const& rhs);

#endif
