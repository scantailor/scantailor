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

#ifndef OUTPUT_IMAGEVIEW_H_
#define OUTPUT_IMAGEVIEW_H_

#include "ImageViewBase.h"
#include "DragHandler.h"
#include "ZoomHandler.h"
#include <QColor>

class ImageTransformation;

namespace output
{

class ImageView : public ImageViewBase
{
	Q_OBJECT
public:
	ImageView(QImage const& image, QImage const& downscaled_image);
	
	virtual ~ImageView();
private:
	DragHandler m_dragHandler;
	ZoomHandler m_zoomHandler;
};

} // namespace output

#endif
