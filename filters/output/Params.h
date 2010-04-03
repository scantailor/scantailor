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

#ifndef OUTPUT_PARAMS_H_
#define OUTPUT_PARAMS_H_

#include "Dpi.h"
#include "ColorParams.h"
#include "DespeckleLevel.h"

class QDomDocument;
class QDomElement;

namespace output
{

class Params
{
public:
	Params(Dpi const& output_dpi, ColorParams const& color_params,
		DespeckleLevel despeckle_level);
	
	Params(QDomElement const& el);
	
	Dpi const& outputDpi() const { return m_dpi; }
	
	ColorParams const& colorParams() const { return m_colorParams; }

	DespeckleLevel despeckleLevel() const { return m_despeckleLevel; }
	
	QDomElement toXml(QDomDocument& doc, QString const& name) const;
private:
	static ColorParams::ColorMode parseColorMode(QString const& str);
	
	static QString formatColorMode(ColorParams::ColorMode mode);
	
	Dpi m_dpi;
	ColorParams m_colorParams;
	DespeckleLevel m_despeckleLevel;
};

} // namespace output

#endif
