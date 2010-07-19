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

#ifndef IMAGEPROC_CYLINDRICAL_SURFACE_DEWARPER_H_
#define IMAGEPROC_CYLINDRICAL_SURFACE_DEWARPER_H_

#include "HomographicTransform.h"
#include "PolylineIntersector.h"
#include "ReverseArcLengthMapper.h"
#include <vector>
#include <utility>
#include <QPointF>
#include <QLineF>

namespace imageproc
{

class CylindricalSurfaceDewarper
{
public:
	class State
	{
		friend class CylindricalSurfaceDewarper;
	private:
		PolylineIntersector::Hint m_intersectionHint1;
		PolylineIntersector::Hint m_intersectionHint2;
		ReverseArcLengthMapper::Hint m_arcLengthHint;
	};

	struct Generatrix
	{
		QLineF imgLine;
		HomographicTransform<1, double> pln2img;

		Generatrix(QLineF const& img_line, HomographicTransform<1, double> const& H)
			: imgLine(img_line), pln2img(H) {}
	};

	/**
	 * \param cam_dist_rel The physical distance from the camera to the plane
	 *        formed by the endpoints of our two directrixes, divided by the
	 *        physical distance between those two directrixes.  For scans,
	 *        use a fake value like 3.5.  Keep in mind though, that when using
	 *        fake values, they have to be reduced if one or both of directrixes
	 *        are in the middle of content.
	 */
	CylindricalSurfaceDewarper(
		std::vector<QPointF> const& img_directrix1,
		std::vector<QPointF> const& img_directrix2,
		double cam_dist_rel);

	Generatrix mapGeneratrix(double x, State& state);
private:
	class CoupledPolylinesIterator;
	
	static HomographicTransform<2, double> calcPlnToImgHomography(
		std::vector<QPointF> const& img_directrix1,
		std::vector<QPointF> const& img_directrix2);

	static double calcPlnStraightLineY(
		std::vector<QPointF> const& img_directrix1,
		std::vector<QPointF> const& img_directrix2,
		HomographicTransform<2, double> pln2img,
		HomographicTransform<2, double> img2pln);

	static HomographicTransform<2, double> fourPoint2DHomography(
		std::vector<std::pair<QPointF, QPointF> > const& points);

	static HomographicTransform<1, double> threePoint1DHomography(
		std::vector<std::pair<double, double> > const& pairs);

	void initReverseArcLengthMapper(
		std::vector<QPointF> const& img_directrix1,
		std::vector<QPointF> const& img_directrix2);

	HomographicTransform<2, double> m_pln2img;
	HomographicTransform<2, double> m_img2pln;
	double m_camDistRel;
	double m_plnStraightLineY;
	ReverseArcLengthMapper m_reverseArcLengthMapper;
	PolylineIntersector m_imgDirectrix1Intersector;
	PolylineIntersector m_imgDirectrix2Intersector;
};

} // namespace imageproc

#endif
