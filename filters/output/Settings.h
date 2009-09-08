/*
    Scan Tailor - Interactive post-processing tool for scanned pages.
    Copyright (C) 2007-2009  Joseph Artsimovich <joseph_a@mail.ru>

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

#ifndef OUTPUT_SETTINGS_H_
#define OUTPUT_SETTINGS_H_

#include "RefCountable.h"
#include "NonCopyable.h"
#include "PageId.h"
#include "Dpi.h"
#include "ColorParams.h"
#include "OutputParams.h"
#include "PictureZoneList.h"
#include <QMutex>
#include <map>
#include <memory>

namespace output
{

class Params;

class Settings : public RefCountable
{
	DECLARE_NON_COPYABLE(Settings)
public:
	Settings();
	
	virtual ~Settings();
	
	void clear();
	
	Params getParams(PageId const& page_id) const;
	
	void setParams(PageId const& page_id, Params const& params);
	
	ColorParams getColorParams(PageId const& page_id) const;
	
	void setColorParams(PageId const& page_id, ColorParams const& params);
	
	void setColorParamsForAllPages(ColorParams const& params);
	
	Dpi getDpi(PageId const& page_id) const;
	
	void setDpi(PageId const& page_id, Dpi const& dpi);
	
	void setDpiForAllPages(Dpi const& dpi);
	
	std::auto_ptr<OutputParams> getOutputParams(PageId const& page_id) const;
	
	void removeOutputParams(PageId const& page_id);
	
	void setOutputParams(PageId const& page_id, OutputParams const& params);

	PictureZoneList zonesForPage(PageId const& page_id) const;

	void setZones(PageId const& page_id, PictureZoneList const& zones);
private:
	typedef std::map<PageId, Dpi> PerPageDpi;
	typedef std::map<PageId, ColorParams> PerPageColorParams;
	typedef std::map<PageId, OutputParams> PerPageOutputParams;
	typedef std::map<PageId, PictureZoneList> PerPageZones;
	
	Dpi getDpiLocked(PageId const& page_id) const;
	
	void setDpiLocked(PageId const& page_id, Dpi const& dpi);
	
	ColorParams getColorParamsLocked(PageId const& page_id) const;
	
	void setColorParamsLocked(
		PageId const& page_id, ColorParams const& params);
	
	mutable QMutex m_mutex;
	PerPageDpi m_perPageDpi;
	PerPageColorParams m_perPageColorParams;
	PerPageOutputParams m_perPageOutputParams;
	PerPageZones m_perPageZones;
	Dpi m_defaultDpi;
	ColorParams m_defaultColorParams;
};

} // namespace output

#endif
