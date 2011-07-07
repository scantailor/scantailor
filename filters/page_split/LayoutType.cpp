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

#include "LayoutType.h"
#include <assert.h>

namespace page_split
{

QString layoutTypeToString(LayoutType const layout_type)
{
	switch (layout_type) {
		case AUTO_LAYOUT_TYPE:
			return "auto-detect";
		case SINGLE_PAGE_UNCUT:
			return "single-uncut";
		case PAGE_PLUS_OFFCUT:
			return "single-cut";
		case TWO_PAGES:
			return "two-pages";
	}
	assert(!"unreachable");
	return QString();
}

LayoutType layoutTypeFromString(QString const& layout_type)
{
	if (layout_type == "single-uncut") {
		return SINGLE_PAGE_UNCUT;
	} else if (layout_type == "single-cut") {
		return PAGE_PLUS_OFFCUT;
	} else if (layout_type == "two-pages") {
		return TWO_PAGES;
	} else {
		return AUTO_LAYOUT_TYPE;
	}
}

} // namespace page_split
