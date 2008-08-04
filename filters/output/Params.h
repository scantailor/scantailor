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

#ifndef OUTPUT_PARAMS_H_
#define OUTPUT_PARAMS_H_

#include "Dpi.h"
#include "ColorParams.h"

class QDomDocument;
class QDomElement;

namespace output
{

class Params
{
public:
	Params(Dpi const& dpi, ColorParams const& color_params)
	: m_dpi(dpi), m_colorParams(color_params) {}
	
	Params(QDomElement const& el);
	
	Dpi const& dpi() const { return m_dpi; }
	
	ColorParams const& colorParams() const { return m_colorParams; }
	
	QDomElement toXml(QDomDocument& doc, QString const& name) const;
private:
	static QRgb parseColor(QString const& str, QRgb dflt);
	
	static ColorParams::ColorMode parseColorMode(QString const& str);
	
	static ColorParams::ThresholdMode parseThresholdMode(QString const& str);
	
	static QString formatColor(QRgb color);
	
	static QString formatColorMode(ColorParams::ColorMode mode);
	
	static QString formatThresholdMode(ColorParams::ThresholdMode mode);
	
	Dpi m_dpi;
	ColorParams m_colorParams;
};

} // namespace output

#endif
