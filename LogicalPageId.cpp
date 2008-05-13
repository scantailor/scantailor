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

#include "LogicalPageId.h"

LogicalPageId::LogicalPageId()
:	m_subPage(SINGLE_PAGE)
{
}

LogicalPageId::LogicalPageId(ImageId const& image_id, SubPage subpage)
:	m_imageId(image_id),
	m_subPage(subpage)
{
}

bool operator==(LogicalPageId const& lhs, LogicalPageId const& rhs)
{
	return lhs.subPage() == rhs.subPage() && lhs.imageId() == rhs.imageId();
}

bool operator!=(LogicalPageId const& lhs, LogicalPageId const& rhs)
{
	return !(lhs == rhs);
}

bool operator<(LogicalPageId const& lhs, LogicalPageId const& rhs)
{
	if (lhs.imageId() < rhs.imageId()) {
		return true;
	} else if (rhs.imageId() < lhs.imageId()) {
		return false;
	} else {
		return lhs.subPage() < rhs.subPage();
	}
}

