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

#include "Settings.h"
#include "Scope.h"
#include "PageSequence.h"
#include "Utils.h"

namespace fix_orientation
{

Settings::Settings(
	IntrusivePtr<PageSequence> const& page_sequence)
:	m_ptrPages(page_sequence)
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
Settings::applyRule(
	ImageId const& image_id, OrthogonalRotation const rotation)
{
	QMutexLocker locker(&m_mutex);
	setImageRotationLocked(image_id, rotation);
}

std::vector<ImageId>
Settings::applyRule(
	Scope const& scope, OrthogonalRotation const rotation)
{
	PageSequenceSnapshot const snapshot(
		m_ptrPages->snapshot(PageSequence::IMAGE_VIEW)
	);
	int const num_images = snapshot.numPages();
	
	std::vector<ImageId> image_ids;
	
	QMutexLocker locker(&m_mutex);
	
	for (int i = scope.from(); i <= scope.to() && i < num_images; ++i) {
		if ((scope.origin() - i) % scope.step() == 0) {
			ImageId const image_id(snapshot.pageAt(i).imageId());
			image_ids.push_back(image_id);
			setImageRotationLocked(image_id, rotation);
		}
	}
	
	return image_ids;
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
	m_ptrPages->autoSetLogicalPagesInImage(image_id, rotation);
}

} // namespace fix_orientation
