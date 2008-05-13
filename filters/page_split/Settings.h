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

#ifndef PAGE_SPLIT_SETTINGS_H_
#define PAGE_SPLIT_SETTINGS_H_

#include "RefCountable.h"
#include "NonCopyable.h"
#include "Rule.h"
#include "Params.h"
#include "ImageId.h"
#include <QMutex>
#include <memory>
#include <map>

namespace page_split
{

class Settings : public RefCountable
{
	DECLARE_NON_COPYABLE(Settings)
public:
	Settings();
	
	virtual ~Settings();
	
	void clear();
	
	void applyToPage(ImageId const& image_id, Rule::LayoutType layout_type);
	
	void applyToAllPages(Rule::LayoutType layout_type);
	
	Rule getRuleFor(ImageId const& image_id) const;
	
	Rule::LayoutType defaultLayoutType() const;
	
	void setPageParams(ImageId const& image_id, Params const& params);
	
	void clearPageParams(ImageId const& image_id);
	
	std::auto_ptr<Params> getPageParams(ImageId const& image_id) const;
private:
	typedef std::map<ImageId, Rule::LayoutType> PerPageLayoutType;
	typedef std::map<ImageId, Params> PerPageParams;
	
	mutable QMutex m_mutex;
	PerPageLayoutType m_perPageLayoutType;
	PerPageParams m_perPageParams;
	Rule::LayoutType m_defaultLayoutType;
};

} // namespace page_split

#endif
