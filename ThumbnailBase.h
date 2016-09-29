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

#ifndef THUMBNAILBASE_H_
#define THUMBNAILBASE_H_

#include "NonCopyable.h"
#include "ImageId.h"
#include "ImageTransformation.h"
#include "IntrusivePtr.h"
#include "ThumbnailPixmapCache.h"
#ifndef Q_MOC_RUN
#include <boost/shared_ptr.hpp>
#endif
#include <QTransform>
#include <QGraphicsItem>
#include <QSizeF>
#include <QRectF>

class ThumbnailLoadResult;

class ThumbnailBase : public QGraphicsItem
{
	DECLARE_NON_COPYABLE(ThumbnailBase)
public:
	ThumbnailBase(
		IntrusivePtr<ThumbnailPixmapCache> const& thumbnail_cache,
		QSizeF const& max_size, ImageId const& image_id,
		ImageTransformation const& image_xform);
	
	virtual ~ThumbnailBase();
	
	virtual QRectF boundingRect() const;
	
	virtual void paint(QPainter* painter,
		QStyleOptionGraphicsItem const* option, QWidget *widget);
protected:
	/**
	 * \brief A hook to allow subclasses to draw over the thumbnail.
	 *
	 * \param painter The painter to be used for drawing.
	 * \param image_to_display Can be supplied to \p painter as a world
	 *        transformation in order to draw in virtual image coordinates,
	 *        that is in coordinates we get after applying the
	 *        ImageTransformation to the physical image coordinates.
	 *        We are talking about full-sized images here.
	 * \param thumb_to_display Can be supplied to \p painter as a world
	 *        transformation in order to draw in thumbnail coordinates.
	 *        Valid thumbnail coordinates lie within this->boundingRect().
	 *
	 * The painter is configured for drawing in thumbnail coordinates by
	 * default.  No clipping is configured, but drawing should be
	 * restricted to this->boundingRect().  Note that it's not necessary
	 * for subclasses to restore the painter state.
	 */
	virtual void paintOverImage(
		QPainter& painter, QTransform const& image_to_display,
		QTransform const& thumb_to_display) {}
	
	/**
	 * By default, the image is clipped by both the crop area (as defined
	 * by imageXform().resultingPostCropArea()), and the physical boundaries of
	 * the image itself.  Basically a point won't be clipped only if it's both
	 * inside of the crop area and inside the image.
	 * Extended clipping area only includes the cropping area, so it's possible
	 * to draw outside of the image but inside the crop area.
	 */
	void setExtendedClipArea(bool enabled) { m_extendedClipArea = enabled; }
	
	void setImageXform(ImageTransformation const& image_xform);
	
	ImageTransformation const& imageXform() const { return m_imageXform; }
	
	/**
	 * \brief Converts from the virtual image coordinates to thumbnail image coordinates.
	 *
	 * Virtual image coordinates is what you get after ImageTransformation.
	 */
	QTransform const& virtToThumb() const { return m_postScaleXform; }
private:
	class LoadCompletionHandler;
	
	void handleLoadResult(ThumbnailLoadResult const& result);
	
	IntrusivePtr<ThumbnailPixmapCache> m_ptrThumbnailCache;
	QSizeF m_maxSize;
	ImageId m_imageId;
	ImageTransformation m_imageXform;
	QRectF m_boundingRect;
	
	/**
	 * Transforms virtual image coordinates into thumbnail coordinates.
	 * Valid thumbnail coordinates lie within this->boundingRect().
	 */
	QTransform m_postScaleXform;
	
	boost::shared_ptr<LoadCompletionHandler> m_ptrCompletionHandler;
	bool m_extendedClipArea;
};

#endif
