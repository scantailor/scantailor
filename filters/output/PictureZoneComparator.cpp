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

#include "PictureZoneComparator.h"
#include "ZoneSet.h"
#include "Zone.h"
#include "PropertySet.h"
#include "PictureLayerProperty.h"
#include <QPolygonF>

namespace output
{

bool
PictureZoneComparator::equal(ZoneSet const& lhs, ZoneSet const& rhs)
{
	ZoneSet::const_iterator lhs_it(lhs.begin());
	ZoneSet::const_iterator rhs_it(rhs.begin());
	ZoneSet::const_iterator const lhs_end(lhs.end());
	ZoneSet::const_iterator const rhs_end(rhs.end());
	for (; lhs_it != lhs_end && rhs_it != rhs_end; ++lhs_it, ++rhs_it) {
		if (!equal(*lhs_it, *rhs_it)) {
			return false;
		}
	}

	return (lhs_it == lhs_end && rhs_it == rhs_end);
}

bool
PictureZoneComparator::equal(Zone const& lhs, Zone const& rhs)
{
	if (lhs.spline().toPolygon() != rhs.spline().toPolygon()) {
		return false;
	}

	return equal(lhs.properties(), rhs.properties());
}

bool
PictureZoneComparator::equal(PropertySet const& lhs, PropertySet const& rhs)
{
	typedef PictureLayerProperty PLP;
	return lhs.locateOrDefault<PLP>()->layer() == rhs.locateOrDefault<PLP>()->layer();
}

} // namespace output
