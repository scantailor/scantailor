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

#include "FilterData.h"
#include "Dpm.h"
#include "Dpi.h"
#include "imageproc/Grayscale.h"

using namespace imageproc;

FilterData::FilterData(QImage const& image)
:	m_origImage(image),
	m_grayImage(toGrayscale(m_origImage)),
	m_xform(image.rect(), Dpm(image)),
	m_bwThreshold(BinaryThreshold::otsuThreshold(m_grayImage))
{
}

FilterData::FilterData(FilterData const& other, ImageTransformation const& xform)
:	m_origImage(other.m_origImage),
	m_grayImage(other.m_grayImage),
	m_xform(xform),
	m_bwThreshold(other.m_bwThreshold)
{
}
