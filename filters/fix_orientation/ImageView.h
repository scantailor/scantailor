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

#ifndef FIX_ORIENTATION_IMAGEVIEW_H_
#define FIX_ORIENTATION_IMAGEVIEW_H_

#include "ImageViewBase.h"
#include "OrthogonalRotation.h"

class ImageTransformation;

namespace fix_orientation
{

class ImageView : public ImageViewBase
{
	Q_OBJECT
public:
	ImageView(QImage const& image, ImageTransformation const& xform);
	
	virtual ~ImageView();
public slots:
	void setPreRotation(OrthogonalRotation rotation);
protected:
	virtual void wheelEvent(QWheelEvent* event);
	
	virtual void mousePressEvent(QMouseEvent* event);
	
	virtual void mouseReleaseEvent(QMouseEvent* event);
	
	virtual void mouseMoveEvent(QMouseEvent* event);
};

} // namespace fix_orientation

#endif
