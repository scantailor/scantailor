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

#include "LayoutTypeResolver.h"
#include "ImageMetadata.h"
#include "OrthogonalRotation.h"
#include "PageSequence.h"
#include <assert.h>

namespace page_split
{

int
LayoutTypeResolver::numLogicalPages(
	ImageMetadata const& metadata, OrthogonalRotation const pre_rotation) const
{
	switch (m_layoutType) {
		case Rule::AUTO_DETECT: {
			return PageSequence::adviseNumberOfLogicalPages(
				metadata, pre_rotation
			);
		}
		case Rule::SINGLE_PAGE: {
			return 1;
		}
		case Rule::TWO_PAGES: {
			return 2;
		}
	}
	
	assert(!"unreachable");
	return 1;
}

} // namespace page_split
