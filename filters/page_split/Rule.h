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

#ifndef PAGE_SPLIT_RULE_H_
#define PAGE_SPLIT_RULE_H_

#include <QString>

namespace page_split
{

class Rule
{
public:
	enum LayoutType {
		AUTO_DETECT,
		SINGLE_PAGE_UNCUT,
		PAGE_PLUS_OFFCUT,
		TWO_PAGES
	};
	
	enum Scope { THIS_PAGE_ONLY, ALL_PAGES };
	
	Rule() : m_layoutType(AUTO_DETECT), m_scope(ALL_PAGES) {}
	
	Rule(LayoutType layout_type, Scope scope)
	: m_layoutType(layout_type), m_scope(scope) {}
	
	LayoutType layoutType() const { return m_layoutType; }
	
	Scope scope() const { return m_scope; }
	
	QString layoutTypeAsString() const;
	
	static QString layoutTypeToString(LayoutType type);
	
	static LayoutType layoutTypeFromString(QString const& layout_type);
private:
	LayoutType m_layoutType;
	Scope m_scope;
};

} // namespace page_split

#endif
