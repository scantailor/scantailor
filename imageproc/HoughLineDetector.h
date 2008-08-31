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

#ifndef IMAGEPROC_HOUGHLINEDETECTOR_H_
#define IMAGEPROC_HOUGHLINEDETECTOR_H_

#include <QPointF>
#include <vector>

class QSize;
class QLineF;
class QImage;

namespace imageproc
{

class BinaryImage;

/**
 * \brief A line detected by HoughLineDetector.
 *
 * A line is represented by a unit-size vector perpendicular to
 * the line, and a distance along that vector to a point on the line.
 * In other words, unit_vector * distance is a point on the line and
 * unit_vector is the normal vector for that line.
 */
class HoughLine
{
public:
	HoughLine() : m_normUnitVector(), m_distance(), m_quality() {}
	
	HoughLine(QPointF const& norm_uv, double distance, unsigned quality)
	: m_normUnitVector(norm_uv), m_distance(distance), m_quality(quality) {}
	
	QPointF const& normUnitVector() const { return m_normUnitVector; }
	
	double distance() const { return m_distance; }
	
	/**
	 * \brief The sum of weights of points on the line.
	 *
	 * The weight of a point is an argument to HoughLineDetector::put().
	 */
	unsigned quality() const { return m_quality; }
	
	QPointF pointAtY(double y) const;
	
	QPointF pointAtX(double x) const;
	
	/**
	 * \brief Returns an arbitrary line segment of length 1.
	 */
	QLineF unitSegment() const;
private:
	QPointF m_normUnitVector;
	double m_distance;
	unsigned m_quality;
};
	

class HoughLineDetector
{
public:
	/**
	 * \brief A line finder based on Hough transform.
	 *
	 * \param input_dimensions The range of valid input coordinates,
	 *        which are [0, width - 1] for x and [0, height - 1] for y.
	 * \param distance_resolution The distance in input units that
	 *        represents the width of the lines we are searching for.
	 *        The more this parameter is, the more pixels on the sides
	 *        of a line will be considered a part of it.
	 *        Normally this parameter greater than 1, but theoretically
	 *        it maybe any positive value.
	 * \param start_angle The first angle to check for.  This angle
	 *        is between the normal vector of a line we are looking for
	 *        and the X axis.  The angle is in degrees.
	 * \param angle_delta The difference (in degrees) between an
	 *        angle and the next one.
	 * \param num_angles The number of angles to check.
	 */
	HoughLineDetector(QSize const& input_dimensions, double distance_resolution,
		double start_angle, double angle_delta, int num_angles);
	
	/**
	 * \brief Processes a point with a specified weight.
	 */
	void process(int x, int y, unsigned weight = 1);
	
	QImage visualizeHoughSpace(unsigned lower_bound) const;
	
	/**
	 * \brief Returns the lines found among the input points.
	 *
	 * The lines will be ordered by the descending quality.
	 * \see HoughLineDetector::Line::quality()
	 *
	 * \param quality_lower_bound The minimum acceptable line quality.
	 */
	std::vector<HoughLine> findLines(unsigned quality_lower_bound) const;
private:
	class GreaterQualityFirst;
	
	static BinaryImage findHistogramPeaks(
		std::vector<unsigned> const& hist, int width, int height,
		unsigned lower_bound);
	
	static BinaryImage findPeakCandidates(
		std::vector<unsigned> const& hist, int width, int height,
		unsigned lower_bound);
	
	static void incrementBinsMasked(
		std::vector<unsigned>& hist,
		int width, int height, BinaryImage const& mask);
	
	static void max5x5(
		std::vector<unsigned> const& src,
		std::vector<unsigned>& dst, int width, int height);
	
	static void max3x1(
		std::vector<unsigned> const& src,
		std::vector<unsigned>& dst, int width, int height);
	
	static void max1x3(
		std::vector<unsigned> const& src,
		std::vector<unsigned>& dst, int width, int height);
	
	static BinaryImage buildEqualMap(
		std::vector<unsigned> const& src1,
		std::vector<unsigned> const& src2,
		int width, int height, unsigned lower_bound);
	
	/**
	 * \brief A 2D histogram laid out in raster order.
	 *
	 * Rows correspond to line angles while columns correspond to
	 * line distances from the origin.
	 */
	std::vector<unsigned> m_histogram;
	
	/**
	 * \brief An array of sines (y) and cosines(x) of angles we working with.
	 */
	std::vector<QPointF> m_angleUnitVectors;
	
	/**
	 * \see HoughLineDetector:HoughLineDetector()
	 */
	double m_distanceResolution;
	
	/**
	 * 1.0 / m_distanceResolution
	 */
	double m_recipDistanceResolution;
	
	/**
	 * The value to be added to distance to make sure it's positive.
	 */
	double m_distanceBias;
	
	/**
	 * The width of m_histogram.
	 */
	int m_histWidth;
	
	/**
	 * The height of m_histogram.
	 */
	int m_histHeight;
};

} // namespace imageproc

#endif
