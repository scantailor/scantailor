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

#ifndef IMAGEPROC_SEDM_H_
#define IMAGEPROC_SEDM_H_

#include "foundation/FlagOps.h"
#include <vector>
#include <QSize>
#include <stdint.h>

namespace imageproc
{

class BinaryImage;
class ConnectivityMap;

/**
 * \brief The squared euclidean distance map.
 *
 * For each pixel of the input image stores the squared euclidean
 * (straight line) distance to either the nearest white or black pixel.
 *
 * The implementation is based on the following paper:\n
 * Meijster, A., Roerdink, J., and Hesselink, W. 2000.
 * A general algorithm for computing distance transforms in linear time.
 * In Proceedings of the 5th International Conference on Mathematical
 * Morphology and its Applications to Image and Signal Processing.
 */
class SEDM
{
public:
	/**
	 * \brief The type of distance to compute.
	 */
	enum DistType {
		/**
		 * For every black pixel, the distance to the nearest
		 * white one is computed.
		 */
		DIST_TO_WHITE,
		
		/**
		 * For every white pixel, the distance to the nearest
		 * black one is computed.
		 */
		DIST_TO_BLACK
	};
	
	/**
	 * \brief Determines whether to compute the distance to borders.
	 */
	enum Borders {
		DIST_TO_NO_BORDERS = 0,
		DIST_TO_TOP_BORDER = 1,
		DIST_TO_LEFT_BORDER = 2,
		DIST_TO_RIGHT_BORDER = 4,
		DIST_TO_BOTTOM_BORDER = 8,
		DIST_TO_VERT_BORDERS = DIST_TO_LEFT_BORDER|DIST_TO_RIGHT_BORDER,
		DIST_TO_HOR_BORDERS = DIST_TO_TOP_BORDER|DIST_TO_BOTTOM_BORDER,
		DIST_TO_ALL_BORDERS = DIST_TO_HOR_BORDERS|DIST_TO_VERT_BORDERS
	};
	
	/**
	 * \brief The infinite distance.
	 *
	 * If the input image doesn't have any objects to compute
	 * distance to, and borders are to DIST_TO_NO_BORDERS,
	 * then the whole distance map will consist of these values.
	 */
	static uint32_t const INF_DIST;
	
	/**
	 * \brief Constructs a null distance map.
	 *
	 * The data() method returns null on such maps.
	 */
	SEDM();
	
	/**
	 * \brief Build a distance map from a binary image.
	 *
	 * For every black pixel in the image, the distance
	 * map will store the squared straight-line distance
	 * to the nearest white pixel.  The distance between
	 * two pixels is the distance between their center points.
	 *
	 * \param image The image to compute the distance map from.
	 * \param dist_type Determines whether to compute distance
	 *        to white or black pixels in the image.
	 * \param borders Determines whether to compute
	 *        distance to particular borders.  The borders
	 *        are assumed to lie one pixel off the image area.
	 */
	explicit SEDM(
		BinaryImage const& image, DistType dist_type = DIST_TO_WHITE,
		Borders borders = DIST_TO_ALL_BORDERS);
	
	/**
	 * \brief Build a distance map from a connectivity map.
	 *
	 * For every zero label in the connectivity map, the distance
	 * map will store the squared straight-line distance to the
	 * nearest non-zero label.
	 * \note Besides building a distance map, it will modify
	 *       the connectivity map by overwriting zero labels
	 *       with the nearest non-zero label.  This applies to
	 *       the padding areas of the connectivity map as well.
	 */
	explicit SEDM(ConnectivityMap& cmap);
	
	SEDM(SEDM const& other);
	
	SEDM& operator=(SEDM const& other);
	
	void swap(SEDM& other);
	
	/**
	 * \brief Return the dimensions of the distance map.
	 */
	QSize size() const { return m_size; }
	
	/**
	 * \brief Return the number of 32bit words in a line.
	 *
	 * This value is going to be size().width() + 2.
	 */
	int stride() const { return m_stride; }
	
	/**
	 * \brief Return a matrix of squared distances in row-major order.
	 */
	uint32_t* data() { return m_pData; }
	
	/**
	 * \brief Return a matrix of squared distances in row-major order.
	 */
	uint32_t const* data() const { return m_pData; }
	
	/**
	 * \brief Finds peaks on the distance map, altering it in the process.
	 *
	 * A peak region is a 4-connected group of cells having the same
	 * distance value, that doesn't have any neighbors with a higher
	 * distance value.
	 *
	 * Peaks on a Euclidean distance map are also known as ultimate
	 * eroded points.
	 *
	 * The Borders flags used to build this SEDM also affect the peaks
	 * on it.  If the distance to a particular object was considered,
	 * that border was considered an object, so a peak may be found
	 * between this border and another object.
	 *
	 * Peaks are returned in a BinaryImage, and the distance
	 * map is altered in an uspecified way.
	 */
	BinaryImage findPeaksDestructive();
private:
	static uint32_t distSq(int x1, int x2, uint32_t dy_sq);
	
	void processColumns();
	
	void processColumns(ConnectivityMap& cmap);
	
	void processRows();
	
	void processRows(ConnectivityMap& cmap);
	
	BinaryImage findPeakCandidatesNonPadded() const;
	
	BinaryImage buildEqualMapNonPadded(uint32_t const* src1, uint32_t const* src2) const;
	
	void max3x3(uint32_t const* src, uint32_t* dst) const;
	
	void max3x1(uint32_t const* src, uint32_t* dst) const;
	
	void max1x3(uint32_t const* src, uint32_t* dst) const;
	
	void incrementMaskedPadded(BinaryImage const& mask);
	
	std::vector<uint32_t> m_data;
	uint32_t* m_pData;
	QSize m_size;
	int m_stride;
};


inline void swap(SEDM& o1, SEDM& o2)
{
	o1.swap(o2);
}

DEFINE_FLAG_OPS(SEDM::Borders)

} // namespace imageproc

#endif
