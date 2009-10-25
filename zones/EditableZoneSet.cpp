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

#include "EditableZoneSet.h.moc"

EditableZoneSet::EditableZoneSet()
:	m_ptrDefaultProps(new PropertySet)
{
}

void
EditableZoneSet::setDefaultProperties(PropertySet const& props)
{
	m_ptrDefaultProps = props.deepCopy();
}

void
EditableZoneSet::addZone(EditableSpline::Ptr const& spline)
{
	m_splineMap.insert(Map::value_type(spline, m_ptrDefaultProps->deepCopy()));
}

void
EditableZoneSet::addZone(EditableSpline::Ptr const& spline, PropertySet const& props)
{
	m_splineMap.insert(Map::value_type(spline, props.deepCopy()));
}

void
EditableZoneSet::removeZone(EditableSpline::Ptr const& spline)
{
	m_splineMap.erase(spline);
}

void
EditableZoneSet::setProperties(EditableSpline::Ptr const& spline, IntrusivePtr<PropertySet> const& props)
{
	Map::iterator it(m_splineMap.find(spline));
	if (it != m_splineMap.end()) {
		it->second = props;
	}
}

void
EditableZoneSet::commit()
{
	emit committed();
}

IntrusivePtr<PropertySet>
EditableZoneSet::propertiesFor(EditableSpline::Ptr const& spline)
{
	Map::iterator it(m_splineMap.find(spline));
	if (it != m_splineMap.end()) {
		return it->second;
	} else {
		return IntrusivePtr<PropertySet>();
	}
}

IntrusivePtr<PropertySet const>
EditableZoneSet::propertiesFor(EditableSpline::Ptr const& spline) const
{
	Map::const_iterator it(m_splineMap.find(spline));
	if (it != m_splineMap.end()) {
		return it->second;
	} else {
		return IntrusivePtr<PropertySet const>();
	}
}
