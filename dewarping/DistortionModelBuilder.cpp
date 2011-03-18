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

#include "DistortionModelBuilder.h"
#include "DistortionModel.h"
#include "CylindricalSurfaceDewarper.h"
#include "LineBoundedByRect.h"
#include "ToLineProjector.h"
#include "SidesOfLine.h"
#include "XSpline.h"
#include "DebugImages.h"
#include "spfit/SqDistApproximant.h"
#include "spfit/PolylineModelShape.h"
#include "spfit/SplineFitter.h"
#include <QTransform>
#include <QImage>
#include <QPainter>
#include <QPen>
#include <QColor>
#include <boost/foreach.hpp>
#include <math.h>
#include <assert.h>

using namespace imageproc;

namespace dewarping
{

struct DistortionModelBuilder::TracedCurve
{
	std::deque<QPointF> trimmedPolyline;   // Both are left to right.
	std::vector<QPointF> extendedPolyline; //
	double order; // Lesser values correspond to upper curves.

	TracedCurve(std::deque<QPointF> const& trimmed_polyline,
		std::vector<QPointF> const& extended_polyline, double ord)
		: trimmedPolyline(trimmed_polyline), extendedPolyline(extended_polyline), order(ord) {}

	bool operator<(TracedCurve const& rhs) const { return order < rhs.order; }
};

struct DistortionModelBuilder::RansacModel
{
	TracedCurve const* topCurve;
	TracedCurve const* bottomCurve; 
	double totalError;

	RansacModel() : topCurve(0), bottomCurve(0), totalError(NumericTraits<double>::max()) {}

	bool isValid() const { return topCurve && bottomCurve; }
};

class DistortionModelBuilder::RansacAlgo
{
public:
	RansacAlgo(std::vector<TracedCurve> const& all_curves)
		: m_rAllCurves(all_curves) {}

	void buildAndAssessModel(
		TracedCurve const* top_curve, TracedCurve const* bottom_curve);

	RansacModel& bestModel() { return m_bestModel; }

	RansacModel const& bestModel() const { return m_bestModel; }
private:
	double calcReferenceHeight(
		CylindricalSurfaceDewarper const& dewarper, QPointF const& loc);

