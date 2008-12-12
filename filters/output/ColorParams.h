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

#ifndef OUTPUT_COLORPARAMS_H_
#define OUTPUT_COLORPARAMS_H_

#include <QColor>

namespace output
{

class ColorParams
{
public:
	enum ColorMode { BLACK_AND_WHITE, BITONAL, COLOR_GRAYSCALE, MIXED };
	enum ThresholdMode { OTSU, MOKJI, SAUVOLA, WOLF };
	
	ColorParams() : m_lightColor(0xFFFFFFFF), m_darkColor(0xFF000000),
	m_colorMode(BLACK_AND_WHITE), m_thresholdMode(OTSU) {}
	
	ColorMode colorMode() const { return m_colorMode; }
	
	void setColorMode(ColorMode mode) { m_colorMode = mode; }
	
	QRgb lightColor() const { return m_lightColor; }
	
	void setLightColor(QRgb color) { m_lightColor = color; }
	
	QRgb darkColor() const { return m_darkColor; }
	
	void setDarkColor(QRgb color) { m_darkColor = color; }
	
	ThresholdMode thresholdMode() const { return m_thresholdMode; }
	
	void setThresholdMode(ThresholdMode mode) { m_thresholdMode = mode; }
private:
	QRgb m_lightColor;
	QRgb m_darkColor;
	ColorMode m_colorMode;
	ThresholdMode m_thresholdMode;
};

} // namespace output

#endif
