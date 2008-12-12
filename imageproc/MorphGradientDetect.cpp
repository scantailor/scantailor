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

#include "MorphGradientDetect.h"
#include "Morphology.h"
#include "Grayscale.h"
#include "GrayRasterOp.h"
#include <QImage>
#include <QSize>

namespace imageproc
{

QImage morphGradientDetectDarkSide(QImage const& image, QSize const& area)
{
	QImage const gray(toGrayscale(image));
	QImage lighter(erodeGray(gray, area, 0x00));
	grayRasterOp<GRopUnclippedSubtract<GRopDst, GRopSrc> >(lighter, gray);
	return lighter;
}

QImage morphGradientDetectLightSide(QImage const& image, QSize const& area)
{
	QImage const gray(toGrayscale(image));
	QImage darker(dilateGray(gray, area, 0xff));
	grayRasterOp<GRopUnclippedSubtract<GRopSrc, GRopDst> >(darker, gray);
	return darker;
}

} // namespace imageproc

