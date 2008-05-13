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

#ifndef LOGICALPAGEID_H_
#define LOGICALPAGEID_H_

#include "ImageId.h"

/**
 * \brief A logical page on an image.
 *
 * An image can contain one or two logical pages.
 */
class LogicalPageId
{
	// Member-wise copying is OK.
public:
	enum SubPage { SINGLE_PAGE, LEFT_PAGE, RIGHT_PAGE };
	
	LogicalPageId();
	
	LogicalPageId(ImageId const& image_id, SubPage subpage);
	
	bool isNull() const { return m_imageId.isNull(); }
	
	ImageId const& imageId() const { return m_imageId; }
	
	SubPage subPage() const { return m_subPage; }
	
	int subPageNum() const { return m_subPage == RIGHT_PAGE ? 1 : 0; }
private:
	ImageId m_imageId;
	SubPage m_subPage;
};

bool operator==(LogicalPageId const& lhs, LogicalPageId const& rhs);
bool operator!=(LogicalPageId const& lhs, LogicalPageId const& rhs);
bool operator<(LogicalPageId const& lhs, LogicalPageId const& rhs);

#endif
