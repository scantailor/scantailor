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

#ifndef PAGEINFO_H_
#define PAGEINFO_H_

#include "PageId.h"
#include "ImageMetadata.h"

class PageInfo
{
	// Member-wise copying is OK.
public:
	PageInfo();
	
	PageInfo(PageId const& page_id, ImageMetadata const& metadata,
		int image_sub_pages, bool left_half_removed, bool right_half_removed);

	PageId const& id() const { return m_pageId; }

	void setId(PageId const& id) { m_pageId = id; }
	
	ImageId const& imageId() const { return m_pageId.imageId(); }
	
	ImageMetadata const& metadata() const { return m_metadata; }
	
	int imageSubPages() const { return m_imageSubPages; }

	bool leftHalfRemoved() const { return m_leftHalfRemoved; }

	bool rightHalfRemoved() const { return m_rightHalfRemoved; }
private:
	PageId m_pageId;
	ImageMetadata m_metadata;
	int m_imageSubPages;
	bool m_leftHalfRemoved;
	bool m_rightHalfRemoved;
};

#endif
