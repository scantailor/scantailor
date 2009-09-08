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

#ifndef OUTPUT_PICTURE_ZONE_H_
#define OUTPUT_PICTURE_ZONE_H_

#include "Zone.h"

namespace output
{

class PictureZone : public Zone
{
public:
	enum Type { NO_OP, ERASER1, PAINTER2, ERASER3 };

	PictureZone(QPolygonF const& shape, Type type);

	PictureZone(QDomElement const& el);

	virtual QDomElement toXml(QDomDocument& doc, QString const& name) const;

	Type type() const { return m_type; }

	void setType(Type type) { m_type = type; }

	bool operator==(PictureZone const& other) const {
		return shape() == other.shape() && m_type == other.m_type;
	}

	bool operator!=(PictureZone const& other) const {
		return !(*this == other);
	}
private:
	static QString typeToString(Type type);

	static Type typeFromString(QString const& str);

	Type m_type;
};

} // namespace output

#endif
