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

#include "ThumbnailBase.h"
#include "ThumbnailPixmapCache.h"
#include "ThumbnailLoadResult.h"
#include "NonCopyable.h"
#include "AbstractCommand.h"
#include "PixmapRenderer.h"
#include "imageproc/PolygonUtils.h"
#ifndef Q_MOC_RUN
#include <boost/weak_ptr.hpp>
#endif
#include <QPixmap>
#include <QPixmapCache>
#include <QPainter>
#include <QColor>
#include <QPen>
#include <QBrush>
#include <QPainterPath>
#include <QStyleOptionGraphicsItem>
#include <QPalette>
#include <QStyle>
#include <QApplication>
#include <QPointF>
#include <Qt>
#include <QDebug>
#include <math.h>

using namespace imageproc;

class ThumbnailBase::LoadCompletionHandler :
	public AbstractCommand1<void, ThumbnailLoadResult const&>
{
	DECLARE_NON_COPYABLE(LoadCompletionHandler)
public:
	LoadCompletionHandler(ThumbnailBase* thumb) : m_pThumb(thumb) {}
	
	virtual void operator()(ThumbnailLoadResult const& result) {
		m_pThumb->handleLoadResult(result);
	}
private:
	ThumbnailBase* m_pThumb;
};


ThumbnailBase::ThumbnailBase(
	IntrusivePtr<ThumbnailPixmapCache> const& thumbnail_cache,
	QSizeF const& max_size, ImageId const& image_id,
	ImageTransformation const& image_xform)
:	m_ptrThumbnailCache(thumbnail_cache),
	m_maxSize(max_size),
	m_imageId(image_id),
	m_imageXform(image_xform),
	m_extendedClipArea(false)
{
	setImageXform(m_imageXform);
}

ThumbnailBase::~ThumbnailBase()
{
}

QRectF
ThumbnailBase::boundingRect() const
{
	return m_boundingRect;
}

