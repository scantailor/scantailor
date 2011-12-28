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

#ifndef SELECT_CONTENT_SETTINGS_H_
#define SELECT_CONTENT_SETTINGS_H_

#include "RefCountable.h"
#include "NonCopyable.h"
#include "PageId.h"
#include "Params.h"
#include <QMutex>
#include <memory>
#include <map>

class AbstractRelinker;

namespace select_content
{

class Settings : public RefCountable
{
	DECLARE_NON_COPYABLE(Settings)
public:
	Settings();
	
	virtual ~Settings();
	
	void clear();
	
	void performRelinking(AbstractRelinker const& relinker);

	void setPageParams(PageId const& page_id, Params const& params);
	
	void clearPageParams(PageId const& page_id);
	
	std::auto_ptr<Params> getPageParams(PageId const& page_id) const;
private:
	typedef std::map<PageId, Params> PageParams;
	
	mutable QMutex m_mutex;
	PageParams m_pageParams;
};

} // namespace select_content

#endif
