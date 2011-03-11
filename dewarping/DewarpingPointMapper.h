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

#ifndef DEWARPING_DEWARPING_POINT_MAPPER_H_
#define DEWARPING_DEWARPING_POINT_MAPPER_H_

#include "CylindricalSurfaceDewarper.h"

class QRect;
class QTransform;

namespace dewarping
{

class DistortionModel;

class DewarpingPointMapper
{
public:
	DewarpingPointMapper(
		dewarping::DistortionModel const& distortion_model, double depth_perception,
		QTransform const& distortion_model_to_output,
		QRect const& output_content_rect);

	/**
	 * Similar to CylindricalSurfaceDewarper::mapToDewarpedSpace(),
	 * except it maps to dewarped image coordinates rather than
	 * to normalized dewarped coordinates.
	 */
	QPointF mapToDewarpedSpace(QPointF const& warped_pt) const;

	/**
	 * Similar to CylindricalSurfaceDewarper::mapToWarpedSpace(),
	 * except it maps from dewarped image coordinates rather than
	 * from normalized dewarped coordinates.
	 */
	QPointF mapToWarpedSpace(QPointF const& dewarped_pt) const;
private:
	CylindricalSurfaceDewarper m_dewarper;
	double m_modelDomainLeft;
	double m_modelDomainTop;
	double m_modelXScaleFromNormalized;
	double m_modelYScaleFromNormalized;
	double m_modelXScaleToNormalized;
	double m_modelYScaleToNormalized;
};

} // namespace dewarping

#endif
