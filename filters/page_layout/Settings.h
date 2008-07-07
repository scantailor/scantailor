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

#ifndef PAGE_LAYOUT_SETTINGS_H_
#define PAGE_LAYOUT_SETTINGS_H_

#include "NonCopyable.h"
#include "RefCountable.h"
#include "Margins.h"
#include "PageId.h"
#include <QMutex>
#include <map>

namespace page_layout
{

class Settings : public RefCountable
{
	DECLARE_NON_COPYABLE(Settings)
public:
	Settings();
	
	virtual ~Settings();
	
	Margins getPageMarginsMM(PageId const& page_id) const;
	
	void setPageMarginsMM(PageId const& page_id, Margins const& margins);
private:
	typedef std::map<PageId, Margins> PerPageMargins;
	
	mutable QMutex m_mutex;
	PerPageMargins m_perPageMarginsMM;
	Margins m_defaultMarginsMM;
};

} // namespace page_layout

#endif
