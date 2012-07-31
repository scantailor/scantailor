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

#ifndef IMAGEPROC_RAST_LINE_FINDER_H_
#define IMAGEPROC_RAST_LINE_FINDER_H_

#include "PriorityQueue.h"
#include <QPointF>
#include <QLineF>
#include <vector>
#include <string>
#include <stddef.h>

namespace imageproc
{

class RastLineFinderParams
{
public:
	RastLineFinderParams();

	/**
	 * The algorithm operates in polar coordinates. One of those coordinates
	 * is a signed distance to the origin. By default the origin is at (0, 0),
	 * but you can set it explicitly with this call.
	 */
	void setOrigin(QPointF const& origin) { m_origin = origin; }

	/** \see setOrigin() */
	QPointF const& origin() const { return m_origin; }

	/**
	 * By default, all angles are considered. Keeping in mind that line direction
	 * doesn't matter, that gives us the range of [0, 180) degrees.
	 * This method allows you to provide a custom range to consider.
	 * Cases where min_angle_deg > max_angle_deg are valid. Consider the difference
	 * between [20, 200) and [200, 20). The latter one is equivalent to [200, 380).
	 * 
	 * \note This is not the angle between the line and the X axis!
	 *       Instead, you take your origin point (which is customizable)
	 *       and draw a perpendicular to your line. This vector,
	 *       from origin to line, is what defines the line angle.
	 *       In other words, after normalizing it to unit length, its
	 *       coordinates will correspond to cosine and sine of your angle.
	 */
	void setAngleRangeDeg(double min_angle_deg, double max_angle_deg) {
		m_minAngleDeg = min_angle_deg;
		m_maxAngleDeg = max_angle_deg;
	}

	/** \see setAngleRangeDeg() */
	double minAngleDeg() const { return m_minAngleDeg; }

	/** \see setAngleRangeDeg() */
	double maxAngleDeg() const { return m_maxAngleDeg; }

	/**
	 * Being a recursive subdivision algorithm, it has to stop refining the angle
	 * at some point. Angle tolerance is the maximum acceptable error (in degrees)
	 * for the lines returned. By default it's set to 0.1 degrees. Setting it to
	 * a higher value will improve performance.
	 */
	void setAngleToleranceDeg(double tolerance_deg) { m_angleToleranceDeg = tolerance_deg; }

	/** \see setAngleToleranceDeg() */
	double angleToleranceDeg() const { return m_angleToleranceDeg; }

	/**
	 * Sets the maximum distance the point is allowed to be from a line
	 * to still be considered a part of it. In reality, this value is
	 * a lower bound. The upper bound depends on angle tolerance and
	 * will tend to the lower bound as angle tolerance tends to zero.
	 *
	 * \see setAngleTolerance()
	 */
	void setMaxDistFromLine(double dist) { m_maxDistFromLine = dist; }

	/** \see setMaxDistFromLine() */
	double maxDistFromLine() const { return m_maxDistFromLine; }

	/**
	 * A support point is a point considered to be a part of a line.
	 * By default, lines consisting of 3 or more points are considered.
	 * The minimum allowed value is 2, while higher values improve performance.
	 *
	 * \see setMaxDistFromLine()
	 */
	void setMinSupportPoints(unsigned pts) { m_minSupportPoints = pts; }

	/**
	 * \see setMinSupportPoints()
	 */
	unsigned minSupportPoints() const { return m_minSupportPoints; }

	/**
	 * \brief Checks if parameters are valid, optionally providing an error string.
	 */
	bool validate(std::string* error = 0) const;
private:
	QPointF m_origin;
	double m_minAngleDeg;
	double m_maxAngleDeg;
	double m_angleToleranceDeg;
	double m_maxDistFromLine;
	unsigned m_minSupportPoints;
};


/**
 * \brief Finds lines in point clouds.
 *
 * This class implements the following algorithm:\n
 * Thomas M. Breuel. Finding Lines under Bounded Error.\n
 * Pattern Recognition, 29(1):167-178, 1996.\n
 * http://infoscience.epfl.ch/record/82286/files/93-11.pdf?version=1
 */
class RastLineFinder
{
private:
	class SearchSpace;

