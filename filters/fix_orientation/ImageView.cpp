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

#include "ImageView.h"
#include "ImageView.h.moc"
#include "ImageTransformation.h"
#include "ImagePresentation.h"

namespace fix_orientation
{

ImageView::ImageView(
	QImage const& image, QImage const& downscaled_image,
	ImageTransformation const& xform)
:	ImageViewBase(
		image, downscaled_image,
		ImagePresentation(xform.transform(), xform.resultingPreCropArea())
	),
	m_dragHandler(*this),
	m_zoomHandler(*this),
	m_xform(xform)
{
	rootInteractionHandler().makeLastFollower(m_dragHandler);
	rootInteractionHandler().makeLastFollower(m_zoomHandler);
}

ImageView::~ImageView()
{
}

void
ImageView::setPreRotation(OrthogonalRotation const rotation)
{
	if (m_xform.preRotation() == rotation) {
		return;
	}
	
	m_xform.setPreRotation(rotation);

	// This should call update() by itself.
	updateTransform(ImagePresentation(m_xform.transform(), m_xform.resultingPreCropArea()));
}

} // namespace fix_orientation