	RansacModel m_bestModel;
	std::vector<TracedCurve> const& m_rAllCurves;
};


DistortionModelBuilder::DistortionModelBuilder(Vec2d const& down_direction)
:	m_downDirection(down_direction),
	m_rightDirection(down_direction[1], -down_direction[0])
{
	assert(down_direction.squaredNorm() > 0);
}

void
DistortionModelBuilder::setVerticalBounds(QLineF const& bound1, QLineF const& bound2)
{
	m_bound1 = bound1;
	m_bound2 = bound2;
}

std::pair<QLineF, QLineF>
DistortionModelBuilder::verticalBounds() const
{
	return std::pair<QLineF, QLineF>(m_bound1, m_bound2);
}

void
DistortionModelBuilder::addHorizontalCurve(std::vector<QPointF> const& polyline)
{
	if (polyline.size() < 2) {
		return;
	}

	if (Vec2d(polyline.back() - polyline.front()).dot(m_rightDirection) > 0) {
		m_ltrPolylines.push_back(polyline);
	} else {
		m_ltrPolylines.push_back(std::vector<QPointF>(polyline.rbegin(), polyline.rend()));
	}
}

void
DistortionModelBuilder::transform(QTransform const& xform)
{
	assert(xform.isAffine());

	QLineF const down_line(xform.map(QLineF(QPointF(0, 0), m_downDirection)));
	QLineF const right_line(xform.map(QLineF(QPointF(0, 0), m_rightDirection)));

	m_downDirection = down_line.p2() - down_line.p1();
	m_rightDirection = right_line.p2() - right_line.p1();
	m_bound1 = xform.map(m_bound1);
	m_bound2 = xform.map(m_bound2);

	BOOST_FOREACH(std::vector<QPointF>& polyline, m_ltrPolylines) {
		BOOST_FOREACH(QPointF& pt, polyline) {
			pt = xform.map(pt);
		}
	}
}

DistortionModel
DistortionModelBuilder::tryBuildModel(DebugImages* dbg, QImage const* dbg_background) const
{
	int const num_curves = m_ltrPolylines.size();

	if (num_curves < 2 || m_bound1.p1() == m_bound1.p2() || m_bound2.p1() == m_bound2.p2()) {
		return DistortionModel();
	}

	std::vector<TracedCurve> ordered_curves;
	ordered_curves.reserve(num_curves);

	BOOST_FOREACH(std::vector<QPointF> const& polyline, m_ltrPolylines) {
		ordered_curves.push_back(polylineToCurve(polyline));
	}
	std::sort(ordered_curves.begin(), ordered_curves.end());

	// Select the best pair using RANSAC.
	RansacAlgo ransac(ordered_curves);

	// First let's try to combine each of the 3 top-most lines
	// with each of the 3 bottom-most ones.
	for (int i = 0; i < std::min<int>(3, num_curves); ++i) {
		for (int j = std::max<int>(0, num_curves - 3); j < num_curves; ++j) {
			if (i < j) {
				ransac.buildAndAssessModel(&ordered_curves[i], &ordered_curves[j]);
			}
		}
	}

	// Continue by throwing in some random pairs of lines.
	qsrand(0); // Repeatablity is important.
	int random_pairs_remaining = 10;
	while (random_pairs_remaining-- > 0) {
		int i = qrand() % num_curves;
		int j = qrand() % num_curves;
		if (i > j) {
			std::swap(i, j);
		}
		if (i < j) {
			ransac.buildAndAssessModel(&ordered_curves[i], &ordered_curves[j]);
		}
	}
	
	if (dbg && dbg_background) {
		dbg->add(visualizeModel(*dbg_background, ordered_curves, ransac.bestModel()), "distortion_model");
	}

	DistortionModel model;
	if (ransac.bestModel().isValid()) {
		model.setTopCurve(Curve(ransac.bestModel().topCurve->extendedPolyline));
		model.setBottomCurve(Curve(ransac.bestModel().bottomCurve->extendedPolyline));
	}
	return model;
}

DistortionModelBuilder::TracedCurve
DistortionModelBuilder::polylineToCurve(std::vector<QPointF> const& polyline) const
{
	std::pair<QLineF, QLineF> const bounds(frontBackBounds(polyline));

	// Trim the polyline if necessary.
	std::deque<QPointF> trimmed_polyline(polyline.begin(), polyline.end());
	trimFront(trimmed_polyline, bounds.first);
	trimBack(trimmed_polyline, bounds.second);

	// Fit the polyline to a spline, extending it to bounds at the same time.
	XSpline const spline(fitExtendedSpline(polyline, bounds));
	
	double const order = centroid(polyline).dot(m_downDirection);
	return TracedCurve(trimmed_polyline, spline.toPolyline(), order);
}

Vec2d
DistortionModelBuilder::centroid(std::vector<QPointF> const& polyline)
{
	int const num_points = polyline.size();
	if (num_points == 0) {
		return Vec2d();
	} else if (num_points == 1) {
		return Vec2d(polyline.front());
	}

	Vec2d accum(0, 0);
	double total_weight = 0;

	for (int i = 1; i < num_points; ++i) {
		QLineF const segment(polyline[i - 1], polyline[i]);
		Vec2d const center(0.5 * (segment.p1() + segment.p2()));
		double const weight = segment.length();
		accum += center * weight;
		total_weight += weight;
	}

	if (total_weight < 1e-06) {
		return Vec2d(polyline.front());
	} else {
		return accum / total_weight;
	}
}

/**
 * \brief Returns bounds ordered according to the direction of \p polyline.
 *
 * The first and second bounds will correspond to polyline.front() and polyline.back()
 * respectively.
 */
std::pair<QLineF, QLineF>
DistortionModelBuilder::frontBackBounds(std::vector<QPointF> const& polyline) const
{
	assert(!polyline.empty());

	ToLineProjector const proj1(m_bound1);
	ToLineProjector const proj2(m_bound2);
	if (proj1.projectionDist(polyline.front()) + proj2.projectionDist(polyline.back()) <
			proj1.projectionDist(polyline.back()) + proj2.projectionDist(polyline.front())) {
		return std::pair<QLineF, QLineF>(m_bound1, m_bound2);
	} else {
		return std::pair<QLineF, QLineF>(m_bound2, m_bound1);
	}
}

bool
DistortionModelBuilder::trimFront(std::deque<QPointF>& polyline, QLineF const& bound)
{
	if (sidesOfLine(bound, polyline.front(), polyline.back()) >= 0) {
		// Doesn't need trimming.
		return false;
	}

	while (polyline.size() > 2 && sidesOfLine(bound, polyline.front(), polyline[1]) > 0) {
		polyline.pop_front();
	}
	
	intersectFront(polyline, bound);
	
	return true;
}

bool
DistortionModelBuilder::trimBack(std::deque<QPointF>& polyline, QLineF const& bound)
{
	if (sidesOfLine(bound, polyline.front(), polyline.back()) >= 0) {
		// Doesn't need trimming.
		return false;
	}

	while (polyline.size() > 2 && sidesOfLine(bound, polyline[polyline.size() - 2], polyline.back()) > 0) {
		polyline.pop_back();
	}
	
	intersectBack(polyline, bound);
	
	return true;
}

void
DistortionModelBuilder::intersectFront(
	std::deque<QPointF>& polyline, QLineF const& bound)
{
	assert(polyline.size() >= 2);

	QLineF const front_segment(polyline.front(), polyline[1]);
	QPointF intersection;
	if (bound.intersect(front_segment, &intersection) != QLineF::NoIntersection) {
		polyline.front() = intersection;
	}
}

void
DistortionModelBuilder::intersectBack(
	std::deque<QPointF>& polyline, QLineF const& bound)
{
	assert(polyline.size() >= 2);

	QLineF const back_segment(polyline[polyline.size() - 2], polyline.back());
	QPointF intersection;
	if (bound.intersect(back_segment, &intersection) != QLineF::NoIntersection) {
		polyline.back() = intersection;
	}
}

XSpline
DistortionModelBuilder::fitExtendedSpline(
	std::vector<QPointF> const& polyline, std::pair<QLineF, QLineF> const& bounds)
{
	using namespace spfit;

	class ModelShape : public PolylineModelShape
	{
	public:
		ModelShape(std::vector<QPointF> const& polyline, std::pair<QLineF, QLineF> const& bounds) :
			PolylineModelShape(polyline),
			m_headPoint(polyline.front()),
			m_tailPoint(polyline.back()),
			m_polylineProj(QLineF(m_headPoint, m_tailPoint)),
			m_bounds(bounds)
		{
		}

		virtual SqDistApproximant localSqDistApproximant(
			QPointF const& pt, FittableSpline::SampleFlags flags) const {

			if (flags & FittableSpline::HEAD_SAMPLE) {
				return SqDistApproximant::lineDistance(m_bounds.first);
			} else if (flags & FittableSpline::TAIL_SAMPLE) {
				return SqDistApproximant::lineDistance(m_bounds.second);
			}

			double const p = m_polylineProj.projectionScalar(pt);
			if (p < 0 || p > 1) {
				// Sample past the snake's endpoints.
				if (flags & FittableSpline::JUNCTION_SAMPLE) {
					// It's too dangerous to let a junction sample (which corresponds
					// to a control point) past the snake endpoints bounds, where little or
					// no influence will be applied to it.  Therefore, attract it to the
					// corresponding snake endpoint.
					return SqDistApproximant::pointDistance(p < 0 ? m_headPoint : m_tailPoint);
				} else {
					//Just ignore it.
					return SqDistApproximant();
				}
			}

			// Delegate to the parent class.
			return PolylineModelShape::localSqDistApproximant(pt, FittableSpline::DEFAULT_SAMPLE);
		}
	private:
		QPointF m_headPoint;
		QPointF m_tailPoint;
		ToLineProjector m_polylineProj;
		std::pair<QLineF, QLineF> m_bounds;
	};

	QLineF const chord(polyline.front(), polyline.back());
	XSpline spline;
	for (int i = 0; i < 5; ++i) {
		spline.appendControlPoint(chord.pointAt(i / 4.0), 1);
	}

	ModelShape const model_shape(polyline, bounds);
	SplineFitter fitter(&spline, &model_shape);
	fitter.fit();

	return spline;
}


/*============================== RansacAlgo ============================*/

void
DistortionModelBuilder::RansacAlgo::buildAndAssessModel(
	TracedCurve const* top_curve, TracedCurve const* bottom_curve)
try {
	DistortionModel model;
	model.setTopCurve(Curve(top_curve->extendedPolyline));
	model.setBottomCurve(Curve(bottom_curve->extendedPolyline));
	if (!model.isValid()) {
		return;
	}

	double const depth_perception = 2.0; // Doesn't matter much here.
	CylindricalSurfaceDewarper const dewarper(
		top_curve->extendedPolyline, bottom_curve->extendedPolyline, depth_perception
	);

	double error = 0;
	BOOST_FOREACH(TracedCurve const& curve, m_rAllCurves) {
		size_t const polyline_size = curve.trimmedPolyline.size();
		double const r_reference_height = 1.0 / 1.0; //calcReferenceHeight(dewarper, curve.centroid);

		// We are going to approximate the dewarped polyline by a straight line
		// using linear least-squares: At*A*x = At*B -> x = (At*A)-1 * At*B
		std::vector<double> At;
		At.reserve(polyline_size * 2);
		std::vector<double> B;
		B.reserve(polyline_size);

		BOOST_FOREACH(QPointF const& warped_pt, curve.trimmedPolyline) {
			// TODO: add another signature with hint for efficiency.
			QPointF const dewarped_pt(dewarper.mapToDewarpedSpace(warped_pt));

			// ax + b = y  <-> x * a + 1 * b = y 
			At.push_back(dewarped_pt.x());
			At.push_back(1);
			B.push_back(dewarped_pt.y());
		}

		DynamicMatrixCalc<double> mc;
		
		// A = Att
		boost::scoped_array<double> A(new double[polyline_size * 2]);
		mc(&At[0], 2, polyline_size).transWrite(&A[0]);

		try {
			boost::scoped_array<double> errvec(new double[polyline_size]);
			double ab[2]; // As in "y = ax + b".

			// errvec = B - A * (At*A)-1 * At * B
			// ab = (At*A)-1 * At * B
			(
				mc(&B[0], polyline_size, 1) - mc(&A[0], polyline_size, 2)
				*((mc(&At[0], 2, polyline_size)*mc(&A[0], polyline_size, 2)).inv()
				*(mc(&At[0], 2, polyline_size)*mc(&B[0], polyline_size, 1))).write(ab)
			).write(&errvec[0]);

			double sum_abs_err = 0;
			for (size_t i = 0; i < polyline_size; ++i) {
				sum_abs_err += fabs(errvec[i]) * r_reference_height;
			}

			// Penalty for not being straight.
			error += sum_abs_err / polyline_size;

			// TODO: penalty for not being horizontal.
		} catch (std::runtime_error const&) {
			// Strictly vertical line?
			error += 1000;
		}
	}

	if (error < m_bestModel.totalError) {
		m_bestModel.topCurve = top_curve;
		m_bestModel.bottomCurve = bottom_curve;
		m_bestModel.totalError = error;
	}
} catch (std::runtime_error const&) {
	// Probably CylindricalSurfaceDewarper didn't like something.
}
#if 0
double
DistortionModelBuilder::RansacAlgo::calcReferenceHeight(
	CylindricalSurfaceDewarper const& dewarper, QPointF const& loc)
{
	// TODO: ideally, we would use the counterpart of CylindricalSurfaceDewarper::mapGeneratrix(),
	// that would map it the other way, and which doesn't currently exist.

	QPointF const pt1(dewarper.mapToDewarpedSpace(loc + QPointF(0.0, -10)));
	QPointF const pt2(dewarper.mapToDewarpedSpace(loc + QPointF(0.0, 10)));
	return fabs(pt1.y() - pt2.y());
}
#endif
QImage
DistortionModelBuilder::visualizeModel(
	QImage const& background, std::vector<TracedCurve> const& curves, RansacModel const& model) const
{
	QImage canvas(background.convertToFormat(QImage::Format_RGB32));
	QPainter painter(&canvas);
	painter.setRenderHint(QPainter::Antialiasing);

	int const width = background.width();
	int const height = background.height();
	double const stroke_width = sqrt(double(width * width + height * height)) / 500;

	QPen active_curve_pen(QColor(0x45, 0xff, 0x53, 180));
	active_curve_pen.setWidthF(stroke_width);
	
	QPen inactive_curve_pen(QColor(0, 0, 255, 140));
	inactive_curve_pen.setWidthF(stroke_width);

	BOOST_FOREACH(TracedCurve const& curve, curves) {
		if (curve.extendedPolyline.empty()) {
			continue;
		}
		if (&curve == model.topCurve || &curve == model.bottomCurve) {
			painter.setPen(active_curve_pen);
		} else {
			painter.setPen(inactive_curve_pen);
		}
		painter.drawPolyline(&curve.extendedPolyline[0], curve.extendedPolyline.size());
	}
	
	// Extend / trim bounds.
	QLineF bound1(m_bound1);
	QLineF bound2(m_bound2);
	lineBoundedByRect(bound1, background.rect());
	lineBoundedByRect(bound2, background.rect());

	// Draw bounds.
	QPen bounds_pen(QColor(0, 0, 255, 180));
	bounds_pen.setWidthF(stroke_width);
	painter.setPen(bounds_pen);
	painter.drawLine(bound1);
	painter.drawLine(bound2);

	return canvas;
}

} // namespace dewarping
