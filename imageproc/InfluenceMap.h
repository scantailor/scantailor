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

#ifndef IMAGEPROC_INFLUENCE_MAP_H_
#define IMAGEPROC_INFLUENCE_MAP_H_

#include <QSize>
#include <vector>
#include <stdint.h>

class QImage;

namespace imageproc
{

class BinaryImage;
class ConnectivityMap;

/**
 * \brief Takes labelled regions and extends them to cover a larger area.
 *
 * Extension goes by taking over zero (that is background) labels.
 * Extension is done in such a way that two different labels meet at
 * a location equidistant from the initial areas of those two labels.
 */
class InfluenceMap
{
public:
	struct Vector
	{
		int16_t x;
		int16_t y;
	};
	
	struct Cell
	{
		/**
		 * The label from the connectivity map.
		 */
		uint32_t label;
		
		/**
		 * The squared euclidean distance to the nearest pixel
		 * initially (that is before extension) labelled with
		 * the same label.
		 */
		uint32_t distSq;
		
		/**
		 * The vector pointing to the nearest pixel initially
		 * (that is before extension) labelled with the same
		 * label.
		 */
		Vector vec;
	};
	
	/**
	 * \brief Construct a null influence map.
	 *
	 * The data() and paddedData() methods return null on such maps.
	 */
	InfluenceMap();
	
	/**
	 * \brief Take labelled regions from a connectivity map
	 *        and extend them to cover the whole available area.
	 */
	explicit InfluenceMap(ConnectivityMap const& cmap);
	
	/**
	 * \brief Take labelled regions from a connectivity map
	 *        and extend them to cover the area that's black in mask.
	 */
	InfluenceMap(ConnectivityMap const& cmap, BinaryImage const& mask);
	
	InfluenceMap(InfluenceMap const& other);
	
	InfluenceMap& operator=(InfluenceMap const& other);
	
	void swap(InfluenceMap& other);
	
	/**
	 * \brief Returns a pointer to the top-left corner of the map.
	 *
	 * The data is stored in row-major order, and is padded,
	 * so moving to the next line requires adding stride() rather
	 * than size().width().
	 */
	Cell const* data() const { return m_pData; }
	
	/**
	 * \brief Returns a pointer to the top-left corner of the map.
	 *
	 * The data is stored in row-major order, and is padded,
	 * so moving to the next line requires adding stride() rather
	 * than size().width().
	 */
	Cell* data() { return m_pData; }
	
	/**
	 * \brief Returns a pointer to the top-left corner of padding of the map.
	 *
	 * The actually has a fake line from each side.  Those lines are
	 * labelled as background (label 0).  Sometimes it might be desirable
	 * to access that data.
	 */
	Cell const* paddedData() const {
		return m_pData ? &m_data[0] : 0;
	}
	
	/**
	 * \brief Returns a pointer to the top-left corner of padding of the map.
	 *
	 * The actually has a fake line from each side.  Those lines are
	 * labelled as background (label 0).  Sometimes it might be desirable
	 * to access that data.
	 */
	Cell* paddedData() {
		return m_pData ? &m_data[0] : 0;
	}
	
	/**
	 * \brief Returns non-padded dimensions of the map.
	 */
	QSize size() const { return m_size; }
	
	/**
	 * \brief Returns the number of units on a padded line.
	 *
	 * Whether working with padded or non-padded maps, adding
	 * this number to a data pointer will move it one line down.
	 */
	int stride() const { return m_stride; }
	
	/**
	 * \brief Returns the maximum label present on the map.
	 */
	uint32_t maxLabel() const { return m_maxLabel; }
	
	/**
	 * \brief Visualizes each label with a different color.
	 *
	 * Label 0 (which is assigned to background) is represented
	 * by transparent pixels.
	 */
	QImage visualized() const;
private:
	void init(ConnectivityMap const& cmap, BinaryImage const* mask = 0);
	
	std::vector<Cell> m_data;
	Cell* m_pData;
	QSize m_size;
	int m_stride;
	uint32_t m_maxLabel;
};


inline void swap(InfluenceMap& o1, InfluenceMap& o2)
{
	o1.swap(o2);
}

} // namespace imageproc

#endif
