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

#ifndef XMLMARSHALLER_H_
#define XMLMARSHALLER_H_

#include <QDomDocument>
#include <QDomElement>
#include <QString>

class QSize;
class QSizeF;
class Dpi;
class OrthogonalRotation;
class Margins;
class QPointF;
class QLineF;
class QPolygonF;
class QRect;
class QRectF;

class XmlMarshaller
{
public:
	XmlMarshaller(QDomDocument const& doc) : m_doc(doc) {}
	
	QDomElement string(QString const& str, QString const& name);
	
	QDomElement size(QSize const& size, QString const& name);
	
	QDomElement sizeF(QSizeF const& size, QString const& name);
	
	QDomElement dpi(Dpi const& dpi, QString const& name);
	
	QDomElement rotation(OrthogonalRotation const& rotation, QString const& name);
	
	QDomElement pointF(QPointF const& p, QString const& name);
	
	QDomElement lineF(QLineF const& line, QString const& name);
	
	QDomElement rect(QRect const& rect, QString const& name);
	
	QDomElement rectF(QRectF const& rect, QString const& name);
	
	QDomElement polygonF(QPolygonF const& poly, QString const& name);
	
	QDomElement margins(Margins const& margins, QString const& name);
private:
	QDomDocument m_doc;
};

#endif
