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

#include "CylindricalSurfaceDewarper.h"
#include "ToLineProjector.h"
#include "MatrixCalc.h"
#include "VecNT.h"
#include "NumericTraits.h"
#include <QLineF>
#include <QtGlobal>
#include <QDebug>
#ifndef Q_MOC_RUN
#include <boost/foreach.hpp>
#endif
#include <algorithm>
#include <math.h>
#include <assert.h>

/*
Naming conventions:
img: Coordinates in the warped image.
pln: Coordinates on a plane where the 4 corner points of the curved
     quadrilateral are supposed to lie.  In our model we assume that
	 all 4 lie on the same plane.  The corner points are mapped to
	 the following points on the plane:
	 * Start point of curve1 [top curve]: (0, 0)
	 * End point of curve1 [top curve]: (1, 0)
	 * Start point of curve2 [bottom curve]: (0, 1)
	 * End point of curve2 [bottom curve]: (1, 1)
	 pln and img coordinates are linked by a 2D homography,
	 namely m_pln2img and m_img2pln.
crv: Dewarped normalized coordinates.  crv X coordinates are linked
     to pln X ccoordinates through m_arcLengthMapper while the Y
	 coordinates are linked by a one dimensional homography that's
	 different for each generatrix.
*/

namespace dewarping
{

class CylindricalSurfaceDewarper::CoupledPolylinesIterator
{
public:
	CoupledPolylinesIterator(
		std::vector<QPointF> const& img_directrix1,
		std::vector<QPointF> const& img_directrix2,
		HomographicTransform<2, double> const& pln2img,
		HomographicTransform<2, double> const& img2pln);

	bool next(QPointF& img_pt1, QPointF& img_pt2, double& pln_x);
private:
	void next1(QPointF& img_pt1, QPointF& img_pt2, double& pln_x);

	void next2(QPointF& img_pt1, QPointF& img_pt2, double& pln_x);

	void advance1();

	void advance2();

