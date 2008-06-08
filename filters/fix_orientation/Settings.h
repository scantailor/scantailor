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

#ifndef FIX_ORIENTATION_SETTINGS_H_
#define FIX_ORIENTATION_SETTINGS_H_

#include "RefCountable.h"
#include "NonCopyable.h"
#include "IntrusivePtr.h"
#include "OrthogonalRotation.h"
#include "ImageId.h"
#include <QMutex>
#include <vector>
#include <map>

class PageSequence;

namespace fix_orientation
{

class Scope;

class Settings : public RefCountable
{
	DECLARE_NON_COPYABLE(Settings)
public:
	Settings(IntrusivePtr<PageSequence> const& page_sequence);
	
	virtual ~Settings();
	
	void clear();
	
	void applyRule(ImageId const& image_id, OrthogonalRotation rotation);
	
	std::vector<ImageId> applyRule(Scope const& scope, OrthogonalRotation rotation);
	
	OrthogonalRotation getRotationFor(ImageId const& image_id) const;
private:
	typedef std::map<ImageId, OrthogonalRotation> PerImageRotation;
	
	void setImageRotationLocked(
		ImageId const& image_id, OrthogonalRotation const& rotation);
	
	mutable QMutex m_mutex;
	IntrusivePtr<PageSequence> m_ptrPages;
	PerImageRotation m_perImageRotation;
};

} // namespace fix_orientation

#endif
