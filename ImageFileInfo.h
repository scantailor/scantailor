/*
    Scan Tailor - Interactive post-processing tool for scanned pages.
    Copyright (C) 2007-2009  Joseph Artsimovich <joseph_a@mail.ru>

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

#ifndef IMAGEFILEINFO_H_
#define IMAGEFILEINFO_H_

#include "ImageMetadata.h"
#include <QFileInfo>
#include <vector>

class ImageFileInfo
{
	// Member-wise copying is OK.
public:
	ImageFileInfo(QFileInfo const& file_info, std::vector<ImageMetadata> const& image_info)
	: m_fileInfo(file_info), m_imageInfo(image_info) {}
	
	QFileInfo const& fileInfo() const { return m_fileInfo; }
	
	std::vector<ImageMetadata>& imageInfo() { return m_imageInfo; }
	
	std::vector<ImageMetadata> const& imageInfo() const { return m_imageInfo; }
	
	bool isDpiOK() const;
private:
	QFileInfo m_fileInfo;
	std::vector<ImageMetadata> m_imageInfo;
};

#endif
