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

#include "PhysicalTransformation.h"
#include "Dpi.h"
#include "imageproc/Constants.h"

using namespace imageproc::constants;

PhysicalTransformation::PhysicalTransformation(Dpi const& dpi)
{
	double const xscale = dpi.horizontal() * (DPI2DPM / 1000.0);
	double const yscale = dpi.vertical() * (DPI2DPM / 1000.0);
	m_mmToPixels.scale(xscale, yscale);
	m_pixelsToMM.scale(1.0 / xscale, 1.0 / yscale);
}
