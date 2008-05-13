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

#ifndef IMAGEPROC_CONNCOMP_H_
#define IMAGEPROC_CONNCOMP_H_

#include <QRect>

namespace imageproc
{

/**
 * \brief Represents a connected group of pixels.
 */
class ConnComp
{
public:
	ConnComp() : m_pixCount(0) {}
	
	ConnComp(QPoint const& seed, QRect const& rect, int pix_count)
	: m_seed(seed), m_rect(rect), m_pixCount(pix_count) {}
	
	bool isNull() const { return m_rect.isNull(); }
	
	/**
	 * \brief Get an arbitrary black pixel position.
	 *
	 * The position is in containing image coordinates,
	 * not in the bounding box coordinates.
	 */
	QPoint const& seed() const { return m_seed; }
	
	int width() const { return m_rect.width(); }
	
	int height() const { return m_rect.height(); }
	
	QRect const& rect() const { return m_rect; }
	
	int pixCount() const { return m_pixCount; }
private:
	QPoint m_seed;
	QRect m_rect;
	int m_pixCount;
};

} // namespace imageproc

#endif
