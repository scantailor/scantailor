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

#ifndef PROPERTY_FACTORY_H_
#define PROPERTY_FACTORY_H_

#include "Property.h"
#include "IntrusivePtr.h"
#include <QString>
#include <map>

class QDomElement;

class PropertyFactory
{
	// Member-wise copying is OK.
public:
	virtual ~PropertyFactory() {}

	typedef IntrusivePtr<Property> (*PropertyConstructor)(QDomElement const& el);

	void registerProperty(QString const& property, PropertyConstructor constructor);

	IntrusivePtr<Property> construct(QDomElement const& el) const;
private:
	typedef std::map<QString, PropertyConstructor> Registry;
	Registry m_registry;
};

#endif
