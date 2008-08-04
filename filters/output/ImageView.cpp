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

namespace output
{

ImageView::ImageView(QImage const& image)
:	ImageViewBase(image)
{
}

ImageView::~ImageView()
{
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
