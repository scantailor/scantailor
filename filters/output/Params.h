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
#include <QColor>

namespace output
{

class Params
{
public:
	enum ColorMode { BLACK_AND_WHITE, BITONAL, COLOR_GRAYSCALE };
	enum ThresholdMode { OTSU, SAUVOLA, WOLF };
	
	Params(Dpi const& dpi)
	: m_dpi(dpi), m_colorMode(BLACK_AND_WHITE), m_thresholdMode(OTSU) {}
	
	Dpi dpi() const { return m_dpi; }
	
	void setDpi(Dpi const& dpi) { m_dpi = dpi; }
	
	ColorMode colorMode() const { return m_colorMode; }
	
	void setColorMode(ColorMode mode) { m_colorMode = mode; }
	
	QColor const& lightColor() const { return m_lightColor; }
	
	void setLightColor(QColor const& color) { m_lightColor = color; }
	
	QColor const& darkColor() const { return m_darkColor; }
	
	void setDarkColor(QColor const& color) { m_darkColor = color; }
	
	ThresholdMode thresholdMode() const { return m_thresholdMode; }
	
	void setThresholdMode(ThresholdMode mode) { m_thresholdMode = mode; }
private:
	Dpi m_dpi;
	QColor m_lightColor;
	QColor m_darkColor;
	ColorMode m_colorMode;
	ThresholdMode m_thresholdMode;
};

} // namespace output

#endif
