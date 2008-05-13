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

#include "ImageId.h"
#include <QFileInfo>

ImageId::ImageId(QString const& file_path, int const page)
:	m_filePath(file_path),
	m_page(page)
{
}

ImageId::ImageId(QFileInfo const& file_info, int const page)
:	m_filePath(file_info.absoluteFilePath()),
	m_page(page)
{
}

bool operator==(ImageId const& lhs, ImageId const& rhs)
{
	return lhs.page() == rhs.page() && lhs.filePath() == rhs.filePath();
}

bool operator!=(ImageId const& lhs, ImageId const& rhs)
{
	return !(lhs == rhs);
}

bool operator<(ImageId const& lhs, ImageId const& rhs)
{
	int const comp = lhs.filePath().compare(rhs.filePath());
	if (comp < 0) {
		return true;
	} else if (comp > 0) {
		return false;
	}
	return lhs.page() < rhs.page();
}
