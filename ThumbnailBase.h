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

#ifndef THUMBNAILBASE_H_
#define THUMBNAILBASE_H_

#include "ImageId.h"
#include "ImageTransformation.h"
#include <boost/shared_ptr.hpp>
#include <QTransform>
#include <QGraphicsItem>
#include <QSizeF>
#include <QRectF>

class ThumbnailPixmapCache;
class ThumbnailLoadResult;

class ThumbnailBase : public QGraphicsItem
{
public:
	ThumbnailBase(
		ThumbnailPixmapCache& thumbnail_cache, QSizeF const& max_size,
		ImageId const& image_id, ImageTransformation const& image_xform);
	
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
	
	void setImageXform(ImageTransformation const& image_xform);
	
	ImageTransformation const& imageXform() const { return m_imageXform; }
	
	QTransform const& imageToThumb() const { return m_postScaleXform; }
private:
	class LoadCompletionHandler;
	
	void handleLoadResult(ThumbnailLoadResult const& result);
	
	ThumbnailPixmapCache& m_rThumbnailCache;
	QSizeF m_maxSize;
	ImageId m_imageId;
	ImageTransformation m_imageXform;
	QRectF m_boundingRect;
	QTransform m_postScaleXform;
	boost::shared_ptr<LoadCompletionHandler> m_ptrCompletionHandler;
};

#endif