	friend void swap(SearchSpace& o1, SearchSpace& o2) {
		o1.swap(o2);
	}
public:
	/**
	 * Construct a line finder from a point cloud and a set of parameters.
	 *
	 * \throw std::invalid_argument if \p params are invalid.
	 * \see RastLineFinderParams::validate()
	 */
	RastLineFinder(std::vector<QPointF> const& points, RastLineFinderParams const& params);

	/**
	 * Look for the next best line in terms of the number of support points.
	 * When a line is found, its support points are removed from the lists of
	 * support points of other candidate lines.
	 *
	 * \param[out] point_idxs If provided, it will be filled with indices of support
	 *             points for this line. The indices index the vector of points
	 *             that was passed to RastLineFinder constructor.
	 * \return If there are no more lines satisfying the search criteria,
	 *         a null (default constructed) QLineF is returned. Otherwise,
	 *         a line that goes near its support points is returned.
	 *         Such a line is not to be treated as a line segment, that is positions
	 *         of its endpoints should not be counted upon. In addition, the
	 *         line won't be properly fit to its support points, but merely be
	 *         close to an optimal line.
	 */
	QLineF findNext(std::vector<unsigned>* point_idxs = 0);
private:
	class Point
	{
	public:
		QPointF pt;
		bool available;

		Point(QPointF const& p) : pt(p), available(true) {}
	};

	class PointUnavailablePred
	{
	public:
		PointUnavailablePred(std::vector<Point> const* points) : m_pPoints(points) {}

		bool operator()(unsigned idx) const { return !(*m_pPoints)[idx].available; }
	private:
		std::vector<Point> const* m_pPoints;
	};

	class SearchSpace
	{
	public:
		SearchSpace();

		SearchSpace(RastLineFinder const& owner, float min_dist, float max_dist,
			float min_angle_rad, float max_angle_rad, std::vector<unsigned> const& candidate_idxs);

		/**
		 * Returns a line that corresponds to the center of this search space.
		 * The returned line should be treated as an unbounded line rather than
		 * line segment, meaning that exact positions of endpoints can't be
		 * counted on.
		 */
		QLineF representativeLine(RastLineFinder const& owner) const;

		bool subdivideDist(RastLineFinder const& owner, SearchSpace& subspace1, SearchSpace& subspace2) const;

		bool subdivideAngle(RastLineFinder const& owner, SearchSpace& subspace1, SearchSpace& subspace2) const;

		void pruneUnavailablePoints(PointUnavailablePred pred);

		std::vector<unsigned>& pointIdxs() { return m_pointIdxs; }

		std::vector<unsigned> const& pointIdxs() const { return m_pointIdxs; }

		void swap(SearchSpace& other);
	private:
		float m_minDist; //
		float m_maxDist; // These are already extended by max-dist-to-line.
		float m_minAngleRad;
		float m_maxAngleRad;
		std::vector<unsigned> m_pointIdxs; // Indexes into m_points of the parent object.
	};

	class OrderedSearchSpaces : public PriorityQueue<SearchSpace, OrderedSearchSpaces>
	{
		friend class PriorityQueue<SearchSpace, OrderedSearchSpaces>;
	private:
		void setIndex(SearchSpace& obj, size_t heap_idx) {}
		
		bool higherThan(SearchSpace const& lhs, SearchSpace const& rhs) const {
			return lhs.pointIdxs().size() > rhs.pointIdxs().size();
		}
	};

	void pushIfGoodEnough(SearchSpace& ssp);

	void markPointsUnavailable(std::vector<unsigned> const& point_idxs);

	void pruneUnavailablePoints();

	QPointF m_origin;
	double m_angleToleranceRad;
	double m_maxDistFromLine;
	unsigned m_minSupportPoints;
	std::vector<Point> m_points;
	OrderedSearchSpaces m_orderedSearchSpaces;
	bool m_firstLine;
};

} // namespace imageproc

#endif
