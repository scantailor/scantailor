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

#include "Params.h"
#include "XmlMarshaller.h"
#include "XmlUnmarshaller.h"
#include <QDomDocument>
#include <QDomElement>
#include <QByteArray>
#include <QString>
#include <QColor>
#include <stdio.h>

namespace output
{

Params::Params(QDomElement const& el)
:	m_dpi(XmlUnmarshaller::dpi(el.namedItem("dpi").toElement()))
{
	QDomElement const cp(el.namedItem("color-params").toElement());
	m_colorParams.setLightColor(
		parseColor(cp.attribute("light"), 0xFFFFFFFF)
	);
	m_colorParams.setDarkColor(
		parseColor(cp.attribute("dark"), 0x00000000)
	);
	m_colorParams.setColorMode(
		parseColorMode(cp.attribute("colorMode"))
	);
	m_colorParams.setThresholdMode(
		parseThresholdMode(cp.attribute("thresholdMode"))
	);
}

QDomElement
Params::toXml(QDomDocument& doc, QString const& name) const
{
	XmlMarshaller marshaller(doc);
	
	QDomElement el(doc.createElement(name));
	el.appendChild(marshaller.dpi(m_dpi, "dpi"));
	
	QDomElement cp(doc.createElement("color-params"));
	cp.setAttribute("light", formatColor(m_colorParams.lightColor()));
	cp.setAttribute("dark", formatColor(m_colorParams.darkColor()));
	cp.setAttribute(
		"colorMode",
		formatColorMode(m_colorParams.colorMode())
	);
	cp.setAttribute(
		"thresholdMode",
		formatThresholdMode(m_colorParams.thresholdMode())
	);
	
	el.appendChild(cp);
	
	return el;
}

QRgb
Params::parseColor(QString const& str, QRgb const dflt)
{
	QByteArray const ba(str.toAscii());
	if (ba.size() != 7) {
		return dflt;
	}
	if (ba[0] != '#') {
		return dflt;
	}
	
	unsigned rgb = 0;
	sscanf(ba.data(), "#%x", &rgb);
	return rgb | 0xFF000000; // set alpha value
}

QString
Params::formatColor(QRgb const color)
{
	QString str;
	str.sprintf("#%02x%02x%02x", qRed(color), qGreen(color), qBlue(color));
	return str;
}

ColorParams::ColorMode
Params::parseColorMode(QString const& str)
{
	if (str == "bw") {
		return ColorParams::BLACK_AND_WHITE;
	} else if (str == "bitonal") {
		return ColorParams::BITONAL;
	} else if (str == "colorOrGray") {
		return ColorParams::COLOR_GRAYSCALE;
	} else if (str == "mixed") {
		return ColorParams::MIXED;
	} else {
		return ColorParams::BLACK_AND_WHITE;
	}
}

QString
Params::formatColorMode(ColorParams::ColorMode const mode)
{
	char const* str = "";
	switch (mode) {
		case ColorParams::BLACK_AND_WHITE:
			str = "bw";
			break;
		case ColorParams::BITONAL:
			str = "bitonal";
			break;
		case ColorParams::COLOR_GRAYSCALE:
			str = "colorOrGray";
			break;
		case ColorParams::MIXED:
			str = "mixed";
			break;
	}
	return QString::fromAscii(str);
}

ColorParams::ThresholdMode
Params::parseThresholdMode(QString const& str)
{
	if (str == "mokji") {
		return ColorParams::MOKJI;
	} else if (str == "otsu") {
		return ColorParams::OTSU;
	} else if (str == "sauvola") {
		return ColorParams::SAUVOLA;
	} else if (str == "wolf") {
		return ColorParams::WOLF;
	} else {
		return ColorParams::OTSU;
	}
}

QString
Params::formatThresholdMode(ColorParams::ThresholdMode const mode)
{
	char const* str = "";
	switch (mode) {
		case ColorParams::MOKJI:
			str = "mokji";
			break;
		case ColorParams::OTSU:
			str = "otsu";
			break;
		case ColorParams::SAUVOLA:
			str = "sauvola";
			break;
		case ColorParams::WOLF:
			str = "wolf";
			break;
	}
	return QString::fromAscii(str);
}

} // namespace output
