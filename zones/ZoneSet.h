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

#ifndef ZONE_SET_H_
#define ZONE_SET_H_

#include "Zone.h"
#include <boost/iterator/iterator_facade.hpp>
#include <list>

class PropertyFactory;
class QDomDocument;
class QDomElement;
class QString;

class ZoneSet
{
public:
	class const_iterator : public boost::iterator_facade<
		const_iterator, Zone const, boost::forward_traversal_tag
	>
	{
		friend class ZoneSet;
		friend class boost::iterator_core_access;
	public:
		const_iterator() {}

		void increment() { ++m_it; }

		bool equal(const_iterator const& other) const {
			return m_it == other.m_it;
		}

		Zone const& dereference() const { return *m_it; }
	private:
		explicit const_iterator(std::list<Zone>::const_iterator it) : m_it(it) {}

		std::list<Zone>::const_iterator m_it;
	};

	typedef const_iterator iterator;


	ZoneSet() {}

	ZoneSet(QDomElement const& el, PropertyFactory const& prop_factory);

	virtual ~ZoneSet() {}

	QDomElement toXml(QDomDocument& doc, QString const& name) const;

	bool empty() const { return m_zones.empty(); }

	void add(Zone const& zone) { m_zones.push_back(zone); }

	const_iterator begin() const { return const_iterator(m_zones.begin()); }

	const_iterator end() const { return const_iterator(m_zones.end()); }
private:
	std::list<Zone> m_zones;
};

#endif
