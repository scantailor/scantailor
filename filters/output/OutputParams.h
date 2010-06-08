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

#ifndef OUTPUT_OUTPUT_PARAMS_H_
#define OUTPUT_OUTPUT_PARAMS_H_

#include "OutputImageParams.h"
#include "OutputFileParams.h"
#include "ZoneSet.h"

class QDomDocument;
class QDomElement;
class QString;

namespace output
{

class OutputParams
{
public:
	OutputParams(OutputImageParams const& output_image_params,
		OutputFileParams const& output_file_params,
		OutputFileParams const& automask_file_params,
		OutputFileParams const& speckles_file_params,
		ZoneSet const& picture_zones, ZoneSet const& fill_zones);
	
	explicit OutputParams(QDomElement const& el);
	
	QDomElement toXml(QDomDocument& doc, QString const& name) const;
	
	OutputImageParams const& outputImageParams() const { return m_outputImageParams; }
	
	OutputFileParams const& outputFileParams() const { return m_outputFileParams; }

	OutputFileParams const& automaskFileParams() const { return m_automaskFileParams; }

	OutputFileParams const& specklesFileParams() const { return m_specklesFileParams; }

	ZoneSet const& pictureZones() const { return m_pictureZones; }

	ZoneSet const& fillZones() const { return m_fillZones; }
private:
	OutputImageParams m_outputImageParams;
	OutputFileParams m_outputFileParams;
	OutputFileParams m_automaskFileParams;
	OutputFileParams m_specklesFileParams;
	ZoneSet m_pictureZones;
	ZoneSet m_fillZones;
};

} // namespace output

#endif
