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
#include "OutputMargins.h"
#include "Curve.h"
#include "VecNT.h"
#include "MatrixCalc.h"
#include "NumericTraits.h"
#include "ToLineProjector.h"
#include "XSpline.h"
#include "CylindricalSurfaceDewarper.h"
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
#include <boost/array.hpp>
#include <boost/bind.hpp>
#include <boost/foreach.hpp>
#include <vector>

namespace output
{

using namespace imageproc;

DewarpingView::DewarpingView(
	QImage const& image, ImagePixmapUnion const& downscaled_image,
	QTransform const& image_to_virt, QPolygonF const& virt_display_area,
	QRectF const& virt_content_rect, PageId const& page_id,
	IntrusivePtr<Settings> const& settings,
	DistortionModel const& distortion_model,
	DepthPerception const& depth_perception)
:	ImageViewBase(
		image, downscaled_image,
		ImagePresentation(image_to_virt, virt_display_area),
		OutputMargins()
	),
	m_pageId(page_id),
	m_virtDisplayArea(virt_display_area),
	m_distortionModel(distortion_model),
	m_depthPerception(depth_perception),
	m_ptrSettings(settings),
	m_dragHandler(*this),
	m_zoomHandler(*this)	
{
	setMouseTracking(true);

	QPolygonF const source_content_rect(virtualToImage().map(virt_content_rect));

	XSpline top_spline(m_distortionModel.topCurve().xspline());
	XSpline bottom_spline(m_distortionModel.bottomCurve().xspline());
	if (top_spline.numControlPoints() < 2) {
		QLineF const top_line(source_content_rect[0], source_content_rect[1]);
		XSpline new_top_spline;

		new_top_spline.appendControlPoint(top_line.p1(), 0);
		new_top_spline.appendControlPoint(top_line.pointAt(1.0/3.0), 1);
		new_top_spline.appendControlPoint(top_line.pointAt(2.0/3.0), 1);
		new_top_spline.appendControlPoint(top_line.p2(), 0);

		top_spline.swap(new_top_spline);
	}
	if (bottom_spline.numControlPoints() < 2) {
		QLineF const bottom_line(source_content_rect[3], source_content_rect[2]);
		XSpline new_bottom_spline;

		new_bottom_spline.appendControlPoint(bottom_line.p1(), 0);
		new_bottom_spline.appendControlPoint(bottom_line.pointAt(1.0/3.0), 1);
		new_bottom_spline.appendControlPoint(bottom_line.pointAt(2.0/3.0), 1);
		new_bottom_spline.appendControlPoint(bottom_line.p2(), 0);

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

	m_distortionModel.setTopCurve(Curve(m_topSpline.spline()));
	m_distortionModel.setBottomCurve(Curve(m_bottomSpline.spline()));

	rootInteractionHandler().makeLastFollower(*this);
	rootInteractionHandler().makeLastFollower(m_dragHandler);
	rootInteractionHandler().makeLastFollower(m_zoomHandler);
}

DewarpingView::~DewarpingView()
{
}

void
DewarpingView::depthPerceptionChanged(double val)
{
	m_depthPerception.setValue(val);
	m_ptrSettings->setDepthPerception(m_pageId, m_depthPerception);
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

	QPen blue_strokes(QColor(0x00, 0x00, 0xff));
	blue_strokes.setCosmetic(true);
	blue_strokes.setWidthF(1.2);

	painter.setPen(blue_strokes);
	painter.setBrush(Qt::NoBrush);
	
	int const num_vert_grid_lines = 30;
	int const num_hor_grid_lines = 30;
	std::vector<QVector<QPointF> > curves(num_hor_grid_lines);

	CylindricalSurfaceDewarper dewarper(
		m_distortionModel.topCurve().polyline(),
		m_distortionModel.bottomCurve().polyline(), m_depthPerception.value()
	);
	CylindricalSurfaceDewarper::State state;

	for (int j = 0; j < num_vert_grid_lines; ++j) {
		double const x = j / (num_vert_grid_lines - 1.0);
		CylindricalSurfaceDewarper::Generatrix const gtx(dewarper.mapGeneratrix(x, state));
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

	paintXSpline(painter, interaction, m_topSpline);
	paintXSpline(painter, interaction, m_bottomSpline);
#if 0
	painter.setWorldTransform(QTransform());

	std::vector<QPointF> data_points;
	for (int i = 0; i < 36; ++i) {
		data_points.push_back(QPointF(200 + 100*cos(i*10*constants::DEG2RAD), 200 + 100*sin(i*10*constants::DEG2RAD)));
	}
	painter.drawPolyline(QVector<QPointF>::fromStdVector(data_points));

	XSpline xspline;
	xspline.appendControlPoint(data_points.front(), 0);
	for (int i = 0; i < 5; ++i) {
		xspline.appendControlPoint(QPointF(0, 0), 1);
	}
	xspline.appendControlPoint(data_points.back(), 0);

	boost::dynamic_bitset<> fixed_points(xspline.numControlPoints());
	fixed_points.set(0);
	fixed_points.set(fixed_points.size() - 1);
	xspline.fit(data_points, &fixed_points);

	painter.drawPolyline(QVector<QPointF>::fromStdVector(xspline.toPolyline()));

	QPen point_pen(Qt::red);
	point_pen.setWidthF(4.0);
	point_pen.setCosmetic(true);
	painter.setPen(point_pen);

	for (int i = 0; i < xspline.numControlPoints(); ++i) {
		painter.drawPoint(xspline.controlPointPosition(i));
	}
#endif
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
		m_distortionModel.setTopCurve(Curve(m_topSpline.spline()));
	} else {
		m_distortionModel.setBottomCurve(Curve(m_bottomSpline.spline()));
	}
	update();
}

void
DewarpingView::dragFinished()
{
	m_ptrSettings->setDistortionModel(m_pageId, m_distortionModel);
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
	XSpline const& top_spline = m_topSpline.spline();
	XSpline const& bottom_spline = m_bottomSpline.spline();
	
	QLineF vert_boundary; // From top to bottom, that's important!

	if (margin_idx == 0) { // Left margin.
		vert_boundary.setP1(top_spline.eval(0));
		vert_boundary.setP2(bottom_spline.eval(0));
	} else { // Right margin.
		vert_boundary.setP1(top_spline.eval(1));
		vert_boundary.setP2(bottom_spline.eval(1));
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