	std::vector<QPointF>::const_iterator m_seq1It;
	std::vector<QPointF>::const_iterator m_seq2It;
	std::vector<QPointF>::const_iterator m_seq1End;
	std::vector<QPointF>::const_iterator m_seq2End;
	HomographicTransform<2, double> m_pln2img;
	HomographicTransform<2, double> m_img2pln;
	Vec2d m_prevImgPt1;
	Vec2d m_prevImgPt2;
	Vec2d m_nextImgPt1;
	Vec2d m_nextImgPt2;
	double m_nextPlnX1;
	double m_nextPlnX2;
};

CylindricalSurfaceDewarper::CylindricalSurfaceDewarper(
	std::vector<QPointF> const& img_directrix1,
	std::vector<QPointF> const& img_directrix2, double depth_perception)
:	m_pln2img(calcPlnToImgHomography(img_directrix1, img_directrix2)),
	m_img2pln(m_pln2img.inv()),
	m_depthPerception(depth_perception),
	m_plnStraightLineY(
		calcPlnStraightLineY(img_directrix1, img_directrix2, m_pln2img, m_img2pln)
	),
	m_directrixArcLength(1.0),
	m_imgDirectrix1Intersector(img_directrix1),
	m_imgDirectrix2Intersector(img_directrix2)
{
	initArcLengthMapper(img_directrix1, img_directrix2);
}

CylindricalSurfaceDewarper::Generatrix
CylindricalSurfaceDewarper::mapGeneratrix(double crv_x, State& state) const
{
	double const pln_x = m_arcLengthMapper.arcLenToX(crv_x, state.m_arcLengthHint);
	
	Vec2d const pln_top_pt(pln_x, 0);
	Vec2d const pln_bottom_pt(pln_x, 1);
	Vec2d const img_top_pt(m_pln2img(pln_top_pt));
	Vec2d const img_bottom_pt(m_pln2img(pln_bottom_pt));
	QLineF const img_generatrix(img_top_pt, img_bottom_pt);
	ToLineProjector const projector(img_generatrix);
	Vec2d const img_directrix1_pt(
		m_imgDirectrix1Intersector.intersect(img_generatrix, state.m_intersectionHint1)
	);
	Vec2d const img_directrix2_pt(
		m_imgDirectrix2Intersector.intersect(img_generatrix, state.m_intersectionHint2)
	);
	Vec2d const img_straight_line_pt(m_pln2img(Vec2d(pln_x, m_plnStraightLineY)));
	double const img_directrix1_proj(projector.projectionScalar(img_directrix1_pt));
	double const img_directrix2_proj(projector.projectionScalar(img_directrix2_pt));
	double const img_straight_line_proj(projector.projectionScalar(img_straight_line_pt));
	
	boost::array<std::pair<double, double>, 3> pairs;
	pairs[0] = std::make_pair(0.0, img_directrix1_proj);
	pairs[1] = std::make_pair(1.0, img_directrix2_proj);
	if (fabs(m_plnStraightLineY) < 0.05 || fabs(m_plnStraightLineY - 1.0) < 0.05) {
		pairs[2] = std::make_pair(0.5, 0.5 * (img_directrix1_proj + img_directrix2_proj));
	} else {
		pairs[2] = std::make_pair(m_plnStraightLineY, img_straight_line_proj);
	}
	HomographicTransform<1, double> H(threePoint1DHomography(pairs));
	
	return Generatrix(img_generatrix, H);
}

QPointF
CylindricalSurfaceDewarper::mapToDewarpedSpace(QPointF const& img_pt) const
{
	State state;

	double const pln_x = m_img2pln(img_pt)[0];
	double const crv_x = m_arcLengthMapper.xToArcLen(pln_x, state.m_arcLengthHint);

	Vec2d const pln_top_pt(pln_x, 0);
	Vec2d const pln_bottom_pt(pln_x, 1);
	Vec2d const img_top_pt(m_pln2img(pln_top_pt));
	Vec2d const img_bottom_pt(m_pln2img(pln_bottom_pt));
	QLineF const img_generatrix(img_top_pt, img_bottom_pt);
	ToLineProjector const projector(img_generatrix);
	Vec2d const img_directrix1_pt(
		m_imgDirectrix1Intersector.intersect(img_generatrix, state.m_intersectionHint1)
	);
	Vec2d const img_directrix2_pt(
		m_imgDirectrix2Intersector.intersect(img_generatrix, state.m_intersectionHint2)
	);
	Vec2d const img_straight_line_pt(m_pln2img(Vec2d(pln_x, m_plnStraightLineY)));
	double const img_directrix1_proj(projector.projectionScalar(img_directrix1_pt));
	double const img_directrix2_proj(projector.projectionScalar(img_directrix2_pt));
	double const img_straight_line_proj(projector.projectionScalar(img_straight_line_pt));

	boost::array<std::pair<double, double>, 3> pairs;
	pairs[0] = std::make_pair(img_directrix1_proj, 0.0);
	pairs[1] = std::make_pair(img_directrix2_proj, 1.0);
	if (fabs(m_plnStraightLineY) < 0.05 || fabs(m_plnStraightLineY - 1.0) < 0.05) {
		pairs[2] = std::make_pair(0.5 * (img_directrix1_proj + img_directrix2_proj), 0.5);
	} else {
		pairs[2] = std::make_pair(img_straight_line_proj, m_plnStraightLineY);
	}
	HomographicTransform<1, double> const H(threePoint1DHomography(pairs));

	double const img_pt_proj(projector.projectionScalar(img_pt));
	double const crv_y = H(img_pt_proj);

	return QPointF(crv_x, crv_y);
}

QPointF
CylindricalSurfaceDewarper::mapToWarpedSpace(QPointF const& crv_pt) const
{
	State state;
	Generatrix const gtx(mapGeneratrix(crv_pt.x(), state));
	return gtx.imgLine.pointAt(gtx.pln2img(crv_pt.y()));
}

HomographicTransform<2, double>
CylindricalSurfaceDewarper::calcPlnToImgHomography(
	std::vector<QPointF> const& img_directrix1,
	std::vector<QPointF> const& img_directrix2)
{
	boost::array<std::pair<QPointF, QPointF>, 4> pairs;
	pairs[0] = std::make_pair(QPointF(0, 0), img_directrix1.front());
	pairs[1] = std::make_pair(QPointF(1, 0), img_directrix1.back());
	pairs[2] = std::make_pair(QPointF(0, 1), img_directrix2.front());
	pairs[3] = std::make_pair(QPointF(1, 1), img_directrix2.back());
	
	return fourPoint2DHomography(pairs);
}

double
CylindricalSurfaceDewarper::calcPlnStraightLineY(
	std::vector<QPointF> const& img_directrix1,
	std::vector<QPointF> const& img_directrix2,
	HomographicTransform<2, double> const pln2img,
	HomographicTransform<2, double> const img2pln)
{	
	double pln_y_accum = 0;
	double weight_accum = 0;

	CoupledPolylinesIterator it(img_directrix1, img_directrix2, pln2img, img2pln);
	QPointF img_curve1_pt;
	QPointF img_curve2_pt;
	double pln_x;
	while (it.next(img_curve1_pt, img_curve2_pt, pln_x)) {
		QLineF const img_generatrix(img_curve1_pt, img_curve2_pt);
		Vec2d const img_line1_pt(pln2img(Vec2d(pln_x, 0)));
		Vec2d const img_line2_pt(pln2img(Vec2d(pln_x, 1)));
		ToLineProjector const projector(img_generatrix);
		double const p1 = 0;
		double const p2 = projector.projectionScalar(img_line1_pt);
		double const p3 = projector.projectionScalar(img_line2_pt);
		double const p4 = 1;
		double const dp1 = p2 - p1;
		double const dp2 = p4 - p3;
		double const weight = fabs(dp1 + dp2);
		if (weight < 0.01) {
			continue;
		}

		double const p0 = (p3 * dp1 + p2 * dp2) / (dp1 + dp2);
		Vec2d const img_pt(img_generatrix.pointAt(p0));
		pln_y_accum += img2pln(img_pt)[1] * weight;
		weight_accum += weight;
	}

	return weight_accum == 0 ? 0.5 : pln_y_accum / weight_accum;
}

HomographicTransform<2, double>
CylindricalSurfaceDewarper::fourPoint2DHomography(
	boost::array<std::pair<QPointF, QPointF>, 4> const& pairs)
{
	VecNT<64, double> A;
	VecNT<8, double> B;
	double* pa = A.data();
	double* pb = B.data();
	int i = 0;

	typedef std::pair<QPointF, QPointF> Pair;
	BOOST_FOREACH(Pair const& pair, pairs) {
		QPointF const from(pair.first);
		QPointF const to(pair.second);
		
		pa[8*0] = -from.x();
		pa[8*1] = -from.y();
		pa[8*2] = -1;
		pa[8*3] = 0;
		pa[8*4] = 0;
		pa[8*5] = 0;
		pa[8*6] = to.x()*from.x();
		pa[8*7] = to.x()*from.y();
		pb[0] = -to.x();
		++pa;
		++pb;
		
		pa[8*0] = 0;
		pa[8*1] = 0;
		pa[8*2] = 0;
		pa[8*3] = -from.x();
		pa[8*4] = -from.y();
		pa[8*5] = -1;
		pa[8*6] = to.y()*from.x();
		pa[8*7] = to.y()*from.y();
		pb[0] = -to.y();
		++pa;
		++pb;
	}

	VecNT<9, double> H;
	H[8] = 1.0;

	MatrixCalc<double> mc;
	mc(A, 8, 8).solve(mc(B, 8, 1)).write(H);
	mc(H, 3, 3).trans().write(H);

	return HomographicTransform<2, double>(H);
}

HomographicTransform<1, double>
CylindricalSurfaceDewarper::threePoint1DHomography(
	boost::array<std::pair<double, double>, 3> const& pairs)
{
	VecNT<9, double> A;
	VecNT<3, double> B;
	double* pa = A.data();
	double* pb = B.data();

	typedef std::pair<double, double> Pair;
	BOOST_FOREACH(Pair const& pair, pairs) {
		double const from = pair.first;
		double const to = pair.second;
		
		pa[3*0] = -from;
		pa[3*1] = -1;
		pa[3*2] = from * to;
		pb[0] = -to;
		++pa;
		++pb;
	}

	Vec4d H;
	H[3] = 1.0;

	MatrixCalc<double> mc;
	mc(A, 3, 3).solve(mc(B, 3, 1)).write(H);
	mc(H, 2, 2).trans().write(H);

	return HomographicTransform<1, double>(H);
}

void
CylindricalSurfaceDewarper::initArcLengthMapper(
	std::vector<QPointF> const& img_directrix1,
	std::vector<QPointF> const& img_directrix2)
{
	double prev_elevation = 0;
	CoupledPolylinesIterator it(img_directrix1, img_directrix2, m_pln2img, m_img2pln);
	QPointF img_curve1_pt;
	QPointF img_curve2_pt;
	double prev_pln_x = NumericTraits<double>::min();
	double pln_x;
	while (it.next(img_curve1_pt, img_curve2_pt, pln_x)) {
		if (pln_x <= prev_pln_x) {
			// This means our surface has an S-like shape.
			// We don't support that, and to make ReverseArcLength happy,
			// we have to skip such points.
			continue;
		}

		QLineF const img_generatrix(img_curve1_pt, img_curve2_pt);
		Vec2d const img_line1_pt(m_pln2img(Vec2d(pln_x, 0)));
		Vec2d const img_line2_pt(m_pln2img(Vec2d(pln_x, 1)));
		
		ToLineProjector const projector(img_generatrix);
		double const y1 = projector.projectionScalar(img_line1_pt);
		double const y2 = projector.projectionScalar(img_line2_pt);

		double elevation = m_depthPerception * (1.0 - (y2 - y1));
		elevation = qBound(-0.5, elevation, 0.5);

		m_arcLengthMapper.addSample(pln_x, elevation);
		prev_elevation = elevation;
		prev_pln_x = pln_x;
	}

	// Needs to go before normalizeRange().
	m_directrixArcLength = m_arcLengthMapper.totalArcLength();

	// Scale arc lengths to the range of [0, 1].
	m_arcLengthMapper.normalizeRange(1);
}


/*======================= CoupledPolylinesIterator =========================*/

CylindricalSurfaceDewarper::CoupledPolylinesIterator::CoupledPolylinesIterator(
	std::vector<QPointF> const& img_directrix1,
	std::vector<QPointF> const& img_directrix2,
	HomographicTransform<2, double> const& pln2img,
	HomographicTransform<2, double> const& img2pln)
:	m_seq1It(img_directrix1.begin()),
	m_seq2It(img_directrix2.begin()),
	m_seq1End(img_directrix1.end()),
	m_seq2End(img_directrix2.end()),
	m_pln2img(pln2img),
	m_img2pln(img2pln),
	m_prevImgPt1(*m_seq1It),
	m_prevImgPt2(*m_seq2It),
	m_nextImgPt1(m_prevImgPt1),
	m_nextImgPt2(m_prevImgPt2),
	m_nextPlnX1(0),
	m_nextPlnX2(0)
{
}

bool
CylindricalSurfaceDewarper::CoupledPolylinesIterator::next(QPointF& img_pt1, QPointF& img_pt2, double& pln_x)
{
	if (m_nextPlnX1 < m_nextPlnX2 && m_seq1It != m_seq1End) {
		next1(img_pt1, img_pt2, pln_x);
		return true;
	} else if (m_seq2It != m_seq2End) {
		next2(img_pt1, img_pt2, pln_x);
		return true;
	} else {
		return false;
	}
}

void
CylindricalSurfaceDewarper::CoupledPolylinesIterator::next1(QPointF& img_pt1, QPointF& img_pt2, double& pln_x)
{
	Vec2d const pln_pt1(m_img2pln(m_nextImgPt1));
	pln_x = pln_pt1[0];
	img_pt1 = m_nextImgPt1;
	
	Vec2d const pln_ptx(pln_pt1[0], pln_pt1[1] + 1);
	Vec2d const img_ptx(m_pln2img(pln_ptx));
	
	if (QLineF(img_pt1, img_ptx).intersect(QLineF(m_nextImgPt2, m_prevImgPt2), &img_pt2) == QLineF::NoIntersection) {
		img_pt2 = m_nextImgPt2;
	}

	advance1();
	if (m_seq2It != m_seq2End && Vec2d(m_nextImgPt2 - img_pt2).squaredNorm() < 1) {
		advance2();
	}
}

void
CylindricalSurfaceDewarper::CoupledPolylinesIterator::next2(QPointF& img_pt1, QPointF& img_pt2, double& pln_x)
{
	Vec2d const pln_pt2(m_img2pln(m_nextImgPt2));
	pln_x = pln_pt2[0];
	img_pt2 = m_nextImgPt2;
	
	Vec2d const pln_ptx(pln_pt2[0], pln_pt2[1] + 1);
	Vec2d const img_ptx(m_pln2img(pln_ptx));
	
	if (QLineF(img_pt2, img_ptx).intersect(QLineF(m_nextImgPt1, m_prevImgPt1), &img_pt1) == QLineF::NoIntersection) {
		img_pt1 = m_nextImgPt1;
	}

	advance2();
	if (m_seq1It != m_seq1End && Vec2d(m_nextImgPt1 - img_pt1).squaredNorm() < 1) {
		advance1();
	}
}

void
CylindricalSurfaceDewarper::CoupledPolylinesIterator::advance1()
{
	if (++m_seq1It == m_seq1End) {
		return;
	}

	m_prevImgPt1 = m_nextImgPt1;
	m_nextImgPt1 = *m_seq1It;
	m_nextPlnX1 = m_img2pln(m_nextImgPt1)[0];
}

void
CylindricalSurfaceDewarper::CoupledPolylinesIterator::advance2()
{
	if (++m_seq2It == m_seq2End) {
		return;
	}

	m_prevImgPt2 = m_nextImgPt2;
	m_nextImgPt2 = *m_seq2It;
	m_nextPlnX2 = m_img2pln(m_nextImgPt2)[0];
}

} // namespace dewarping
