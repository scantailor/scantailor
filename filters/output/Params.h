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
#include "DewarpingMode.h"
#include "dewarping/DistortionModel.h"
#include "DepthPerception.h"
#include "DespeckleLevel.h"

class QDomDocument;
class QDomElement;

namespace output
{

class Params
{
public:
	Params();
	
	Params(QDomElement const& el);
	
	Dpi const& outputDpi() const { return m_dpi; }

	void setOutputDpi(Dpi const& dpi) { m_dpi = dpi; }
	
	ColorParams const& colorParams() const { return m_colorParams; }

	void setColorParams(ColorParams const& params) { m_colorParams = params; }

	DewarpingMode const& dewarpingMode() const { return m_dewarpingMode; }

	void setDewarpingMode(DewarpingMode const& mode) { m_dewarpingMode = mode; }

	dewarping::DistortionModel const& distortionModel() const { return m_distortionModel; }

	void setDistortionModel(dewarping::DistortionModel const& model) { m_distortionModel = model; }

	DepthPerception const& depthPerception() const { return m_depthPerception; }

	void setDepthPerception(DepthPerception depth_perception) {
		m_depthPerception = depth_perception;
	}

	DespeckleLevel despeckleLevel() const { return m_despeckleLevel; }

	void setDespeckleLevel(DespeckleLevel level) { m_despeckleLevel = level; }
	
	QDomElement toXml(QDomDocument& doc, QString const& name) const;
private:
	static ColorParams::ColorMode parseColorMode(QString const& str);
	
	static QString formatColorMode(ColorParams::ColorMode mode);
	
	Dpi m_dpi;
	ColorParams m_colorParams;
	dewarping::DistortionModel m_distortionModel;
	DepthPerception m_depthPerception;
	DewarpingMode m_dewarpingMode;
	DespeckleLevel m_despeckleLevel;
};

} // namespace output

#endif
