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
#include "VecNT.h"
#include "imageproc/XSpline.h"
#include "imageproc/MatrixCalc.h"
#include "imageproc/CylindricalSurfaceDewarper.h"
#include <QBitmap>
#include <QCursor>
#include <QLineF>
#include <QPolygonF>
#include <QTransform>
#include <QPainter>
#include <QPen>
#include <QColor>
#include <Qt>
#include <boost/bind.hpp>
#include <boost/foreach.hpp>
#include <vector>

namespace output
{

using namespace imageproc;

DewarpingView::DewarpingView(
	QImage const& image, ImagePixmapUnion const& downscaled_image,
	QTransform const& source_to_virt, QRectF const& virt_content_rect)
:	ImageViewBase(
		image, downscaled_image,
		ImagePresentation(QTransform(), QRectF(image.rect())),
		OutputMargins()
	),
	m_sourceToVirt(source_to_virt),
	m_virtToSource(source_to_virt.inverted()),
	m_dragHandler(*this),
	m_zoomHandler(*this)
{
	setMouseTracking(true);

	QPolygonF const source_content_rect(m_virtToSource.map(virt_content_rect));
	
	QLineF const top_line(source_content_rect[0], source_content_rect[1]);
	m_topSpline.appendControlPoint(top_line.p1(), 0);
	m_topSpline.appendControlPoint(top_line.pointAt(1.0/3.0), 1);
	m_topSpline.appendControlPoint(top_line.pointAt(2.0/3.0), 1);
	m_topSpline.appendControlPoint(top_line.p2(), 0);
	
	QLineF const bottom_line(source_content_rect[3], source_content_rect[2]);
	m_bottomSpline.appendControlPoint(bottom_line.p1(), 0);
	m_bottomSpline.appendControlPoint(bottom_line.pointAt(1.0/3.0), 1);
	m_bottomSpline.appendControlPoint(bottom_line.pointAt(2.0/3.0), 1);
	m_bottomSpline.appendControlPoint(bottom_line.p2(), 0);

	for (int i = 0; i < 2; ++i) {
		for (int j = 0; j < 4; ++j) {
			m_curvePoints[i][j].setPositionCallback(
				boost::bind(&DewarpingView::curvePointPosition, this, i, j)
			);
			m_curvePoints[i][j].setMoveRequestCallback(
				boost::bind(&DewarpingView::curvePointMoveRequest, this, i, j, _1)
			);
			m_curvePoints[i][j].setDragFinishedCallback(
				boost::bind(&DewarpingView::dragFinished, this)
			);

			//m_curvePointInteractors[i][j].setProximityStatusTip(tip);
			QBitmap zero_bitmap(32, 32);
			zero_bitmap.fill(Qt::color0);
			m_curvePointInteractors[i][j].setInteractionCursor(QCursor(zero_bitmap, zero_bitmap));
			m_curvePointInteractors[i][j].setObject(&m_curvePoints[i][j]);

			makeLastFollower(m_curvePointInteractors[i][j]);
		}
	}

	rootInteractionHandler().makeLastFollower(*this);
	rootInteractionHandler().makeLastFollower(m_dragHandler);
	rootInteractionHandler().makeLastFollower(m_zoomHandler);
}

DewarpingView::~DewarpingView()
{
}

void
DewarpingView::onPaint(QPainter& painter, InteractionState const& interaction)
{
	painter.setWorldTransform(m_sourceToVirt * painter.worldTransform());
	painter.setRenderHint(QPainter::Antialiasing);
	painter.setBrush(Qt::NoBrush);

	QPen blue_strokes(QColor(0x00, 0x00, 0xff));
	blue_strokes.setCosmetic(true);
	blue_strokes.setWidthF(1.5);
	painter.setPen(blue_strokes);
	
	int const num_vert_grid_lines = 30;
	int const num_hor_grid_lines = 30;
	std::vector<QVector<QPointF> > curves(num_hor_grid_lines);

	CylindricalSurfaceDewarper dewarper(m_topSpline.toPolyline(), m_bottomSpline.toPolyline(), 3.5);
	CylindricalSurfaceDewarper::State state;

	for (int j = 0; j < num_vert_grid_lines; ++j) {
		double const x = j / (num_vert_grid_lines - 1.0);
		CylindricalSurfaceDewarper::Generatrix const gtx(dewarper.mapGeneratrix(x, state));
		QPointF const gtx_p0(gtx.imgLine.pointAt(gtx.pln2img(Vec1d(0))[0]));
		QPointF const gtx_p1(gtx.imgLine.pointAt(gtx.pln2img(Vec1d(1))[0]));
		painter.drawLine(gtx_p0, gtx_p1);
		for (int i = 0; i < num_hor_grid_lines; ++i) {
			double const y = i / (num_hor_grid_lines - 1.0);
			curves[i].push_back(gtx.imgLine.pointAt(gtx.pln2img(Vec1d(y))[0]));
		}
	}

	BOOST_FOREACH(QVector<QPointF> const& curve, curves) {
		painter.drawPolyline(curve);
	}

	QPen red_dots(QColor(0xff, 0x00, 0x00));
	red_dots.setCosmetic(true);
	red_dots.setWidthF(3.0);
	painter.setPen(red_dots);

	// Drawing cosmetic points in transformed coordinates seems unreliable,
	// so let's draw them in widget coordinates.
	painter.setWorldTransform(QTransform());

	for (int i = 0; i < m_topSpline.numControlPoints(); ++i) {
		QPointF const source_pt(m_topSpline.controlPointPosition(i));
		QPointF const virt_pt(m_sourceToVirt.map(source_pt));
		QPointF const widget_pt(virtualToWidget().map(virt_pt));
		painter.drawPoint(widget_pt);
	}
	for (int i = 0; i < m_bottomSpline.numControlPoints(); ++i) {
		QPointF const source_pt(m_bottomSpline.controlPointPosition(i));
		QPointF const virt_pt(m_sourceToVirt.map(source_pt));
		QPointF const widget_pt(virtualToWidget().map(virt_pt));
		painter.drawPoint(widget_pt);
	}
}

QPointF
DewarpingView::curvePointPosition(int curve_idx, int point_idx) const
{
	XSpline const& spline = curve_idx == 0 ? m_topSpline : m_bottomSpline;
	QPointF const source_pt(spline.controlPointPosition(point_idx));
	QPointF const virt_pt(m_sourceToVirt.map(source_pt));
	QPointF const widget_pt(virtualToWidget().map(virt_pt));
	return widget_pt;
}

static Vec4d rotationAndScale(QPointF const& from, QPointF const& to)
{
	Vec4d A;
	A[0] = from.x();
	A[1] = from.y();
	A[2] = from.y();
	A[3] = -from.x();

	Vec2d B(to.x(), to.y());

	Vec2d x;
	MatrixCalc<double> mc;
	mc(2, 2, A).solve(mc(2, 1, B)).write(x);
	
	A[0] = x[0];
	A[1] = -x[1];
	A[2] = x[1];
	A[3] = x[0];
	return A;
}

void
DewarpingView::curvePointMoveRequest(int curve_idx, int point_idx, QPointF const& widget_pos)
{
	QPointF const virt_pos(widgetToVirtual().map(widget_pos));
	QPointF const source_pos(m_virtToSource.map(virt_pos));

	XSpline& spline = curve_idx == 0 ? m_topSpline : m_bottomSpline;
	int const num_control_points = spline.numControlPoints();
	if (point_idx > 0 && point_idx < num_control_points - 1) {
		spline.moveControlPoint(point_idx, source_pos);
	} else {
		std::vector<QPointF> points;
		for (int i = 0; i < num_control_points; ++i) {
			points.push_back(spline.controlPointPosition(i));
		}
		QPointF const origin(point_idx == 0 ? points.back() : points.front());
		QPointF const handle(point_idx == 0 ? points.front() : points.back());
		QPointF const old_vec(handle - origin);
		QPointF const new_vec(source_pos - origin);
		Vec4d A(rotationAndScale(old_vec, new_vec));
		int i = -1;
		BOOST_FOREACH(QPointF& pt, points) {
			++i;
			MatrixCalc<double> mc;
			Vec2d v(pt - origin);
			(mc(2, 2, A) * mc(2, 1, v)).write(v);
			pt = v + origin;
			spline.moveControlPoint(i, pt);
		}
	}
	
	update();
}

void
DewarpingView::dragFinished()
{

}

} // namespace output
