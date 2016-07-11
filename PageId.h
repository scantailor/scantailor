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

#ifndef PAGEID_H_
#define PAGEID_H_

#include "ImageId.h"

class QString;

/**
 * \brief A logical page on an image.
 *
 * An image can contain one or two logical pages.
 */
class PageId
{
	// Member-wise copying is OK.
public:
	enum SubPage { SINGLE_PAGE, LEFT_PAGE, RIGHT_PAGE };
	
	PageId();
	
	/**
	 * \note The default parameter for subpage is not arbitrary.  It has to
	 *       precede other values in terms of operator<().  That's necessary
	 *       to be able to use lower_bound() to find the first page with
	 *       a matching image id.
	 */
	explicit PageId(ImageId const& image_id, SubPage subpage = SINGLE_PAGE);
	
	bool isNull() const { return m_imageId.isNull(); }
	
	ImageId& imageId() { return m_imageId; }

	ImageId const& imageId() const { return m_imageId; }
	
	SubPage subPage() const { return m_subPage; }
	
	QString subPageAsString() const { return subPageToString(m_subPage); }
	
	static QString subPageToString(SubPage sub_page);
	
	static SubPage subPageFromString(QString const& string, bool* ok = 0);
private:
	ImageId m_imageId;
	SubPage m_subPage;
};

bool operator==(PageId const& lhs, PageId const& rhs);
bool operator!=(PageId const& lhs, PageId const& rhs);
bool operator<(PageId const& lhs, PageId const& rhs);

#endif
