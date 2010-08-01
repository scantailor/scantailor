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
#include "imageproc/CubicBSpline.h"
#include "imageproc/MatrixCalc.h"
#include "imageproc/CylindricalSurfaceDewarper.h"
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
	m_distortionModel(distortion_model),
	m_depthPerception(depth_perception),
	m_ptrSettings(settings),
	m_dragHandler(*this),
	m_zoomHandler(*this)
	
{
	setMouseTracking(true);

	QPolygonF const source_content_rect(virtualToImage().map(virt_content_rect));

	double const step = 1.0 / 3.0;
	CubicBSpline top_spline(m_distortionModel.topCurve().bspline());
	CubicBSpline bottom_spline(m_distortionModel.bottomCurve().bspline());
	if (!top_spline.isValid()) {
		CubicBSpline().swap(top_spline);
		top_spline.makeValid();

		QLineF const top_line(source_content_rect[0], source_content_rect[1]);
		
		boost::array<QPointF, 4> top_bezier_segment;
		for (int i = 0; i < 4; ++i) {
			top_bezier_segment[i] = top_line.pointAt(i * step);
		}

		top_spline.setBezierSegment(0, top_bezier_segment);
	}
	if (!bottom_spline.isValid()) {
		CubicBSpline().swap(bottom_spline);
		bottom_spline.makeValid();

		QLineF const bottom_line(source_content_rect[3], source_content_rect[2]);

		boost::array<QPointF, 4> bottom_bezier_segment;
		for (int i = 0; i < 4; ++i) {
			bottom_bezier_segment[i] = bottom_line.pointAt(i * step);
		}

		bottom_spline.setBezierSegment(0, bottom_bezier_segment);
	}
	
	m_topSpline.setSpline(top_spline);
	m_bottomSpline.setSpline(bottom_spline);
	
	InteractiveBSpline* splines[2] = { &m_topSpline, &m_bottomSpline };
	int curve_idx = -1;
	BOOST_FOREACH(InteractiveBSpline* spline, splines) {
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
	painter.setWorldTransform(imageToVirtual() * painter.worldTransform());
	painter.setRenderHint(QPainter::Antialiasing);
	painter.setBrush(Qt::NoBrush);

	QPen blue_strokes(QColor(0x00, 0x00, 0xff));
	blue_strokes.setCosmetic(true);
	blue_strokes.setWidthF(1.5);
	painter.setPen(blue_strokes);
	
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

	paintBSpline(painter, interaction, m_topSpline);
	paintBSpline(painter, interaction, m_bottomSpline);
}

void
DewarpingView::paintBSpline(
	QPainter& painter, InteractionState const& interaction,
	InteractiveBSpline const& ispline)
{
	CubicBSpline const& spline = ispline.spline();
	assert(spline.isValid());

	painter.save();
	painter.setWorldTransform(imageToVirtual() * virtualToWidget());

	int const num_segments = spline.numSegments();
	std::vector<QPointF> bezier_points;
	bezier_points.reserve(num_segments * 4);

	QPainterPath path;
	
	// Process the first point.
	boost::array<QPointF, 4> bezier_segment(spline.toBezierSegment(0));
	path.moveTo(bezier_segment[0]);
	bezier_points.push_back(bezier_segment[0]);

	QPen tangent_pen(QColor(0x0e0061));
	tangent_pen.setWidthF(1.0);
	tangent_pen.setCosmetic(true);
	painter.setPen(tangent_pen);

	// Loop over segments.
	for (int seg = 0;;) {
		path.cubicTo(bezier_segment[1], bezier_segment[2], bezier_segment[3]);
		bezier_points.push_back(bezier_segment[1]);
		bezier_points.push_back(bezier_segment[2]);
		bezier_points.push_back(bezier_segment[3]);

		painter.drawLine(bezier_segment[0], bezier_segment[1]);
		painter.drawLine(bezier_segment[2], bezier_segment[3]);

		if (++seg >= num_segments) {
			break;
		}
		bezier_segment = spline.toBezierSegment(seg);
	}

	QPen curve_pen(Qt::blue);
	curve_pen.setWidthF(1.5);
	curve_pen.setCosmetic(true);
	painter.setPen(curve_pen);
	painter.drawPath(path);

	// Drawing cosmetic points in transformed coordinates seems unreliable,
	// so let's draw them in widget coordinates.
	painter.setWorldMatrixEnabled(false);

	QPen point_pen(Qt::red);
	point_pen.setWidthF(4.0);
	point_pen.setCosmetic(true);
	painter.setPen(point_pen);

	BOOST_FOREACH(QPointF const& pt, bezier_points) {
		painter.drawPoint(sourceToWidget(pt));
	}

	QPointF pt;
	if (ispline.curveHighlighted(interaction, &pt)) {
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
#if 0
	qDebug() << "top_spline:";
	for (int i = 0; i < 4; ++i) {
		qDebug() << m_topSpline.controlPointPosition(i);
	}

	qDebug() << "bottom_spline:";
	for (int i = 0; i < 4; ++i) {
		qDebug() << m_bottomSpline.controlPointPosition(i);
	}
#endif
}

QPointF
DewarpingView::sourceToWidget(QPointF const& pt) const
{
	return virtualToWidget().map(imageToVirtual().map(pt));
}

QPointF
DewarpingView::widgetToSource(QPointF const& pt) const
{
	return virtualToImage().map(widgetToVirtual().map(pt));
}

} // namespace output