void
ThumbnailBase::paint(QPainter* painter,
	QStyleOptionGraphicsItem const* option, QWidget *widget)
{
	QPixmap pixmap;
	
	if (!m_ptrCompletionHandler.get()) {
		boost::shared_ptr<LoadCompletionHandler> handler(
			new LoadCompletionHandler(this)
		);
		ThumbnailPixmapCache::Status const status =
			m_ptrThumbnailCache->loadRequest(m_imageId, pixmap, handler);
		if (status == ThumbnailPixmapCache::QUEUED) {
			m_ptrCompletionHandler.swap(handler);
		}
	}
	
	QTransform const image_to_display(m_postScaleXform * painter->worldTransform());
	QTransform const thumb_to_display(painter->worldTransform());
	
	if (pixmap.isNull()) {
		double const border = 1.0;
		double const shadow = 2.0;
		QRectF rect(m_boundingRect);
		rect.adjust(border, border, -(border + shadow), -(border + shadow));
		
		painter->fillRect(m_boundingRect, QColor(0x00, 0x00, 0x00));
		painter->fillRect(rect, QColor(0xff, 0xff, 0xff));
		
		paintOverImage(*painter, image_to_display, thumb_to_display);
		return;
	}
	
	
	QSizeF const orig_image_size(m_imageXform.origRect().size());
	double const x_pre_scale = orig_image_size.width() / pixmap.width();
	double const y_pre_scale = orig_image_size.height() / pixmap.height();
	QTransform pre_scale_xform;
	pre_scale_xform.scale(x_pre_scale, y_pre_scale);
	
	QTransform const pixmap_to_thumb(
		pre_scale_xform * m_imageXform.transform() * m_postScaleXform
	);
	
	// The polygon to draw into in original image coordinates.
	QPolygonF image_poly(PolygonUtils::round(m_imageXform.resultingPostCropArea()));
	if (!m_extendedClipArea) {
		image_poly = image_poly.intersected(
			PolygonUtils::round(
				m_imageXform.transform().map(m_imageXform.origRect())
			)
		);
	}
	
	// The polygon to draw into in display coordinates.
	QPolygonF display_poly(image_to_display.map(image_poly));
	
	QRectF display_rect(display_poly.boundingRect());
	display_rect.setTop(floor(display_rect.top()));
	display_rect.setLeft(floor(display_rect.left()));
	display_rect.setBottom(ceil(display_rect.bottom()));
	display_rect.setRight(ceil(display_rect.right()));
		
	QPixmap temp_pixmap;
	QString const cache_key(QString::fromAscii("ThumbnailBase::temp_pixmap"));
	if (!QPixmapCache::find(cache_key, temp_pixmap)
			|| temp_pixmap.width() < display_rect.width()
			|| temp_pixmap.height() < display_rect.width()) {
		int w = (int)display_rect.width();
		int h = (int)display_rect.height();
		
		// Add some extra, to avoid rectreating the pixmap too often.
		w += w / 10;
		h += h / 10;
		
		temp_pixmap = QPixmap(w, h);
		
		if (!temp_pixmap.hasAlphaChannel()) {
			// This actually forces the alpha channel to be created.
			temp_pixmap.fill(Qt::transparent);
		}
		
		QPixmapCache::insert(cache_key, temp_pixmap);
	}
	
	QPainter temp_painter;
	temp_painter.begin(&temp_pixmap);
	
	QTransform temp_adjustment;
	temp_adjustment.translate(-display_rect.left(), -display_rect.top());
	
	temp_painter.setWorldTransform(
		pixmap_to_thumb * thumb_to_display * temp_adjustment
	);
	
	// Turn off alpha compositing.
	temp_painter.setCompositionMode(QPainter::CompositionMode_Source);
	
	temp_painter.setRenderHint(QPainter::SmoothPixmapTransform);
	temp_painter.setRenderHint(QPainter::Antialiasing);
	
	PixmapRenderer::drawPixmap(temp_painter, pixmap);
	
	// Turn alpha compositing on again.
	temp_painter.setCompositionMode(QPainter::CompositionMode_SourceOver);
	
	// Setup the painter for drawing in thumbnail coordinates,
	// as required for paintOverImage().
	temp_painter.setWorldTransform(thumb_to_display * temp_adjustment);
	
	temp_painter.save();
	paintOverImage(
		temp_painter, image_to_display * temp_adjustment,
		thumb_to_display * temp_adjustment
	);
	temp_painter.restore();
	
	temp_painter.setPen(Qt::NoPen);
	temp_painter.setBrush(Qt::white);
	temp_painter.setWorldTransform(temp_adjustment);
#ifndef Q_WS_X11
	// That's how it's supposed to be.
	temp_painter.setCompositionMode(QPainter::CompositionMode_Clear);
#else
	// QPainter::CompositionMode_Clear doesn't work for arbitrarily shaped
	// objects on X11, as well as CompositionMode_Source with a transparent
	// brush.  Fortunately, CompositionMode_DestinationOut with a non-transparent
	// brush does actually work.
	temp_painter.setCompositionMode(QPainter::CompositionMode_DestinationOut);
#endif
	temp_painter.drawPolygon(
		QPolygonF(display_rect).subtracted(PolygonUtils::round(display_poly))
	);
	
	temp_painter.end();
	
	painter->setWorldTransform(QTransform());
	painter->setClipRect(display_rect);
	painter->setRenderHint(QPainter::SmoothPixmapTransform, false);
	painter->setCompositionMode(QPainter::CompositionMode_SourceOver);
	painter->drawPixmap(display_rect.topLeft(), temp_pixmap);
}

void
ThumbnailBase::setImageXform(ImageTransformation const& image_xform)
{
	m_imageXform = image_xform;
	QSizeF const unscaled_size(
		image_xform.resultingRect().size().expandedTo(QSizeF(1, 1))
	);
	QSizeF scaled_size(unscaled_size);
	scaled_size.scale(m_maxSize, Qt::KeepAspectRatio);
	
	m_boundingRect = QRectF(QPointF(0.0, 0.0), scaled_size);
	
	double const x_post_scale = scaled_size.width() / unscaled_size.width();
	double const y_post_scale = scaled_size.height() / unscaled_size.height();
	m_postScaleXform.reset();
	m_postScaleXform.scale(x_post_scale, y_post_scale);
}

void
ThumbnailBase::handleLoadResult(ThumbnailLoadResult const& result)
{
	m_ptrCompletionHandler.reset();
	
	if (result.status() != ThumbnailLoadResult::LOAD_FAILED) {
		// Note that we don't store result.pixmap() in
		// this object, because we may have already went
		// out of view, so we may never receive a paint event.
		update();
	}
}
