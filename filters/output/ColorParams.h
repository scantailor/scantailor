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

#ifndef OUTPUT_COLORPARAMS_H_
#define OUTPUT_COLORPARAMS_H_

#include "ColorGrayscaleOptions.h"
#include "BlackWhiteOptions.h"

class QDomDocument;
class QDomElement;

namespace output
{

class ColorParams
{
public:
	enum ColorMode { BLACK_AND_WHITE, COLOR_GRAYSCALE, MIXED };
	
	ColorParams(): m_colorMode(BLACK_AND_WHITE) {}
	
	ColorParams(QDomElement const& el);
	
	QDomElement toXml(QDomDocument& doc, QString const& name) const;
	
	ColorMode colorMode() const { return m_colorMode; }
	
	void setColorMode(ColorMode mode) { m_colorMode = mode; }
	
	ColorGrayscaleOptions const& colorGrayscaleOptions() const {
		return m_colorGrayscaleOptions;
	}
	
	void setColorGrayscaleOptions(ColorGrayscaleOptions const& opt) {
		m_colorGrayscaleOptions = opt;
	}
	
	BlackWhiteOptions const& blackWhiteOptions() const {
		return m_bwOptions;
	}
	
	void setBlackWhiteOptions(BlackWhiteOptions const& opt) {
		m_bwOptions = opt;
	}
private:
	static ColorMode parseColorMode(QString const& str);
	
	static QString formatColorMode(ColorMode mode);
	
	ColorMode m_colorMode;
	ColorGrayscaleOptions m_colorGrayscaleOptions;
	BlackWhiteOptions m_bwOptions;
};

} // namespace output

#endif
