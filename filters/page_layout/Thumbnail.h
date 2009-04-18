/*
    Scan Tailor - Interactive post-processing tool for scanned pages.
    Copyright (C) 2007-2008  Joseph Artsimovich <joseph_a@mail.ru>

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

#ifndef PAGE_LAYOUT_THUMBNAIL_H_
#define PAGE_LAYOUT_THUMBNAIL_H_

#include "ThumbnailBase.h"
#include "Params.h"
#include "ImageTransformation.h"
#include "PhysicalTransformation.h"
#include <QTransform>
#include <QSizeF>
#include <QRectF>

class ThumbnailPixmapCache;
class ImageId;

namespace page_layout
{

class Thumbnail : public ThumbnailBase
{
public:
	Thumbnail(ThumbnailPixmapCache& thumbnail_cache, QSizeF const& max_size,
		ImageId const& image_id, ImageTransformation const& xform,
		Params const& params, QRectF const& adapted_content_rect,
		QSizeF const& aggregate_hard_size_mm);
	
	virtual void paintOverImage(
		QPainter& painter,
		QTransform const& image_to_display,
		QTransform const& thumb_to_display);
private:
	void recalcBoxesAndPresentationTransform();
	
	Params const m_params;
	QRectF const m_adaptedContentRect; /**< In m_origXform coortinates. */
	QSizeF const m_aggregateHardSizeMM;
	
	/**
	 * \brief Image transformation, as provided by the previous filter.
	 *
	 * We pass another transformation to ThumbnailBase, which we call
	 * "presentation transformation" in order to be able to display margins
	 * that may be outside the image area.  The presentation transformation
	 * is accessible via imageXform().
	 */
	ImageTransformation const m_origXform;
	
	/**
	 * Transformation between the original image coordinates and millimeters,
	 * assuming that point (0, 0) in pixel coordinates corresponds to point
	 * (0, 0) in millimeter coordinates.
	 */
	PhysicalTransformation const m_physXform;
	
	/**
	 * Transformation from m_origXform coordinates to millimeter coordinates.
	 */
	QTransform const m_origToMM;
	
	/**
	 * Transformation from millimeter coordinates to m_origXform coordinates.
	 */
	QTransform const m_mmToOrig;
	
	/**
	 * The outer rectangle (bounded by soft margins) in m_origXform coordinates.
	 */
	QRectF m_outerRect;
};

} // namespace page_layout

#endif
