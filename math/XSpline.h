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

#ifndef XSPLINE_H_
#define XSPLINE_H_

#include "spfit/FittableSpline.h"
#include "QuadraticFunction.h"
#include "VirtualFunction.h"
#include "NumericTraits.h"
#include <QPointF>
#include <QLineF>
#include <vector>

/**
 * \brief An open X-Spline.
 *
 * [1] Blanc, C., Schlick, C.: X-splines: a spline model designed for the end-user.
 * http://scholar.google.com/scholar?cluster=2002168279173394147&hl=en&as_sdt=0,5
 */
class XSpline : public spfit::FittableSpline
{
public:
	struct PointAndDerivs
	{
		QPointF point; /**< Point on a spline. */
		QPointF firstDeriv; /**< First derivative with respect to t. */
		QPointF secondDeriv; /**< Second derivative with respect to t. */

		/**
		 * \brief Curvature at a given point on the spline.
		 *
		 * The sign indicates curving direction.  Positive signs normally
		 * indicate anti-clockwise direction, though in 2D computer graphics
		 * it's usually the other way around, as the Y axis points down.
		 * In other words, if you rotate your coordinate system so that
		 * the X axis aligns with the tangent vector, curvature will be
		 * positive if the spline curves towards the positive Y direction.
		 */
		double signedCurvature() const;
	};

	virtual int numControlPoints() const;

	/**
	 * Returns the number of segments, that is spans between adjacent control points.
	 * Because this class only deals with open splines, the number of segments
	 * will always be max(0, numControlPoints() - 1).
	 */
	int numSegments() const;

	double controlPointIndexToT(int idx) const;

	/**
	 * \brief Appends a control point to the end of the spline.
	 *
	 * Tension values lie in the range of [-1, 1].
	 * \li tension < 0 produces interpolating patches.
	 * \li tension == 0 produces sharp angle interpolating patches.
	 * \li tension > 0 produces approximating patches.
	 */
	void appendControlPoint(QPointF const& pos, double tension);

	/**
	 * \brief Inserts a control at a specified position.
	 *
	 * \p idx is the position where the new control point will end up in.
	 * The following control points will be shifted.
	 */
	void insertControlPoint(int idx, QPointF const& pos, double tension);

	void eraseControlPoint(int idx);

	virtual QPointF controlPointPosition(int idx) const;

	virtual void moveControlPoint(int idx, QPointF const& pos);

	double controlPointTension(int idx) const;

	void setControlPointTension(int idx, double tension);

	/**
	 * \brief Calculates a point on the spline at position t.
	 *
	 * \param t Position on a spline in the range of [0, 1].
	 * \return Point on a spline.
	 *
	 * \note Calling this function with less then 2 control points
	 *       leads to undefined behaviour.
	 */
	QPointF pointAt(double t) const;
	
	/**
	 * \brief Calculates a point on the spline plus the first and the second derivatives at position t.
	 */
	PointAndDerivs pointAndDtsAt(double t) const;

	/** \see spfit::FittableSpline::linearCombinationAt() */
	virtual void linearCombinationAt(double t, std::vector<LinearCoefficient>& coeffs) const;

	/**
	 * Returns a function equivalent to:
	 * \code
	 * sum((cp[i].x - cp[i - 1].x)^2 + (cp[i].y - cp[i - 1].y)^2)
	 * \endcode
	 * Except the returned function is a function of control point displacements,
	 * not positions.
	 * The sum is calculated across all segments.
	 */
	QuadraticFunction controlPointsAttractionForce() const;

	/**
	 * Same as the above one, but you provide a range of segments to consider.
	 * The range is half-closed: [seg_begin, seg_end)
	 */
	QuadraticFunction controlPointsAttractionForce(int seg_begin, int seg_end) const;

	/**
	 * Returns a function equivalent to:
	 * \code
	 * sum(Vec2d(pointAt(controlPointIndexToT(i)) - pointAt(controlPointIndexToT(i - 1))).squaredNorm())
	 * \endcode
	 * Except the returned function is a function of control point displacements,
	 * not positions.
	 * The sum is calculated across all segments.
	 */
	QuadraticFunction junctionPointsAttractionForce() const;

	/**
	 * Same as the above one, but you provide a range of segments to consider.
	 * The range is half-closed: [seg_begin, seg_end)
	 */
	QuadraticFunction junctionPointsAttractionForce(int seg_begin, int seg_end) const;
	
	/**
	 * \brief Finds a point on the spline that's closest to a given point.
	 *
	 * \param to The point which we trying to minimize the distance to.
	 * \param t If provided, the t value corresponding to the found point will be written there.
	 * \param accuracy The maximum distance from the found point to the spline.
	 * \return The closest point found.
	 */
	QPointF pointClosestTo(QPointF to, double* t, double accuracy = 0.2) const;

	QPointF pointClosestTo(QPointF to, double accuracy = 0.2) const;

	/** \see spfit::FittableSpline::sample() */
	virtual void sample(
		VirtualFunction3<void, QPointF, double, SampleFlags>& sink,
		SamplingParams const& params = SamplingParams(),
		double from_t = 0.0, double to_t = 1.0) const;

	std::vector<QPointF> toPolyline(
		SamplingParams const& params = SamplingParams(),
		double from_t = 0.0, double to_t = 1.0) const;

	void swap(XSpline& other) { m_controlPoints.swap(other.m_controlPoints); }
private:
	struct ControlPoint
	{
		QPointF pos;

		/**
		 * Tension is in range of [-1 .. 1] and corresponds to sk as defined in section 5 of [1],
		 * not to be confused with sk defined in section 4, which has a range of [0 .. 1].
		 */
		double tension;

		ControlPoint() : tension(0) {}

		ControlPoint(QPointF const& p, double tns) : pos(p), tension(tns) {}
	};

	struct TensionDerivedParams;
	class GBlendFunc;
	class HBlendFunc;
	struct DecomposedDerivs;
	
	QPointF pointAtImpl(int segment, double t) const;

	int linearCombinationFor(LinearCoefficient* coeffs, int segment, double t) const;

	DecomposedDerivs decomposedDerivs(double t) const;

	DecomposedDerivs decomposedDerivsImpl(int segment, double t) const;

	void maybeAddMoreSamples(
		VirtualFunction3<void, QPointF, double, SampleFlags>& sink,
		double max_sqdist_to_spline, double max_sqdist_between_samples,
		double num_segments, double r_num_segments,
		double prev_t, QPointF const& prev_pt,
		double next_t, QPointF const& next_pt) const;

	static double sqDistToLine(QPointF const& pt, QLineF const& line);

	std::vector<ControlPoint> m_controlPoints;
};

inline void swap(XSpline& o1, XSpline& o2)
{
	o1.swap(o2);
}

#endif
