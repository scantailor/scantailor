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

#ifndef DEBUGIMAGES_H_
#define DEBUGIMAGES_H_

#include <QImage>
#include <QString>
#include <list>

namespace imageproc
{
	class BinaryImage;
}

/**
 * \brief A list of image + label pairs.
 */
class DebugImages
{
public:
	class Item
	{
	public:
		Item(QImage const& image, QString const& label);
		
		QImage& image() { return m_image; }
		
		QImage const& image() const { return m_image; }
		
		QString const& label() const { return m_label; }
	private:
		QImage m_image;
		QString m_label;
	};
	
	void add(QImage const& image, QString const& label);
	
	void add(imageproc::BinaryImage const& image, QString const& label);
	
	std::list<Item>& items() { return m_items; }
	
	std::list<Item> const& items() const { return m_items; }
private:
	std::list<Item> m_items;
};

#endif
