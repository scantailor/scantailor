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

#ifndef SELECT_CONTENT_THUMBNAIL_H_
#define SELECT_CONTENT_THUMBNAIL_H_

#include "ThumbnailBase.h"
#include <QRectF>

class QSizeF;
class ThumbnailPixmapCache;
class ImageId;
class ImageTransformation;

namespace select_content
{

class Thumbnail : public ThumbnailBase
{
public:
	Thumbnail(IntrusivePtr<ThumbnailPixmapCache> const& thumbnail_cache,
		QSizeF const& max_size, ImageId const& image_id,
		ImageTransformation const& xform, QRectF const& content_rect);
	
	virtual void paintOverImage(
		QPainter& painter,
		QTransform const& image_to_display,
		QTransform const& thumb_to_display);
private:
	QRectF m_contentRect;
};

} // namespace select_content

#endif
