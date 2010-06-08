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

#ifndef OUTPUT_FILL_COLOR_PROPERTY_H_
#define OUTPUT_FILL_COLOR_PROPERTY_H_

#include "Property.h"
#include "IntrusivePtr.h"
#include <QColor>
#include <Qt>

class PropertyFactory;
class QDomDocument;
class QDomElement;
class QString;

namespace output
{

class FillColorProperty : public Property
{
public:
	FillColorProperty(QColor const& color = Qt::white) : m_rgb(color.rgb()) {}

	FillColorProperty(QDomElement const& el);

	static void registerIn(PropertyFactory& factory);

	virtual IntrusivePtr<Property> clone() const;

	virtual QDomElement toXml(QDomDocument& doc, QString const& name) const;

	QColor color() const { return QColor(m_rgb); }

	void setColor(QColor const& color) { m_rgb = color.rgb(); }
private:
	static IntrusivePtr<Property> construct(QDomElement const& el);

	static QRgb rgbFromString(QString const& str);

	static QString rgbToString(QRgb rgb);

	static char const m_propertyName[];
	QRgb m_rgb;
};

} // namespace output

#endif
