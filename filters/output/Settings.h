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

#ifndef OUTPUT_SETTINGS_H_
#define OUTPUT_SETTINGS_H_

#include "RefCountable.h"
#include "NonCopyable.h"
#include "PageId.h"
#include "Dpi.h"
#include "ColorParams.h"
#include "OutputParams.h"
#include "DewarpingMode.h"
#include "dewarping/DistortionModel.h"
#include "DespeckleLevel.h"
#include "ZoneSet.h"
#include "PropertySet.h"
#include <QMutex>
#include <map>
#include <memory>

class AbstractRelinker;

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

	void performRelinking(AbstractRelinker const& relinker);
	
	Params getParams(PageId const& page_id) const;
	
	void setParams(PageId const& page_id, Params const& params);

	void setColorParams(PageId const& page_id, ColorParams const& prms);

	void setDpi(PageId const& page_id, Dpi const& dpi);

	void setDewarpingMode(PageId const& page_id, DewarpingMode const& mode);

	void setDistortionModel(PageId const& page_id, dewarping::DistortionModel const& model);

	void setDepthPerception(PageId const& page_id, DepthPerception const& depth_perception);

	void setDespeckleLevel(PageId const& page_id, DespeckleLevel level);
	
	std::auto_ptr<OutputParams> getOutputParams(PageId const& page_id) const;
	
	void removeOutputParams(PageId const& page_id);
	
	void setOutputParams(PageId const& page_id, OutputParams const& params);

	ZoneSet pictureZonesForPage(PageId const& page_id) const;

	ZoneSet fillZonesForPage(PageId const& page_id) const;

	void setPictureZones(PageId const& page_id, ZoneSet const& zones);

	void setFillZones(PageId const& page_id, ZoneSet const& zones);

	/**
	 * For now, default zone properties are not persistent.
	 * They may become persistent later though.
	 */
	PropertySet defaultPictureZoneProperties() const;

	PropertySet defaultFillZoneProperties() const;

	void setDefaultPictureZoneProperties(PropertySet const& props);

	void setDefaultFillZoneProperties(PropertySet const& props);
private:
	typedef std::map<PageId, Params> PerPageParams;
	typedef std::map<PageId, OutputParams> PerPageOutputParams;
	typedef std::map<PageId, ZoneSet> PerPageZones;
	
	static PropertySet initialPictureZoneProps();

	static PropertySet initialFillZoneProps();

	mutable QMutex m_mutex;
	PerPageParams m_perPageParams;
	PerPageOutputParams m_perPageOutputParams;
	PerPageZones m_perPagePictureZones;
	PerPageZones m_perPageFillZones;
	PropertySet m_defaultPictureZoneProps;
	PropertySet m_defaultFillZoneProps;
};

} // namespace output

#endif
