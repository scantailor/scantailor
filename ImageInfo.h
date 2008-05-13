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

#ifndef IMAGEINFO_H_
#define IMAGEINFO_H_

#include "ImageId.h"
#include "ImageMetadata.h"

class ImageInfo
{
	// Member-wise copying is OK.
public:
	ImageInfo() : m_numSubPages(0) {}
	
	ImageInfo(ImageId const& id, ImageMetadata const& metadata, int num_sub_pages)
	: m_id(id), m_metadata(metadata), m_numSubPages(num_sub_pages) {}
	
	ImageId const& id() const { return m_id; }
	
	ImageMetadata const& metadata() const { return m_metadata; }
	
	int numSubPages() const { return m_numSubPages; }
private:
	ImageId m_id;
	ImageMetadata m_metadata;
	int m_numSubPages;
};

#endif
