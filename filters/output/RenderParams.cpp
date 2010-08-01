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

#include "RenderParams.h"
#include "ColorParams.h"
#include "ColorGrayscaleOptions.h"
#include "DespeckleLevel.h"

namespace output
{

RenderParams::RenderParams(ColorParams const& cp)
:	m_mask(0)
{
	switch (cp.colorMode()) {
		case ColorParams::BLACK_AND_WHITE:
			m_mask |= WHITE_MARGINS|NORMALIZE_ILLUMINATION
					|NEED_BINARIZATION;
			break;
		case ColorParams::COLOR_GRAYSCALE: {
			ColorGrayscaleOptions const opt(
				cp.colorGrayscaleOptions()
			);
			if (opt.whiteMargins()) {
				m_mask |= WHITE_MARGINS;
				if (opt.normalizeIllumination()) {
					m_mask |= NORMALIZE_ILLUMINATION;
				}
			}
			break;
		}
		case ColorParams::MIXED:
			m_mask |= WHITE_MARGINS|NORMALIZE_ILLUMINATION
					|NEED_BINARIZATION|MIXED_OUTPUT;
			break;
	}
}

} // namespace output
