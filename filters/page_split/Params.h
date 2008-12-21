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

#ifndef PAGE_SPLIT_PARAMS_H_
#define PAGE_SPLIT_PARAMS_H_

#include "PageLayout.h"
#include "Dependencies.h"
#include "AutoManualMode.h"
#include <QString>

class QDomDocument;
class QDomElement;

namespace page_split
{

class Params
{
public:
	// Member-wise copying is OK.
	
	Params(PageLayout const& layout,
		Dependencies const& deps, AutoManualMode split_line_mode);
	
	Params(QDomElement const& el);
	
	~Params();
	
	PageLayout const& pageLayout() const { return m_layout; }
	
	Dependencies const& dependencies() const { return m_deps; }
	
	AutoManualMode splitLineMode() const { return m_splitLineMode; }
	
	QDomElement toXml(QDomDocument& doc, QString const& name) const;
private:
	PageLayout m_layout;
	Dependencies m_deps;
	AutoManualMode m_splitLineMode;
};

} // namespace page_split

#endif
