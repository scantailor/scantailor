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

#include "Rule.h"
#include <assert.h>

namespace page_split
{

QString
Rule::layoutTypeAsString() const
{
	return layoutTypeToString(m_layoutType);
}

QString
Rule::layoutTypeToString(LayoutType const layout_type)
{
	switch (layout_type) {
		case AUTO_DETECT:
			return "auto-detect";
		case SINGLE_PAGE:
			return "single-page";
		case TWO_PAGES:
			return "two-pages";
	}
	assert(!"unreachable");
	return QString();
}

Rule::LayoutType
Rule::layoutTypeFromString(QString const& layout_type)
{
	if (layout_type == "single-page") {
		return SINGLE_PAGE;
	} else if (layout_type == "two-pages") {
		return TWO_PAGES;
	} else {
		return AUTO_DETECT;
	}
}

} // namespace page_split
