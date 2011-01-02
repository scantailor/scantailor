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

#include "PixmapRenderer.h"
#include "config.h"
#include <QPainter>
#include <QPixmap>
#include <QWidget>
#include <QTransform>
#include <QRect>
#include <QRectF>
#include <Qt>
#include <QDebug>
#include <math.h>

#ifdef ENABLE_OPENGL
#	include <QGLWidget>
#endif

#ifdef Q_WS_X11
#  include <QX11Info>
#  include <QRegion>
#  include <X11/extensions/Xrender.h>
#endif

void
PixmapRenderer::drawPixmap(
	QPainter& painter, QPixmap const& pixmap)
{
#if !defined(Q_WS_X11)
	drawPixmapNoXRender(painter, pixmap);
#else
	QPaintDevice* const dev = painter.device();
	
	QPoint offset; // Both x and y will be either zero or negative.
	QPaintDevice* const redir_dev = QPainter::redirected(painter.device(), &offset);
	QPaintDevice* const paint_dev = redir_dev ? redir_dev : dev;

#if defined(ENABLE_OPENGL)
	if (dynamic_cast<QGLWidget*>(paint_dev)) {
		drawPixmapNoXRender(painter, pixmap);
		return;
	}
#endif

	QRect const device_rect(
		QRect(0, 0, dev->width(), dev->height()).translated(-offset)
	);
	
	QRectF const src_rect(pixmap.rect());
	
	Display* const dpy = QX11Info::display();
	Picture const src_pict = pixmap.x11PictureHandle();
	Picture dst_pict = 0;
	if (QWidget* widget = dynamic_cast<QWidget*>(paint_dev)) {
		dst_pict = widget->x11PictureHandle();
	} else if (QPixmap* pixmap = dynamic_cast<QPixmap*>(paint_dev)) {
		dst_pict = pixmap->x11PictureHandle();
	}
	
	if (!dst_pict) {
		drawPixmapNoXRender(painter, pixmap);
		return;
	}
	
	// Note that device transform already accounts for offset
	// within a destination surface.
	QTransform const src_to_dst(painter.deviceTransform());
	QTransform const dst_to_src(src_to_dst.inverted());
	QPolygonF const dst_poly(src_to_dst.map(src_rect));
	
	XTransform xform = {{
		{
			XDoubleToFixed(dst_to_src.m11()),
			XDoubleToFixed(dst_to_src.m21()),
			XDoubleToFixed(dst_to_src.m31())
		},
		{
			XDoubleToFixed(dst_to_src.m12()),
			XDoubleToFixed(dst_to_src.m22()),
			XDoubleToFixed(dst_to_src.m32())
		},
		{
			XDoubleToFixed(dst_to_src.m13()),
			XDoubleToFixed(dst_to_src.m23()),
			XDoubleToFixed(dst_to_src.m33())
		}
	}};
	
	XRenderSetPictureTransform(dpy, src_pict, &xform);

	char const* filter = "fast";
	if (painter.testRenderHint(QPainter::SmoothPixmapTransform)) {
		filter = "good";
	}
	
	XRenderSetPictureFilter(dpy, src_pict, filter, 0, 0);
	
	QRectF const dst_rect_precise(dst_poly.boundingRect());
	QRect const dst_rect_fitting(
		QPoint(
			int(ceil(dst_rect_precise.left())),
			int(ceil(dst_rect_precise.top()))
		),
		QPoint(
			int(floor(dst_rect_precise.right())) - 1,
			int(floor(dst_rect_precise.bottom())) - 1
		)
	);
	QRect dst_bounding_rect(device_rect);
	if (painter.hasClipping()) {
		QRect const clip_rect(
			src_to_dst.map(painter.clipPath()).boundingRect().toRect()
		);
		dst_bounding_rect = dst_bounding_rect.intersected(clip_rect);
	}
	QRect const dst_rect(dst_rect_fitting.intersect(dst_bounding_rect));
	
	// Note that XRenderComposite() expects destination coordinates
	// everywhere, even for source picture origin.
	XRenderComposite(
		dpy, PictOpSrc,
		src_pict, 0, dst_pict, dst_rect.left(), dst_rect.top(), 0, 0,
		dst_rect.left(), dst_rect.top(), dst_rect.width(), dst_rect.height()
	);
#endif
}

void
PixmapRenderer::drawPixmapNoXRender(QPainter& painter, QPixmap const& pixmap)
{
	QTransform const inv_transform(painter.worldTransform().inverted());
	QRectF const src_rect(inv_transform.map(QRectF(painter.viewport())).boundingRect());
	QRectF const bounded_src_rect(src_rect.intersected(pixmap.rect()));
	painter.drawPixmap(bounded_src_rect, pixmap, bounded_src_rect);
}
