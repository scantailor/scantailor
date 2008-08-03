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

#include "ImageTransformation.h"
#include <algorithm>

ImageTransformation::ImageTransformation(
	QRectF const& orig_image_rect, Dpi const& orig_dpi)
:	m_postRotation(0.0),
	m_origRect(orig_image_rect),
	m_croppedRect(orig_image_rect),
	m_resultingRect(orig_image_rect),
	m_origDpi(orig_dpi)
{
	preScaleToEqualizeDpi();
}

ImageTransformation::~ImageTransformation()
{
}

void
ImageTransformation::preScaleToDpi(Dpi const& dpi)
{
	if (m_origDpi.isNull() || dpi.isNull()) {
		return;
	}
	
	m_preScaledDpi = dpi;
	
	double const xscale = (double)dpi.horizontal() / m_origDpi.horizontal();
	double const yscale = (double)dpi.vertical() / m_origDpi.vertical();
	
	QSizeF const new_pre_scaled_image_size(
		m_origRect.width() * xscale, m_origRect.height() * yscale
	);
	
	// Reverse pre-rotation.
	QPolygonF tmp_crop_area(m_preRotateXform.inverted().map(m_cropArea));
	
	// Reverse pre-scaling.
	tmp_crop_area = m_preScaleXform.inverted().map(tmp_crop_area);
	
	// Update m_preScaleXform and m_preRotateXform.
	m_preScaleXform.reset();
	m_preScaleXform.scale(xscale, yscale);
	m_preRotateXform = m_preRotation.transform(new_pre_scaled_image_size);
	
	// Apply new pre-scaling.
	tmp_crop_area = m_preScaleXform.map(tmp_crop_area);
	
	// Re-apply pre-rotation.
	m_cropArea = m_preRotateXform.map(tmp_crop_area);
	
	m_cropXform = calcCropXform(m_cropArea);
	m_postRotateXform = calcPostRotateXform(m_postRotation);
	
	update();
}

void
ImageTransformation::preScaleToEqualizeDpi()
{
	int const min_dpi = std::min(m_origDpi.horizontal(), m_origDpi.vertical());
	preScaleToDpi(Dpi(min_dpi, min_dpi));
}

void
ImageTransformation::setPreRotation(OrthogonalRotation const rotation)
{
	m_preRotation = rotation;
	m_preRotateXform = m_preRotation.transform(m_origRect.size());
	resetCropArea();
	resetPostRotation();
	update();
}

QRectF
ImageTransformation::rectBeforeCropping() const
{
	return (m_preScaleXform * m_preRotateXform).mapRect(m_origRect);
}

void
ImageTransformation::setCropArea(QPolygonF const& area)
{
	m_cropArea = area;
	m_cropXform = calcCropXform(area);
	resetPostRotation();
	update();
}

QTransform
ImageTransformation::calcCropXform(QPolygonF const& area)
{
	QRectF const bounds(area.boundingRect());
	QTransform xform;
	xform.translate(-bounds.x(), -bounds.y());
	return xform;
}

void
ImageTransformation::setPostRotation(double const degrees)
{
	m_postRotateXform = calcPostRotateXform(degrees);
	m_postRotation = degrees;
	update();
}

QTransform
ImageTransformation::calcPostRotateXform(double const degrees)
{
	QTransform xform;
	if (degrees != 0.0) {
		QPointF const origin(m_cropArea.boundingRect().center());
		xform.translate(-origin.x(), -origin.y());
		xform *= QTransform().rotate(degrees);
		xform *= QTransform().translate(origin.x(), origin.y());
		
		// Calculate size changes.
		QPolygonF const pre_rotate_poly(m_cropXform.map(m_cropArea));
		QRectF const pre_rotate_rect(pre_rotate_poly.boundingRect());
		QPolygonF const post_rotate_poly(xform.map(pre_rotate_poly));
		QRectF const post_rotate_rect(post_rotate_poly.boundingRect());
		
		xform *= QTransform().translate(
			pre_rotate_rect.left() - post_rotate_rect.left(),
			pre_rotate_rect.top() - post_rotate_rect.top()
		);
	}
	return xform;
}

void
ImageTransformation::resetCropArea()
{
	m_cropArea.clear();
	m_cropXform.reset();
}

void
ImageTransformation::resetPostRotation()
{
	m_postRotation = 0.0;
	m_postRotateXform.reset();
}

void
ImageTransformation::update()
{
	QTransform const pre_scale_then_pre_rotate(m_preScaleXform * m_preRotateXform);
	QTransform const crop_then_post_rotate(m_cropXform * m_postRotateXform);
	m_transform = pre_scale_then_pre_rotate * crop_then_post_rotate;
	m_invTransform = m_transform.inverted();
	if (m_cropArea.empty()) {
		m_cropArea = pre_scale_then_pre_rotate.map(m_origRect);
	}
	m_resultingCropArea = crop_then_post_rotate.map(m_cropArea);
	m_resultingRect = m_resultingCropArea.boundingRect();
	m_croppedRect = m_cropXform.map(m_cropArea).boundingRect();
}
