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

#include "ThumbnailBase.h"
#include "ThumbnailPixmapCache.h"
#include "ThumbnailLoadResult.h"
#include "PixmapRenderer.h"
#include "boost_signals.h"
#include <boost/bind.hpp>
#include <QPixmap>
#include <QPainter>
#include <QColor>
#include <QPen>
#include <QBrush>
#include <QPainterPath>
#include <QSizeF>
#include <QPointF>
#include <QDebug>

ThumbnailBase::ThumbnailBase(
	ThumbnailPixmapCache& thumbnail_cache, ImageId const& image_id,
	ImageTransformation const& image_xform)
:	m_rThumbnailCache(thumbnail_cache),
	m_imageId(image_id),
	m_imageXform(image_xform),
	m_pixmapLoadPending(false)
{
	QSizeF const unscaled_size(m_imageXform.resultingRect().size());
	QSizeF scaled_size(unscaled_size);
	scaled_size.scale(QSize(250, 160), Qt::KeepAspectRatio);
	// FIXME: don't hardcode sizes.
	
	// Note: the reason to round the size is to ensure every thumbnail
	// is placed at integer coordinates.  This helps us do draw
	// non-antialiased lines over the thumbnail.
	m_boundingRect = QRectF(QPointF(0.0, 0.0), scaled_size.toSize());
	
	double const x_post_scale = m_boundingRect.width() / unscaled_size.width();
	double const y_post_scale = m_boundingRect.height() / unscaled_size.height();
	m_postScaleXform.scale(x_post_scale, y_post_scale);
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
	
	if (!m_pixmapLoadPending) {
		ThumbnailPixmapCache::Status const status =
		m_rThumbnailCache.loadRequest(
			m_imageId, pixmap,
			boost::bind(&ThumbnailBase::handleLoadResult, this, _1)
		);
		if (status == ThumbnailPixmapCache::QUEUED) {
			m_pixmapLoadPending = true;
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
	} else {
		QSizeF const orig_image_size(m_imageXform.origRect().size());
		double const x_pre_scale = orig_image_size.width() / pixmap.width();
		double const y_pre_scale = orig_image_size.height() / pixmap.height();
		QTransform pre_scale_xform;
		pre_scale_xform.scale(x_pre_scale, y_pre_scale);
		
		QTransform const pixmap_to_thumb(
			pre_scale_xform * m_imageXform.transform() * m_postScaleXform
		);
		
		painter->save();
		
		painter->setWorldTransform(pixmap_to_thumb * thumb_to_display);
		painter->setRenderHint(QPainter::SmoothPixmapTransform);
		painter->setRenderHint(QPainter::Antialiasing);
		
		QPainterPath clip_region;
		clip_region.addPolygon(pixmap_to_thumb.inverted().map(m_boundingRect));
		painter->setClipPath(clip_region);
		
		PixmapRenderer::drawPixmap(*painter, pixmap);
		
		painter->setWorldTransform(thumb_to_display);
		
		QPainterPath rect_path;
		// Adjusting bounding rect fixes a clipping bug in Qt,
		// which is still present in 4.4.0
		rect_path.addRect(m_boundingRect.adjusted(-1, -1, 1, 1));
		QPainterPath outline_path;
		outline_path.addPolygon(m_postScaleXform.map(m_imageXform.resultingCropArea()));
		
		painter->fillPath(rect_path.subtracted(outline_path), painter->background());
		
		painter->restore();
	}
	
	paintOverImage(*painter, image_to_display, thumb_to_display);
}

void
ThumbnailBase::handleLoadResult(ThumbnailLoadResult const& result)
{
	m_pixmapLoadPending = false;
	
	if (result.status() != ThumbnailLoadResult::LOAD_FAILED) {
		// Note that we don't store result.pixmap() in
		// this object, because we may have already went
		// out of view, so we may never receive a paint event.
		update();
	}
}
