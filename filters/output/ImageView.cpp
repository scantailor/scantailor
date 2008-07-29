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

#include "ImageView.h.moc"
#include "ImageTransformation.h"
#include <QDebug>
#include <assert.h>

namespace output
{

ImageView::ImageView(QImage const& image)
:	ImageViewBase(image),
	m_lightIdx(0),
	m_darkIdx(0),
	m_lightColor(Qt::white),
	m_darkColor(Qt::black)
{
	if (image.format() == QImage::Format_Mono
			|| image.format() == QImage::Format_MonoLSB) {
		m_bitonalImage = image;
		if (qGray(image.color(0)) < qGray(image.color(1))) {
			m_lightIdx = 1;
		} else {
			m_darkIdx = 1;
		}
	}
}

ImageView::~ImageView()
{
}

void
ImageView::tonesChanged(QColor const& light, QColor const& dark)
{
	assert(!m_bitonalImage.isNull());
	
	if (light == m_lightColor && dark == m_darkColor) {
		return;
	}
	
	m_lightColor = light;
	m_darkColor = dark;
	m_bitonalImage.setColor(m_lightIdx, light.rgb());
	m_bitonalImage.setColor(m_darkIdx, dark.rgb());
	updateImage(m_bitonalImage);
}

void
ImageView::wheelEvent(QWheelEvent* const event)
{
	handleZooming(event);
}

void
ImageView::mousePressEvent(QMouseEvent* const event)
{
	handleImageDragging(event);
}

void
ImageView::mouseReleaseEvent(QMouseEvent* const event)
{
	handleImageDragging(event);
}

void
ImageView::mouseMoveEvent(QMouseEvent* const event)
{
	handleImageDragging(event);
}

} // namespace output
