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

#ifndef PAGE_SPLIT_LAYOUT_TYPE_H_
#define PAGE_SPLIT_LAYOUT_TYPE_H_

#include <QString>

namespace page_split
{

enum LayoutType {
	AUTO_LAYOUT_TYPE,
	SINGLE_PAGE_UNCUT,
	PAGE_PLUS_OFFCUT,
	TWO_PAGES
};

QString layoutTypeToString(LayoutType type);

LayoutType layoutTypeFromString(QString const& layout_type);

} // namespace page_split

#endif
