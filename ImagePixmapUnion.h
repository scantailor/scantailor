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

#ifndef IMAGE_PIXMAP_UNION_H_
#define IMAGE_PIXMAP_UNION_H_

#include <QImage>
#include <QPixmap>

class ImagePixmapUnion
{
	// Member-wise copying is OK.
public:
	ImagePixmapUnion() {}

	ImagePixmapUnion(QImage const& image) : m_image(image) {}

	ImagePixmapUnion(QPixmap const& pixmap) : m_pixmap(pixmap) {}

	QImage const& image() const { return m_image; }

	QPixmap const& pixmap() const { return m_pixmap; }

	bool isNull() const { return m_image.isNull() && m_pixmap.isNull(); }
private:
	QImage m_image;
	QPixmap m_pixmap;
};

#endif
