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

#include "Settings.h"
#include "Params.h"
#include "PictureLayerProperty.h"
#include "FillColorProperty.h"
#include "RelinkablePath.h"
#include "AbstractRelinker.h"
#include "../../Utils.h"
#ifndef Q_MOC_RUN
#include <boost/foreach.hpp>
#endif
#include <Qt>
#include <QColor>
#include <QMutexLocker>

namespace output
{

Settings::Settings()
:	m_defaultPictureZoneProps(initialPictureZoneProps()),
	m_defaultFillZoneProps(initialFillZoneProps())
{
}

Settings::~Settings()
{
}

void
Settings::clear()
{
	QMutexLocker const locker(&m_mutex);
	
	initialPictureZoneProps().swap(m_defaultPictureZoneProps);
	initialFillZoneProps().swap(m_defaultFillZoneProps);
	m_perPageParams.clear();
	m_perPageOutputParams.clear();
	m_perPagePictureZones.clear();
	m_perPageFillZones.clear();
}

void
Settings::performRelinking(AbstractRelinker const& relinker)
{
	QMutexLocker const locker(&m_mutex);

	PerPageParams new_params;
	PerPageOutputParams new_output_params;
	PerPageZones new_picture_zones;
	PerPageZones new_fill_zones;

	BOOST_FOREACH(PerPageParams::value_type const& kv, m_perPageParams) {
		RelinkablePath const old_path(kv.first.imageId().filePath(), RelinkablePath::File);
		PageId new_page_id(kv.first);
		new_page_id.imageId().setFilePath(relinker.substitutionPathFor(old_path));
		new_params.insert(PerPageParams::value_type(new_page_id, kv.second));
	}

	BOOST_FOREACH(PerPageOutputParams::value_type const& kv, m_perPageOutputParams) {
		RelinkablePath const old_path(kv.first.imageId().filePath(), RelinkablePath::File);
		PageId new_page_id(kv.first);
		new_page_id.imageId().setFilePath(relinker.substitutionPathFor(old_path));
		new_output_params.insert(PerPageOutputParams::value_type(new_page_id, kv.second));
	}

	BOOST_FOREACH(PerPageZones::value_type const& kv, m_perPagePictureZones) {
		RelinkablePath const old_path(kv.first.imageId().filePath(), RelinkablePath::File);
		PageId new_page_id(kv.first);
		new_page_id.imageId().setFilePath(relinker.substitutionPathFor(old_path));
		new_picture_zones.insert(PerPageZones::value_type(new_page_id, kv.second));
	}

	BOOST_FOREACH(PerPageZones::value_type const& kv, m_perPageFillZones) {
		RelinkablePath const old_path(kv.first.imageId().filePath(), RelinkablePath::File);
		PageId new_page_id(kv.first);
		new_page_id.imageId().setFilePath(relinker.substitutionPathFor(old_path));
		new_fill_zones.insert(PerPageZones::value_type(new_page_id, kv.second));
	}

	m_perPageParams.swap(new_params);
	m_perPageOutputParams.swap(new_output_params);
	m_perPagePictureZones.swap(new_picture_zones);
	m_perPageFillZones.swap(new_fill_zones);
}

Params
Settings::getParams(PageId const& page_id) const
{
	QMutexLocker const locker(&m_mutex);
	
	PerPageParams::const_iterator const it(m_perPageParams.find(page_id));
	if (it != m_perPageParams.end()) {
		return it->second;
	} else {
		return Params();
	}
}

void
Settings::setParams(PageId const& page_id, Params const& params)
{
	QMutexLocker const locker(&m_mutex);
	Utils::mapSetValue(m_perPageParams, page_id, params);
}

void
Settings::setColorParams(PageId const& page_id, ColorParams const& prms)
{
	QMutexLocker const locker(&m_mutex);

	PerPageParams::iterator const it(m_perPageParams.lower_bound(page_id));
	if (it == m_perPageParams.end() || m_perPageParams.key_comp()(page_id, it->first)) {
		Params params;
		params.setColorParams(prms);
		m_perPageParams.insert(it, PerPageParams::value_type(page_id, params));
	} else {
		it->second.setColorParams(prms);
	}
}

void
Settings::setDpi(PageId const& page_id, Dpi const& dpi)
{
	QMutexLocker const locker(&m_mutex);

	PerPageParams::iterator const it(m_perPageParams.lower_bound(page_id));
	if (it == m_perPageParams.end() || m_perPageParams.key_comp()(page_id, it->first)) {
		Params params;
		params.setOutputDpi(dpi);
		m_perPageParams.insert(it, PerPageParams::value_type(page_id, params));
	} else {
		it->second.setOutputDpi(dpi);
	}
}

void
Settings::setDewarpingMode(PageId const& page_id, DewarpingMode const& mode)
{
	QMutexLocker const locker(&m_mutex);

	PerPageParams::iterator const it(m_perPageParams.lower_bound(page_id));
	if (it == m_perPageParams.end() || m_perPageParams.key_comp()(page_id, it->first)) {
		Params params;
		params.setDewarpingMode(mode);
		m_perPageParams.insert(it, PerPageParams::value_type(page_id, params));
	} else {
		it->second.setDewarpingMode(mode);
	}
}

void
Settings::setDistortionModel(PageId const& page_id, dewarping::DistortionModel const& model)
{
	QMutexLocker const locker(&m_mutex);

	PerPageParams::iterator const it(m_perPageParams.lower_bound(page_id));
	if (it == m_perPageParams.end() || m_perPageParams.key_comp()(page_id, it->first)) {
		Params params;
		params.setDistortionModel(model);
		m_perPageParams.insert(it, PerPageParams::value_type(page_id, params));
	} else {
		it->second.setDistortionModel(model);
	}
}

void
Settings::setDepthPerception(PageId const& page_id, DepthPerception const& depth_perception)
{
	QMutexLocker const locker(&m_mutex);

	PerPageParams::iterator const it(m_perPageParams.lower_bound(page_id));
	if (it == m_perPageParams.end() || m_perPageParams.key_comp()(page_id, it->first)) {
		Params params;
		params.setDepthPerception(depth_perception);
		m_perPageParams.insert(it, PerPageParams::value_type(page_id, params));
	} else {
		it->second.setDepthPerception(depth_perception);
	}
}

void
Settings::setDespeckleLevel(PageId const& page_id, DespeckleLevel level)
{
	QMutexLocker const locker(&m_mutex);

	PerPageParams::iterator const it(m_perPageParams.lower_bound(page_id));
	if (it == m_perPageParams.end() || m_perPageParams.key_comp()(page_id, it->first)) {
		Params params;
		params.setDespeckleLevel(level);
		m_perPageParams.insert(it, PerPageParams::value_type(page_id, params));
	} else {
		it->second.setDespeckleLevel(level);
	}
}

std::auto_ptr<OutputParams>
Settings::getOutputParams(PageId const& page_id) const
{
	QMutexLocker const locker(&m_mutex);
	
	PerPageOutputParams::const_iterator const it(m_perPageOutputParams.find(page_id));
	if (it != m_perPageOutputParams.end()) {
		return std::auto_ptr<OutputParams>(new OutputParams(it->second));
	} else {
		return std::auto_ptr<OutputParams>();
	}
}

void
Settings::removeOutputParams(PageId const& page_id)
{
	QMutexLocker const locker(&m_mutex);
	m_perPageOutputParams.erase(page_id);
}

void
Settings::setOutputParams(PageId const& page_id, OutputParams const& params)
{
	QMutexLocker const locker(&m_mutex);
	Utils::mapSetValue(m_perPageOutputParams, page_id, params);
}

ZoneSet
Settings::pictureZonesForPage(PageId const& page_id) const
{
	QMutexLocker const locker(&m_mutex);

	PerPageZones::const_iterator const it(m_perPagePictureZones.find(page_id));
	if (it != m_perPagePictureZones.end()) {
		return it->second;
	} else {
		return ZoneSet();
	}
}

ZoneSet
Settings::fillZonesForPage(PageId const& page_id) const
{
	QMutexLocker const locker(&m_mutex);

	PerPageZones::const_iterator const it(m_perPageFillZones.find(page_id));
	if (it != m_perPageFillZones.end()) {
		return it->second;
	} else {
		return ZoneSet();
	}
}

void
Settings::setPictureZones(PageId const& page_id, ZoneSet const& zones)
{
	QMutexLocker const locker(&m_mutex);
	Utils::mapSetValue(m_perPagePictureZones, page_id, zones);
}

void
Settings::setFillZones(PageId const& page_id, ZoneSet const& zones)
{
	QMutexLocker const locker(&m_mutex);
	Utils::mapSetValue(m_perPageFillZones, page_id, zones);
}

PropertySet
Settings::defaultPictureZoneProperties() const
{
	QMutexLocker const locker(&m_mutex);
	return m_defaultPictureZoneProps;
}

PropertySet
Settings::defaultFillZoneProperties() const
{
	QMutexLocker const locker(&m_mutex);
	return m_defaultFillZoneProps;
}

void
Settings::setDefaultPictureZoneProperties(PropertySet const& props)
{
	QMutexLocker const locker(&m_mutex);
	m_defaultPictureZoneProps = props;
}

void
Settings::setDefaultFillZoneProperties(PropertySet const& props)
{
	QMutexLocker const locker(&m_mutex);
	m_defaultFillZoneProps = props;
}

PropertySet
Settings::initialPictureZoneProps()
{
	PropertySet props;
	props.locateOrCreate<PictureLayerProperty>()->setLayer(PictureLayerProperty::PAINTER2);
	return props;
}

PropertySet
Settings::initialFillZoneProps()
{
	PropertySet props;
	props.locateOrCreate<FillColorProperty>()->setColor(Qt::white);
	return props;
}

} // namespace output
