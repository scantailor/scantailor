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

#include "DewarpingView.h"
#include "DewarpingView.h.moc"
#include "ImagePresentation.h"
#include "dewarping/Curve.h"
#include "VecNT.h"
#include "MatrixCalc.h"
#include "NumericTraits.h"
#include "ToLineProjector.h"
#include "XSpline.h"
#include "dewarping/CylindricalSurfaceDewarper.h"
#include "dewarping/Curve.h"
#include "spfit/SplineFitter.h"
#include "spfit/ConstraintSet.h"
#include "spfit/PolylineModelShape.h"
#include "spfit/LinearForceBalancer.h"
#include "spfit/OptimizationResult.h"
#include "imageproc/Constants.h"
#include <QCursor>
#include <QLineF>
#include <QPolygonF>
#include <QTransform>
#include <QPainter>
#include <QPainterPath>
#include <QPen>
#include <QColor>
#include <Qt>
#include <QDebug>
#ifndef Q_MOC_RUN
#include <boost/array.hpp>
#include <boost/bind.hpp>
#include <boost/foreach.hpp>
#endif
#include <vector>
#include <stdexcept>

namespace output
{

using namespace imageproc;

DewarpingView::DewarpingView(
	QImage const& image, ImagePixmapUnion const& downscaled_image,
	QTransform const& image_to_virt, QPolygonF const& virt_display_area,
	QRectF const& virt_content_rect, PageId const& page_id,
	DewarpingMode dewarping_mode,
	dewarping::DistortionModel const& distortion_model,
	DepthPerception const& depth_perception)
:	ImageViewBase(
		image, downscaled_image,
		ImagePresentation(image_to_virt, virt_display_area)
	),
	m_pageId(page_id),
	m_virtDisplayArea(virt_display_area),
	m_dewarpingMode(dewarping_mode),
	m_distortionModel(distortion_model),
	m_depthPerception(depth_perception),
	m_dragHandler(*this),
	m_zoomHandler(*this)	
{
	setMouseTracking(true);
	
	QPolygonF const source_content_rect(virtualToImage().map(virt_content_rect));

	XSpline top_spline(m_distortionModel.topCurve().xspline());
	XSpline bottom_spline(m_distortionModel.bottomCurve().xspline());
	if (top_spline.numControlPoints() < 2) {
		std::vector<QPointF> const& polyline = m_distortionModel.topCurve().polyline();
		
		XSpline new_top_spline;
		if (polyline.size() < 2) {
			initNewSpline(new_top_spline, source_content_rect[0], source_content_rect[1]);
		} else {
			initNewSpline(new_top_spline, polyline.front(), polyline.back());
			fitSpline(new_top_spline, polyline);
		}

		top_spline.swap(new_top_spline);
	}
	if (bottom_spline.numControlPoints() < 2) {
		std::vector<QPointF> const& polyline = m_distortionModel.bottomCurve().polyline();

		XSpline new_bottom_spline;
		if (polyline.size() < 2) {
			initNewSpline(new_bottom_spline, source_content_rect[3], source_content_rect[2]);
		} else {
			initNewSpline(new_bottom_spline, polyline.front(), polyline.back());
			fitSpline(new_bottom_spline, polyline);
		}

		bottom_spline.swap(new_bottom_spline);
	}

	m_topSpline.setSpline(top_spline);
	m_bottomSpline.setSpline(bottom_spline);
	
	InteractiveXSpline* splines[2] = { &m_topSpline, &m_bottomSpline };
	int curve_idx = -1;
	BOOST_FOREACH(InteractiveXSpline* spline, splines) {
		++curve_idx;
		spline->setModifiedCallback(boost::bind(&DewarpingView::curveModified, this, curve_idx));
		spline->setDragFinishedCallback(boost::bind(&DewarpingView::dragFinished, this));
		spline->setStorageTransform(
			boost::bind(&DewarpingView::sourceToWidget, this, _1),
			boost::bind(&DewarpingView::widgetToSource, this, _1)
		);
		makeLastFollower(*spline);
	}

	m_distortionModel.setTopCurve(dewarping::Curve(m_topSpline.spline()));
	m_distortionModel.setBottomCurve(dewarping::Curve(m_bottomSpline.spline()));

	rootInteractionHandler().makeLastFollower(*this);
	rootInteractionHandler().makeLastFollower(m_dragHandler);
	rootInteractionHandler().makeLastFollower(m_zoomHandler);
}

DewarpingView::~DewarpingView()
{
}

void
DewarpingView::initNewSpline(XSpline& spline, QPointF const& p1, QPointF const& p2)
{
	QLineF const line(p1, p2);
	spline.appendControlPoint(line.p1(), 0);
	spline.appendControlPoint(line.pointAt(1.0/4.0), 1);
	spline.appendControlPoint(line.pointAt(2.0/4.0), 1);
	spline.appendControlPoint(line.pointAt(3.0/4.0), 1);
	spline.appendControlPoint(line.p2(), 0);
}

void
DewarpingView::fitSpline(XSpline& spline, std::vector<QPointF> const& polyline)
{
	using namespace spfit;

	SplineFitter fitter(&spline);
	PolylineModelShape const model_shape(polyline);

	ConstraintSet constraints(&spline);
	constraints.constrainSplinePoint(0.0, polyline.front());
	constraints.constrainSplinePoint(1.0, polyline.back());
	fitter.setConstraints(constraints);

	FittableSpline::SamplingParams sampling_params;
	sampling_params.maxDistBetweenSamples = 10;
	fitter.setSamplingParams(sampling_params);

	int iterations_remaining = 20;
	LinearForceBalancer balancer(0.8);
	balancer.setTargetRatio(0.1);
	balancer.setIterationsToTarget(iterations_remaining - 1);

	for (; iterations_remaining > 0; --iterations_remaining, balancer.nextIteration()) {
		fitter.addAttractionForces(model_shape);
		fitter.addInternalForce(spline.controlPointsAttractionForce());

		double internal_force_weight = balancer.calcInternalForceWeight(
			fitter.internalForce(), fitter.externalForce()
		);
		OptimizationResult const res(fitter.optimize(internal_force_weight));
		if (dewarping::Curve::splineHasLoops(spline)) {
			fitter.undoLastStep();
			break;
		}

		if (res.improvementPercentage() < 0.5) {
			break;
		}
	}
}

void
DewarpingView::depthPerceptionChanged(double val)
{
	m_depthPerception.setValue(val);
	update();
}

void
DewarpingView::onPaint(QPainter& painter, InteractionState const& interaction)
{
	painter.setRenderHint(QPainter::Antialiasing);

	painter.setPen(Qt::NoPen);
	painter.setBrush(QColor(0xff, 0xff, 0xff, 150)); // Translucent white.
	painter.drawPolygon(virtMarginArea(0)); // Left margin.
	painter.drawPolygon(virtMarginArea(1)); // Right margin.

	painter.setWorldTransform(imageToVirtual() * painter.worldTransform());
	painter.setBrush(Qt::NoBrush);

	QPen grid_pen;
	grid_pen.setColor(Qt::blue);
	grid_pen.setCosmetic(true);
	grid_pen.setWidthF(1.2);

	painter.setPen(grid_pen);
	painter.setBrush(Qt::NoBrush);
	
	int const num_vert_grid_lines = 30;
	int const num_hor_grid_lines = 30;

	bool valid_model = m_distortionModel.isValid();

	if (valid_model) {
		try {
			std::vector<QVector<QPointF> > curves(num_hor_grid_lines);

			dewarping::CylindricalSurfaceDewarper dewarper(
				m_distortionModel.topCurve().polyline(),
				m_distortionModel.bottomCurve().polyline(), m_depthPerception.value()
			);
			dewarping::CylindricalSurfaceDewarper::State state;

			for (int j = 0; j < num_vert_grid_lines; ++j) {
				double const x = j / (num_vert_grid_lines - 1.0);
				dewarping::CylindricalSurfaceDewarper::Generatrix const gtx(dewarper.mapGeneratrix(x, state));
				QPointF const gtx_p0(gtx.imgLine.pointAt(gtx.pln2img(0)));
				QPointF const gtx_p1(gtx.imgLine.pointAt(gtx.pln2img(1)));
				painter.drawLine(gtx_p0, gtx_p1);
				for (int i = 0; i < num_hor_grid_lines; ++i) {
					double const y = i / (num_hor_grid_lines - 1.0);
					curves[i].push_back(gtx.imgLine.pointAt(gtx.pln2img(y)));
				}
			}

			BOOST_FOREACH(QVector<QPointF> const& curve, curves) {
				painter.drawPolyline(curve);
			}
		} catch (std::runtime_error const&) {
			// Still probably a bad model, even though DistortionModel::isValid() was true.
			valid_model = false;
		}
	} // valid_model

	if (!valid_model) {
		// Just draw the frame.
		dewarping::Curve const& top_curve = m_distortionModel.topCurve();
		dewarping::Curve const& bottom_curve = m_distortionModel.bottomCurve();
		painter.drawLine(top_curve.polyline().front(), bottom_curve.polyline().front());
		painter.drawLine(top_curve.polyline().back(), bottom_curve.polyline().back());
		painter.drawPolyline(QVector<QPointF>::fromStdVector(top_curve.polyline()));
		painter.drawPolyline(QVector<QPointF>::fromStdVector(bottom_curve.polyline()));
	}

	paintXSpline(painter, interaction, m_topSpline);
	paintXSpline(painter, interaction, m_bottomSpline);
}

void
DewarpingView::paintXSpline(
	QPainter& painter, InteractionState const& interaction,
	InteractiveXSpline const& ispline)
{
	XSpline const& spline = ispline.spline();

	painter.save();
	painter.setBrush(Qt::NoBrush);

#if 0 // No point in drawing the curve itself - we already draw the grid.
	painter.setWorldTransform(imageToVirtual() * virtualToWidget());
	
	QPen curve_pen(Qt::blue);
	curve_pen.setWidthF(1.5);
	curve_pen.setCosmetic(true);
	painter.setPen(curve_pen);
	
	std::vector<QPointF> const polyline(spline.toPolyline());
	painter.drawPolyline(&polyline[0], polyline.size());
#endif

	// Drawing cosmetic points in transformed coordinates seems unreliable,
	// so let's draw them in widget coordinates.
	painter.setWorldMatrixEnabled(false);

	QPen existing_point_pen(Qt::red);
	existing_point_pen.setWidthF(4.0);
	existing_point_pen.setCosmetic(true);
	painter.setPen(existing_point_pen);

	int const num_control_points = spline.numControlPoints();
	for (int i = 0; i < num_control_points; ++i) {
		painter.drawPoint(sourceToWidget(spline.controlPointPosition(i)));
	}

	QPointF pt;
	if (ispline.curveIsProximityLeader(interaction, &pt)) {
		QPen new_point_pen(existing_point_pen);
		new_point_pen.setColor(QColor(0x00ffff));
		painter.setPen(new_point_pen);
		painter.drawPoint(pt);
	}

	painter.restore();
}

void
DewarpingView::curveModified(int curve_idx)
{
	if (curve_idx == 0) {
		m_distortionModel.setTopCurve(dewarping::Curve(m_topSpline.spline()));
	} else {
		m_distortionModel.setBottomCurve(dewarping::Curve(m_bottomSpline.spline()));
	}
	update();
}

void
DewarpingView::dragFinished()
{
	if (m_dewarpingMode == DewarpingMode::AUTO) {
		m_dewarpingMode = DewarpingMode::MANUAL;
	}
	emit distortionModelChanged(m_distortionModel);
}

/** Source image coordinates to widget coordinates. */
QPointF
DewarpingView::sourceToWidget(QPointF const& pt) const
{
	return virtualToWidget().map(imageToVirtual().map(pt));
}

/** Widget coordinates to source image coordinates. */
QPointF
DewarpingView::widgetToSource(QPointF const& pt) const
{
	return virtualToImage().map(widgetToVirtual().map(pt));
}

QPolygonF
DewarpingView::virtMarginArea(int margin_idx) const
{
	dewarping::Curve const& top_curve = m_distortionModel.topCurve();
	dewarping::Curve const& bottom_curve = m_distortionModel.bottomCurve();
	
	QLineF vert_boundary; // From top to bottom, that's important!

	if (margin_idx == 0) { // Left margin.
		vert_boundary.setP1(top_curve.polyline().front());
		vert_boundary.setP2(bottom_curve.polyline().front());
	} else { // Right margin.
		vert_boundary.setP1(top_curve.polyline().back());
		vert_boundary.setP2(bottom_curve.polyline().back());
	}

	vert_boundary = imageToVirtual().map(vert_boundary);
	
	QLineF normal;
	if (margin_idx == 0) { // Left margin.
		normal = QLineF(vert_boundary.p2(), vert_boundary.p1()).normalVector();
	} else { // Right margin.
		normal = vert_boundary.normalVector();
	}
	
	// Project every vertex in the m_virtDisplayArea polygon
	// to vert_line and to its normal, keeping track min and max values. 
	double min = NumericTraits<double>::max();
	double max = NumericTraits<double>::min();
	double normal_max = max;
	ToLineProjector const vert_line_projector(vert_boundary);
	ToLineProjector const normal_projector(normal);
	BOOST_FOREACH(QPointF const& pt, m_virtDisplayArea) {
		double const p1 = vert_line_projector.projectionScalar(pt);
		if (p1 < min) {
			min = p1;
		}
		if (p1 > max) {
			max = p1;
		}

		double const p2 = normal_projector.projectionScalar(pt);
		if (p2 > normal_max) {
			normal_max = p2;
		}
	}

	// Workaround clipping bugs in QPolygon::intersected().
	min -= 1.0;
	max += 1.0;
	normal_max += 1.0;

	QPolygonF poly;
	poly << vert_boundary.pointAt(min);
	poly << vert_boundary.pointAt(max);
	poly << vert_boundary.pointAt(max) + normal.pointAt(normal_max) - normal.p1();
	poly << vert_boundary.pointAt(min) + normal.pointAt(normal_max) - normal.p1();

	return m_virtDisplayArea.intersected(poly);
}

} // namespace output
