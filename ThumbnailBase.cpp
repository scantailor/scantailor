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
#include "NonCopyable.h"
#include "AbstractCommand.h"
#include "PixmapRenderer.h"
#include "imageproc/PolygonUtils.h"
#include <boost/weak_ptr.hpp>
#include <QPixmap>
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
#include <QDebug>

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
	ThumbnailPixmapCache& thumbnail_cache, QSizeF const& max_size,
	ImageId const& image_id, ImageTransformation const& image_xform)
:	m_rThumbnailCache(thumbnail_cache),
	m_maxSize(max_size),
	m_imageId(image_id),
	m_imageXform(image_xform)
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
			m_rThumbnailCache.loadRequest(m_imageId, pixmap, handler);
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
		
		// Cover parts of the image that should not be visible with background.
		// Note that because of Qt::WA_OpaquePaintEvent attribute, we need
		// to paint the whole widget, which we do here.
		
		QPolygonF const image_area(
			PolygonUtils::round(
				m_postScaleXform.map(
					m_imageXform.transform().map(
						m_imageXform.origRect()
					)
				)
			)
		);
		QPolygonF const crop_area(
			PolygonUtils::round(
				m_postScaleXform.map(m_imageXform.resultingCropArea())
			)
		);
		
		QPolygonF const intersected_area(
			PolygonUtils::round(image_area.intersected(crop_area))
		);
		
		QPainterPath intersected_path;
		intersected_path.addPolygon(intersected_area);
		
		QPainterPath containing_path;
		containing_path.addRect(m_boundingRect);
		
		QBrush brush;
		
		QPalette const palette(QApplication::palette());
		if (option->state & QStyle::State_Selected) {
			brush = palette.color(QPalette::Highlight);
		} else {
			brush = palette.color(QPalette::Window);
		}
		
		QPen pen(brush, 1.0);
		pen.setCosmetic(true);
		
		// By using a pen with the same color as the brush, we essentially
		// expanding the area we are going to draw.  It's necessary because
		// XRender doesn't provide subpixel accuracy.
		
		painter->setPen(pen);
		painter->setBrush(brush);
		painter->drawPath(containing_path.subtracted(intersected_path));
		
		painter->restore();
	}
	
	paintOverImage(*painter, image_to_display, thumb_to_display);
}

void
ThumbnailBase::setImageXform(ImageTransformation const& image_xform)
{
	m_imageXform = image_xform;
	QSizeF const unscaled_size(image_xform.resultingRect().size());
	QSizeF scaled_size(unscaled_size);
	scaled_size.scale(m_maxSize, Qt::KeepAspectRatio);
	
	m_boundingRect = QRectF(QPointF(0.0, 0.0), scaled_size);
	
	double const x_post_scale = m_boundingRect.width() / unscaled_size.width();
	double const y_post_scale = m_boundingRect.height() / unscaled_size.height();
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
