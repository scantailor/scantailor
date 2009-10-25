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

#ifndef OUTPUT_PICTURE_LAYER_PROPERTY_H_
#define OUTPUT_PICTURE_LAYER_PROPERTY_H_

#include "Property.h"
#include "IntrusivePtr.h"

class PropertyFactory;
class QDomDocument;
class QDomElement;
class QString;

namespace output
{

class PictureLayerProperty : public Property
{
public:
	enum Layer { NO_OP, ERASER1, PAINTER2, ERASER3 };

	PictureLayerProperty(Layer layer = NO_OP) : m_layer(layer) {}

	PictureLayerProperty(QDomElement const& el);

	static void registerIn(PropertyFactory& factory);

	virtual IntrusivePtr<Property> clone() const;

	virtual QDomElement toXml(QDomDocument& doc, QString const& name) const;

	Layer layer() const { return m_layer; }

	void setLayer(Layer layer) { m_layer = layer; }
private:
	static IntrusivePtr<Property> construct(QDomElement const& el);

	static Layer layerFromString(QString const& str);

	static QString layerToString(Layer layer);

	static char const m_propertyName[];
	Layer m_layer;
};

} // namespace output

#endif
