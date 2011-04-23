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

#ifndef DEWARPING_DISTORTION_MODEL_BUILDER_H_
#define DEWARPING_DISTORTION_MODEL_BUILDER_H_

#include "VecNT.h"
#include <QTransform>
#include <QPointF>
#include <QLineF>
#include <vector>
#include <deque>
#include <utility>

class QImage;
class DebugImages;
class XSpline;

namespace dewarping
{

class DistortionModel;

class DistortionModelBuilder
{
	// Member-wise copying is OK.
public:
	/**
	 * \brief Constructor.
	 *
	 * \param down_direction A vector pointing approximately downwards in terms of content.
	 *        The vector can't be zero-length.
	 */
	DistortionModelBuilder(Vec2d const& down_direction);

	/**
	 * \brief Set the vertical content boundaries.
	 *
	 * Note that we are setting lines, not line segments, so endpoint
	 * positions along the line don't really matter.  It also doesn't
	 * matter which one is the left bound and which one is the right one.
	 */
	void setVerticalBounds(QLineF const& bound1, QLineF const& bound2);

	/**
	 * \brief Returns the current vertical bounds.
	 *
	 * It's not specified which one is the left and which one is the right bound.
	 */
	std::pair<QLineF, QLineF> verticalBounds() const;

	/**
	 * \brief Add a curve that's meant to become straight and horizontal after dewarping.
	 *
	 * The curve doesn't have to touch or intersect the vertical bounds, although
	 * long curves are preferable.  The minimum number of curves to build a distortion
	 * model is 2, although that doesn't guarantee successful model construction.
	 * The more apart the curves are, the better.
	 */
	void addHorizontalCurve(std::vector<QPointF> const& polyline);

	/**
	 * \brief Applies an affine transformation to the internal representation.
	 */
	void transform(QTransform const& xform);

	/**
	 * \brief Tries to build a distortion model based on information provided so far.
	 *
	 * \return A DistortionModel that may be invalid.
	 * \see DistortionModel::isValid()
	 */
	DistortionModel tryBuildModel(DebugImages* dbg = 0, QImage const* dbg_background = 0) const;
private:
	struct TracedCurve;
	struct RansacModel;
	class RansacAlgo;
	class BadCurve;

	TracedCurve polylineToCurve(std::vector<QPointF> const& polyline) const;

	static Vec2d centroid(std::vector<QPointF> const& polyline);

	std::pair<QLineF, QLineF> frontBackBounds(std::vector<QPointF> const& polyline) const;

	static std::vector<QPointF> maybeTrimPolyline(
		std::vector<QPointF> const& polyline, std::pair<QLineF, QLineF> const& bounds);

	static bool maybeTrimFront(std::deque<QPointF>& polyline, QLineF const& bound);

	static bool maybeTrimBack(std::deque<QPointF>& polyline, QLineF const& bound);

	static void intersectFront(std::deque<QPointF>& polyline, QLineF const& bound);

	static void intersectBack(std::deque<QPointF>& polyline, QLineF const& bound);

	static XSpline fitExtendedSpline(
		std::vector<QPointF> const& polyline, Vec2d const& centroid,
		std::pair<QLineF, QLineF> const& bounds);

	QImage visualizeTrimmedPolylines(
		QImage const& background, std::vector<TracedCurve> const& curves) const;

	QImage visualizeModel(QImage const& background,
		std::vector<TracedCurve> const& curves, RansacModel const& model) const;

	Vec2d m_downDirection;
	Vec2d m_rightDirection;
	QLineF m_bound1; // It's not specified which one is left
	QLineF m_bound2; // and which one is right.

	/** These go left to right in terms of content. */
	std::deque<std::vector<QPointF> > m_ltrPolylines;
};

} // namespace dewarping

#endif
