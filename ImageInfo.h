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

#ifndef IMAGEINFO_H_
#define IMAGEINFO_H_

#include "ImageId.h"
#include "ImageMetadata.h"

/**
 * This class stores the same information about an image as ProjectPages does,
 * and is used for adding images to ProjectPages objects.  Beyond that,
 * ProjectPages doesn't operate with ImageInfo objects, but with PageInfo ones.
 */
class ImageInfo
{
	// Member-wise copying is OK.
public:
	ImageInfo();
	
	ImageInfo(ImageId const& id, ImageMetadata const& metadata,
		int num_sub_pages, bool left_page_removed, bool right_page_removed);
	
	ImageId const& id() const { return m_id; }
	
	ImageMetadata const& metadata() const { return m_metadata; }
	
	int numSubPages() const { return m_numSubPages; }

	bool leftHalfRemoved() const { return m_leftHalfRemoved; }

	bool rightHalfRemoved() const { return m_rightHalfRemoved; }
private:
	ImageId m_id;
	ImageMetadata m_metadata;
	int m_numSubPages; // 1 or 2
	bool m_leftHalfRemoved;  // Both can't be true, and if one is true,
	bool m_rightHalfRemoved; // then m_numSubPages is 1.
};

#endif
