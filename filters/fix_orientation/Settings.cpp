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
#include "Utils.h"
#include "RelinkablePath.h"
#include "AbstractRelinker.h"
#ifndef Q_MOC_RUN
#include <boost/foreach.hpp>
#endif

namespace fix_orientation
{

Settings::Settings()
{
}

Settings::~Settings()
{
}

void
Settings::clear()
{
	QMutexLocker locker(&m_mutex);
	m_perImageRotation.clear();
}

void
Settings::performRelinking(AbstractRelinker const& relinker)
{
	QMutexLocker locker(&m_mutex);
	PerImageRotation new_rotations;

	BOOST_FOREACH(PerImageRotation::value_type const& kv, m_perImageRotation) {
		RelinkablePath const old_path(kv.first.filePath(), RelinkablePath::File);
		ImageId new_image_id(kv.first);
		new_image_id.setFilePath(relinker.substitutionPathFor(old_path));
		new_rotations.insert(PerImageRotation::value_type(new_image_id, kv.second));
	}

	m_perImageRotation.swap(new_rotations);
}

void
Settings::applyRotation(
	ImageId const& image_id, OrthogonalRotation const rotation)
{
	QMutexLocker locker(&m_mutex);
	setImageRotationLocked(image_id, rotation);
}

void
Settings::applyRotation(
	std::set<PageId> const& pages, OrthogonalRotation const rotation)
{
	QMutexLocker locker(&m_mutex);
	
	BOOST_FOREACH(PageId const& page, pages) {
		setImageRotationLocked(page.imageId(), rotation);
	}
}

OrthogonalRotation
Settings::getRotationFor(ImageId const& image_id) const
{
	QMutexLocker locker(&m_mutex);
	
	PerImageRotation::const_iterator it(m_perImageRotation.find(image_id));
	if (it != m_perImageRotation.end()) {
		return it->second;
	} else {
		return OrthogonalRotation();
	}
}

void
Settings::setImageRotationLocked(
	ImageId const& image_id, OrthogonalRotation const& rotation)
{
	Utils::mapSetValue(m_perImageRotation, image_id, rotation);
}

} // namespace fix_orientation
